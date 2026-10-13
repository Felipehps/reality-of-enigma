using Godot;
using Vaelorn;
using System;
using System.Linq;

public partial class Battlefield
{
    private record DamageNumber(Unit Unit,double Amount,string Kind,bool Critical,int Index,double At);
    private double VisualHp(Unit unit)
    {
        double hp=unit.Hp;
        foreach(var hit in damageNumbers.Where(n=>n.Unit==unit&&clock<n.At))hp+=hit.Kind=="heal"?-hit.Amount:hit.Amount;
        return Math.Clamp(hp,0,unit.MaxHp);
    }
    private readonly SystemFont damageFont=new(){FontNames=new[]{"Segoe UI","Arial"},FontWeight=800,FontItalic=false};
    public override void _ExitTree(){damageFont.Dispose();}
    private void DrawDamageNumbers()
    {
        foreach(var hit in damageNumbers){
            float age=(float)(clock-hit.At);
            if(age<0||age>1.3f||!cards.TryGetValue(hit.Unit,out var r))continue;
            bool heal=hit.Kind=="heal";
            float alpha=Mathf.Clamp((1.3f-age)/.32f,0,1);
            var color=new Color(heal?"7dffad":hit.Kind=="essence"?"d397ff":"ff7474"){A=alpha};
            float rise=68*(1-Mathf.Exp(-age*3));
            float pop=1+.24f*Mathf.Exp(-age*16);
            int lane=hit.Index%3;
            var foot=GroundPosition(hit.Unit);
            var at=foot+new Vector2((lane-1)*31+Mathf.Sin(age*2.2f)*(lane-1)*18,-92-rise-lane*12);
            string value=(heal?"+":"")+hit.Amount.ToString("F0");
            DrawSetTransform(at,0,Vector2.One*pop);
            int size=hit.Critical?39:30;
            float width=damageFont.GetStringSize(value,fontSize:size).X;
            float x=-width/2;
            for(int digit=0;digit<value.Length;digit++){
                string glyph=value[digit].ToString();
                float bounce=-Mathf.Sin(Mathf.Clamp((age-digit*.015f)*10,0,Mathf.Pi))*7;
                var position=new Vector2(x,bounce);
                DrawStringOutline(damageFont,position+new Vector2(1,2),glyph,HorizontalAlignment.Left,-1,size,4,new Color(.035f,.02f,.08f,alpha));
                DrawString(damageFont,position,glyph,HorizontalAlignment.Left,-1,size,color);
                x+=damageFont.GetStringSize(glyph,fontSize:size).X;
            }
            if(hit.Critical){
                DrawStringOutline(damageFont,new Vector2(width/2+3,-8),"!",fontSize:25,size:3,modulate:new Color(.03f,.01f,.06f,alpha));
                DrawString(damageFont,new Vector2(width/2+3,-8),"!",fontSize:25,modulate:new Color(1,.85f,.44f,alpha));
            }
            DrawSetTransform(Vector2.Zero);
        }
    }
}
