using System;
namespace EnigmaAudio;
public sealed class AudioStreamWav { public enum FormatEnum {Format16Bits} public FormatEnum Format;public int MixRate;public bool Stereo;public byte[] Data=Array.Empty<byte>(); }
public static class Generator {
public static AudioStreamWav MakeCue(string cue){
        if(cue.StartsWith("akuroth-"))return MakeAkurothCue(cue);
        const int rate=44100;double duration=cue.StartsWith("akuroth-")?cue is "akuroth-transform" or "akuroth-annihilation"?1.4:.7:cue is "heal" or "chime"?.65:cue is "leather" or "soft" or "metal" or "clawstep"?.15:.40;
        var bytes=new byte[(int)(rate*duration)*2];var random=new Random(626);double low=0,soft=0;
        for(int n=0;n<bytes.Length/2;n++){
            double t=n/(double)rate,noise=random.NextDouble()*2-1;low=.86*low+.14*noise;
            double tone(double hz)=>Math.Sin(Math.Tau*hz*t);
            double v=cue switch{
                "steel"=>(tone(1567)*.24+tone(2639)*.16+tone(4213)*.08)*Math.Exp(-t*13)+noise*.25*Math.Exp(-t*75),
                "kick"=>tone(92-50*t)*Math.Exp(-t*28)*.65+low*Math.Exp(-t*40)*1.2,
                "heavy"=>tone(57)*Math.Exp(-t*13)*.65+tone(279)*Math.Exp(-t*22)*.22+noise*Math.Exp(-t*45)*.35,
                "ice"=>(tone(3201)+tone(4637)*.6+tone(5723)*.3)*Math.Exp(-t*14)*.20+noise*Math.Exp(-t*33)*.2,
                "heal" or "chime"=>(tone(659)+tone(988)*.6+tone(1318)*.3)*Math.Exp(-t*5)*.24,
                "void"=>Math.Sin(Math.Tau*(110*t-70*t*t))*Math.Exp(-t*7)*.45+low*Math.Exp(-t*8)*.5,
                "charge" or "beam"=>Math.Sin(Math.Tau*(240*t+900*t*t))*Math.Exp(-t*6)*.36+noise*Math.Exp(-t*14)*.18,
                "fire" or "sizzle"=>low*Math.Exp(-t*9)*2+noise*Math.Exp(-t*18)*.18,
                "servo"=>tone(189)*(0.5+0.5*tone(32))*Math.Exp(-t*10)*.4+noise*Math.Exp(-t*25)*.15,
                "growl"=>tone(78)*(0.6+0.4*tone(29))*Math.Exp(-t*8)*.5+low*Math.Exp(-t*8)*1.1,
                "claw"=>noise*Math.Exp(-t*26)*.35+tone(168)*Math.Exp(-t*24)*.32,
                "metal"=>tone(180)*Math.Exp(-t*45)*.4+tone(1271)*Math.Exp(-t*60)*.2+noise*Math.Exp(-t*80)*.2,
                "leather" or "clawstep"=>tone(105)*Math.Exp(-t*48)*.45+low*Math.Exp(-t*40)*1.4,
                "soft"=>low*Math.Exp(-t*55)*1.2,
                _=>noise*Math.Sin(Math.PI*Math.Clamp(t/.3,0,1))*Math.Exp(-t*9)*.5
            };
            // Broad, smooth material sounds replace the sharp broadband transients and laser chirps.
            double air=low-soft;soft=.97*soft+.03*low;
            double swell=Math.Sin(Math.PI*Math.Clamp(t/duration,0,1));
            if(cue is "charge" or "beam" or "servo")v=(air*2.4+tone(155)*.07)*swell*Math.Exp(-t*3);
            else if(cue=="growl")v=(soft*4+tone(83)*.055+tone(121)*.025)*swell;
            else if(cue=="claw")v=(air*2.5+soft*2)*swell*Math.Exp(-t*4);
            else if(cue is "heavy" or "kick")v=(soft*2.8+tone(cue=="heavy"?68:105)*.14)*Math.Exp(-t*10);
            else if(cue=="steel")v=(air*1.8+tone(1261)*.035+tone(1937)*.025)*Math.Exp(-t*9);
            else if(cue=="ice")v=(air*.8+tone(2201)*.025+tone(2893)*.018)*swell*Math.Exp(-t*5);
            else if(cue is "swing" or "fire" or "sizzle")v=air*2.1*swell*Math.Exp(-t*3);
            if(cue.StartsWith("akuroth-")){
                double baseHz=cue switch{"akuroth-cut"=>185,"akuroth-waves"=>75,"akuroth-prison"=>330,"akuroth-meteor"=>54,"akuroth-rift"=>121,"akuroth-chaos"=>220,"akuroth-collapse"=>63,"akuroth-rain"=>147,"akuroth-rewrite"=>440,"akuroth-transform"=>98,"akuroth-annihilation"=>41,_=>85};
                double pulse=cue=="akuroth-chaos"?.6+.4*tone(13):1;
                v=(tone(baseHz)*.22+tone(baseHz*1.498)*.12+soft*3+air*.7)*swell*pulse;
                if(cue is "akuroth-prison" or "akuroth-rewrite")v+=tone(baseHz*4)*.07*Math.Exp(-t*7);
                if(cue=="akuroth-impact")v=(tone(57)*.28+soft*3)*Math.Exp(-t*8);
            }
            v*=Math.Min(1,t/.018)*Math.Min(1,(duration-t)/.05);short sample=(short)(Math.Tanh(v)*22000);bytes[n*2]=(byte)sample;bytes[n*2+1]=(byte)(sample>>8);
        }
        return new AudioStreamWav{Format=AudioStreamWav.FormatEnum.Format16Bits,MixRate=rate,Data=bytes};
    }
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
