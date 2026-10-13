using Godot;
using System;
using System.Linq;

public partial class Battlefield
{
    // Original stereo Foley synthesis: turbulent air, impact bodies, metal modes,
    // granular rubble, sub-bass pressure and damped spatial reflections.
    public static AudioStreamWav MakeAkurothCue(string cue){
        const int rate=44100;
        string id=cue.Replace("akuroth-","").Replace("-cast","").Replace("-impact","");
        bool impact=cue.EndsWith("-impact")||id=="impact",transform=id=="transform";
        double duration=transform?3.5:impact?id switch{"annihilation"=>4.8,"collapse"=>3.1,"meteor"=>2.3,"rain"=>1.8,"cut" or "chaos"=>.8,_=>2.1}:id=="annihilation"?3.6:id is "prison" or "rewrite"?1.8:1.15;
        int count=(int)(duration*rate);var left=new double[count];var right=new double[count];
        int seed=817;foreach(char ch in cue)seed=unchecked(seed*31+ch);var random=new Random(seed);
        double low=0,middle=0,air=0;
        double baseHz=id switch{"cut"=>68,"waves"=>43,"prison"=>91,"meteor"=>34,"rift"=>48,"chaos"=>75,"collapse"=>29,"rain"=>39,"rewrite"=>62,"annihilation"=>25,_=>37};
        for(int n=0;n<count;n++){
            double t=n/(double)rate,noise=random.NextDouble()*2-1;
            low=.997*low+.003*noise;middle=.92*middle+.08*noise;air=.64*air+.36*noise;
            double wind=middle-low,grit=air-middle;
            double smooth(double x)=>Math.Clamp(x,0,1)*Math.Clamp(x,0,1)*(3-2*Math.Clamp(x,0,1));
            double tail=Math.Exp(-t*(id=="annihilation"?1.1:2.4));
            double tone(double hz)=>Math.Sin(Math.Tau*hz*t);
            double pressure=(tone(baseHz)+.35*tone(baseHz*1.49))*.13;
            double sample;
            if(!impact){
                double swell=Math.Pow(Math.Sin(Math.PI*Math.Clamp(t/duration,0,1)),1.7);
                sample=(wind*2.1+low*8+pressure*.6)*swell;
                if(id is "cut" or "chaos")sample=wind*3.0*Math.Exp(-Math.Pow((t-.23)/.17,2))+pressure*Math.Exp(-t*7);
                if(id is "prison" or "rewrite"){
                    double metal=0;
                    for(int k=0;k<9;k++){double a=t-k*.095;if(a>0)metal+=(Math.Sin(Math.Tau*(739+k*83)*a)*.045+Math.Sin(Math.Tau*(1261+k*97)*a)*.025)*Math.Exp(-a*14)*smooth(a/.004);}
                    sample+=metal;
                }
                if(id is "annihilation" or "transform")sample+=(tone(73)*.06+tone(109.4)*.05+tone(146.9)*.045)*swell*(.5+.5*tone(3.1));
            }else{
                sample=(pressure*2.3+low*10+wind*1.5)*tail*smooth(t/.008);
                sample+=grit*.4*Math.Exp(-t*18)*smooth(t/.002);
                if(id is "cut" or "chaos")sample=(wind*2.8+grit*.8)*Math.Exp(-t*8)+pressure*Math.Exp(-t*13);
                if(id=="prison")sample+=(tone(419)*.08+tone(857)*.045+tone(1439)*.02)*Math.Exp(-t*5);
                if(id is "meteor" or "collapse" or "annihilation"){
                    for(int k=0;k<12;k++){double a=t-.075-k*.087;if(a>0)sample+=(grit*.33+wind*.28)*Math.Exp(-a*(18+k))*smooth(a/.005);}
                }
                if(id=="rewrite")sample+=(tone(173)*.07+tone(231)*.05+tone(347)*.025)*Math.Exp(-t*2.5)*(1+.2*tone(5));
            }
            double end=Math.Min(1,(duration-t)/.15);sample*=Math.Min(1,t/.006)*Math.Max(0,end);
            left[n]=sample*(.97+.03*Math.Sin(t*2.4));right[n]=sample*(.97+.03*Math.Cos(t*2.7));
        }
        var wetLeft=(double[])left.Clone();var wetRight=(double[])right.Clone();
        int[] reflections={ (int)(rate*.071),(int)(rate*.127),(int)(rate*.211),(int)(rate*.337),(int)(rate*.519) };
        for(int k=0;k<reflections.Length;k++){
            int delay=reflections[k];double gain=.24*Math.Pow(.65,k);
            for(int n=delay;n<count;n++){wetLeft[n]+=(k%2==0?right[n-delay]:left[n-delay])*gain;wetRight[n]+=(k%2==0?left[n-delay]:right[n-delay])*gain;}
        }
        double peak=Math.Max(wetLeft.Max(Math.Abs),wetRight.Max(Math.Abs));double gainOut=peak>0?.76/Math.Max(1,peak):1;
        var bytes=new byte[count*4];
        for(int n=0;n<count;n++){
            short l=(short)(Math.Tanh(wetLeft[n]*gainOut)*26000),r=(short)(Math.Tanh(wetRight[n]*gainOut)*26000);
            bytes[n*4]=(byte)l;bytes[n*4+1]=(byte)(l>>8);bytes[n*4+2]=(byte)r;bytes[n*4+3]=(byte)(r>>8);
        }
        return new AudioStreamWav{Format=AudioStreamWav.FormatEnum.Format16Bits,MixRate=rate,Stereo=true,Data=bytes};
    }
}
