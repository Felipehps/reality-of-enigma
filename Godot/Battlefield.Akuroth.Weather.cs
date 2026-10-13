using Godot;
using System;
using System.Linq;

public partial class Battlefield
{
    private void DrawAkurothWeather()
    {
        if(Battle?.Enemies.Any(u=>u.Id=="akuroth")!=true)return;
        float inferno=AkurothInfernoMix(),time=(float)clock;
        // Flame tongues deform continuously; anchored vents stay on the arena floor.
        if(inferno>0)for(int vent=0;vent<10;vent++){
            var root=AkurothCameraPoint(new Vector2(Size.X*(.035f+vent*.103f),Size.Y*(vent%2==0?.78f:.58f)));
            float heat=1;float age=(float)(clock-actionAt);
            if(actor?.Id=="akuroth"&&age>=0&&age<EffectDuration)foreach(var hit in Battle.LastImpacts){float local=age-(float)ImpactDelay(hit);if(local>=0)heat=Mathf.Max(heat,1+Mathf.Exp(-local*3)*.35f);}
            float height=(85+vent%3*30)*inferno*heat*AkurothCameraZoom;
            DrawEllipse(root,new Vector2(26,5),new Color(.95f,.12f,.025f,.18f*inferno));
            DrawLivingFlame(root,height,vent,time,inferno);
        }
        for(int n=0;n<85;n++){
            float t=(time*(.035f+n%5*.009f)+n*.618034f)%1;
            var p=AkurothCameraPoint(new Vector2((n*193+Mathf.Sin(time+n)*22)%Size.X,Size.Y*(1-t)));
            float alpha=Mathf.Sin(t*Mathf.Pi)*(.12f+.45f*inferno);
            DrawLine(p,p+new Vector2(Mathf.Sin(time+n)*2,-3-n%4),new Color(1,.15f+.4f*(n%3/3f),.07f,alpha),.8f+n%3*.4f,true);
        }
        // Seeded branching lightning flashes in the distant sky, never over the UI.
        float cycle=time%7.7f;float flash=cycle<.22f?Mathf.Sin(cycle/.22f*Mathf.Pi):cycle>.34f&&cycle<.43f?.45f*Mathf.Sin((cycle-.34f)/.09f*Mathf.Pi):0;
        if(flash<=0)return;int epoch=(int)(time/7.7f);var random=new Random(193+epoch);
        float start=Size.X*(.16f+(float)random.NextDouble()*.65f);var previous=new Vector2(start,48);
        for(int n=1;n<=9;n++){
            var next=new Vector2(start-35+(float)random.NextDouble()*70,48+Size.Y*.027f*n);
            DrawLine(previous,next,new Color(.8f,.04f,.20f,flash*.2f),9,true);
            DrawLine(previous,next,new Color(1,.38f,.48f,flash*(.4f+.6f*inferno)),2,true);
            if(n%3==0)DrawLine(next,next+new Vector2(40+random.Next(30),30),new Color(1,.25f,.40f,flash*.5f),1,true);
            previous=next;
        }
        DrawRect(new Rect2(0,120,Size.X,Size.Y*.23f),new Color(.55f,.05f,.16f,flash*.055f));
    }
    private void DrawLivingFlame(Vector2 root,float height,int vent,float time,float opacity){
        float width=height*.67f;var tint=new Color(1,1,1,opacity*(.8f+.12f*Mathf.Sin(time*7+vent)));
        Vector2 Vertex(Vector2 uv){float bend=Mathf.Pow(1-uv.Y,1.4f);return root+new Vector2((uv.X-.5f)*width+Mathf.Sin(time*4.7f+uv.Y*11+vent)*width*.10f*bend,(uv.Y-.91f)*height*(1+Mathf.Sin(time*3.1f+vent+uv.X*2)*.045f*bend));}
        for(int y=0;y<12;y++)for(int x=0;x<4;x++){
            var a=new Vector2(x/4f,y/12f);var b=new Vector2((x+1)/4f,y/12f);var c=new Vector2(x/4f,(y+1)/12f);var d=new Vector2((x+1)/4f,(y+1)/12f);
            DrawPrimitive(new[]{Vertex(a),Vertex(b),Vertex(c)},new[]{tint,tint,tint},new[]{a,b,c},akurothFire);
            DrawPrimitive(new[]{Vertex(b),Vertex(d),Vertex(c)},new[]{tint,tint,tint},new[]{b,d,c},akurothFire);
        }
    }
}
