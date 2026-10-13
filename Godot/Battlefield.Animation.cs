using Godot;
using Vaelorn;
using System;
using System.Linq;

public partial class Battlefield
{
    private Color PowerColor(Unit u)=>new(u.Id switch{
        "lyra"=>"80eaff","aelia"=>"bda2ff","savor"=>"ff9a42","brakk"=>"ffc971",
        "varkhan"=>"bc75ff","solarius"=>"ffe18a","raizen"=>"ad70ff","akuroth"=>u.Ascended?"e495ff":"ff365a",
        "kael" when u.Path=="martial"=>u.Gate==8?"ff344e":u.Gate>=6?"42baff":"69f5b3",_=>"70e5d3"});

    // Articulated pixel silhouettes: each limb has its own combat pose.
    private void DrawFighter(Unit u,Rect2 r,bool friendly,Color tint)
    {
        float age=(float)(clock-actionAt),attack=u==actor&&Battle?.LastSkill!=null?Mathf.Sin(Mathf.Clamp(age/.7f,0,1)*Mathf.Pi):0;
        float breathe=u.Alive&&u.Freeze==0&&u.Stun==0?(float)Math.Sin(clock*3+u.Column)*1.5f:0;
        var c=r.GetCenter()+new Vector2(0,10+breathe);float dir=friendly?1:-1;
        var accent=PowerColor(u);var cloth=new Color(u.Id switch{"lyra"=>"377aaf","aelia"=>"7755a2","savor"=>"e6d7bf","brakk"=>"667986","solarius"=>"bb863b","varkhan"=>"655179",_=>"284853"})*tint;
        var skin=new Color(u.Id is "brakk" or "varkhan"?"a6a3b9":"e2ac88")*tint;
        DrawEllipse(c+new Vector2(0,37),new Vector2(28,5),new Color(0,0,0,.3f));
        if(!u.Alive){DrawSetTransform(c,Mathf.Pi/2,new Vector2(.8f,.8f));c=Vector2.Zero;}
        if(u.Gate>0&&u.Alive)for(int n=0;n<9;n++){
            float a=n*Mathf.Tau/9+(float)clock;var p=c+new Vector2(Mathf.Cos(a)*29,Mathf.Sin(a)*38);
            DrawRect(new Rect2(p,new Vector2(3,7)),new Color(accent, .3f+.3f*Mathf.Sin(a)*Mathf.Sin(a)));
        }
        float bulk=u.Id=="brakk"?20:u.Gate==8?19:12;
        void Limb(Vector2 a,Vector2 b,Color color,float width){DrawLine(c+a,c+b,new Color("101724"),width+4);DrawLine(c+a,c+b,color,width);}
        if(u.Id is "lyra" or "aelia" or "varkhan" or "solarius"){
            float flutter=(float)Math.Sin(clock*4)*4;
            DrawColoredPolygon(new[]{c+new Vector2(-dir*10,-17),c+new Vector2(-dir*(25+flutter),29),c+new Vector2(dir*6,27)},cloth.Darkened(.25f));
        }
        Limb(new(-7,12),new(-11-attack*7,34),cloth,9);
        Limb(new(7,12),u.Id=="savor"?new Vector2(12+dir*attack*35,34-attack*45):new Vector2(12+attack*10,34),cloth,9);
        DrawRect(new Rect2(c+new Vector2(-bulk,-16),new Vector2(bulk*2,35)),cloth);
        DrawRect(new Rect2(c+new Vector2(-bulk,9),new Vector2(bulk*2,5)),accent*tint);
        var rear=new Vector2(-dir*(bulk+5),7-attack*12);var hand=new Vector2(dir*(bulk+8+attack*25),3-attack*19);
        Limb(new(-dir*bulk,-10),rear,cloth,8);Limb(new(dir*bulk,-10),hand,skin,8);
        DrawRect(new Rect2(c+new Vector2(-11,-39),new Vector2(22,23)),skin);
        var hair=new Color(u.Id=="lyra"?"d3f4ff":u.Id=="aelia"?"ded0ef":"1a2334");
        DrawRect(new Rect2(c+new Vector2(-12,-42),new Vector2(24,u.Id=="kael"?11:8)),hair*tint);
        if(u.Id is "aelia" or "lyra")DrawRect(new Rect2(c+new Vector2(-dir*14,-37),new Vector2(7,33)),hair*tint);
        DrawRect(new Rect2(c+new Vector2(dir*5-2,-29),new Vector2(4,3)),u.Id=="brakk"?accent:new Color("14243a"));
        if(u.Id.StartsWith("enemy")){
            DrawColoredPolygon(new[]{c+new Vector2(-14,-31),c+new Vector2(-19,-49),c+new Vector2(-5,-38)},cloth);
            DrawColoredPolygon(new[]{c+new Vector2(14,-31),c+new Vector2(19,-49),c+new Vector2(5,-38)},cloth);
            DrawRect(new Rect2(c+new Vector2(-9,-31),new Vector2(18,4)),new Color("ee9274"));
        }
        if(u.Id=="solarius")for(int n=0;n<5;n++)DrawRect(new Rect2(c+new Vector2(-13+n*6,-50+(n%2)*4),new Vector2(4,14)),accent);
        if(u.Id=="kael"&&u.Path!="martial"){
            var tip=hand+new Vector2(dir*(10+attack*20),-32+attack*20);Limb(hand,tip,new Color("d1f5ee"),4);
            Limb(rear,rear+new Vector2(-dir*17,-22),new Color("b9d6e9"),3);
        }
        if(u.Id is "aelia" or "lyra"){
            var p=c+hand+new Vector2(dir*4,-12);DrawCircle(p,5+attack*6,new Color(accent,.65f));Star(p,9+attack*9,accent);
        }
        if(u.Id=="brakk")DrawRect(new Rect2(c+hand-new Vector2(8,8),new Vector2(21,17)),accent*tint);
        if(!u.Alive)DrawSetTransform(Vector2.Zero);
    }
    private void DrawEllipse(Vector2 c,Vector2 size,Color color)
    {if(size.X<.1f||size.Y<.1f||color.A<=0)return;DrawColoredPolygon(Enumerable.Range(0,24).Select(n=>c+new Vector2(Mathf.Cos(n*Mathf.Tau/24)*size.X,Mathf.Sin(n*Mathf.Tau/24)*size.Y)).ToArray(),color);}

}
