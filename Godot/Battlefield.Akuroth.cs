using Godot;
using Vaelorn;
using System;
using System.Linq;
using System.Collections.Generic;

public partial class Battlefield
{
    private Texture2D akurothArt=null!;
    private Rect2[] akurothRegions=Array.Empty<Rect2>();
    private readonly Dictionary<Unit,double> akurothTransformations=new();
    private void LoadAkurothArt(){
        akurothArt=GD.Load<Texture2D>("res://Art/akuroth-phases-v1.png");
        var boxes=System.Text.Json.JsonSerializer.Deserialize<int[][]>(Godot.FileAccess.GetFileAsString("res://Data/akuroth-frames.json"))!;
        if(boxes.Length!=2)throw new InvalidOperationException("AKUROTH requires two phase sprites");
        akurothRegions=boxes.Select(b=>new Rect2(b[0],b[1],b[2],b[3])).ToArray();
    }
    private Rect2 AkurothRegion(Unit u)=>akurothRegions[u.Ascended?1:0];
    private void ScheduleAkurothTransformation(){
        foreach(var u in Battle!.Allies.Concat(Battle.Enemies).Where(u=>u.Id=="akuroth"&&u.Ascended&&u.Alive))
            if(!akurothTransformations.ContainsKey(u)){
                double at=clock+Battle.LastImpacts.Where(h=>h.Target==u).Select(ImpactDelay).DefaultIfEmpty(0).Max();
                akurothTransformations[u]=at;soundEvents.Add((at,"akuroth-transform",1,-12));
            }
    }
    private void EclipseOrb(Vector2 at,float radius,Color color,float alpha=1){
        Glow(at,color,radius*1.8f,alpha);
        DrawCircle(at,radius,new Color(.025f,.007f,.04f,alpha));
        DrawArc(at,radius,0,Mathf.Tau,40,new Color(color,alpha),2,true);
        for(int n=0;n<6;n++){
            float a=n*Mathf.Tau/6+(float)clock;
            var ray=new Vector2(Mathf.Cos(a),Mathf.Sin(a));
            DrawLine(at+ray*radius,at+ray*radius*1.3f,new Color(color,alpha*.7f),1.5f,true);
        }
    }
    private void DrawAkuroth(Unit u,Vector2 at,float h,bool friendly,bool alive,bool active,float age,Color tint,float lean,Vector2 stretch){
        if(alive&&akurothTransformations.TryGetValue(u,out var transformAt)&&clock>=transformAt&&clock-transformAt<AkurothTransformDuration){DrawAkurothTransformation(u,at,h,friendly,(float)(clock-transformAt));return;}
        var box=AkurothRegion(u);bool phased=u.Ascended;
        if(phased&&akurothTransformations.TryGetValue(u,out var changed)&&clock<changed){box=akurothRegions[0];phased=false;}
        float phase=(float)clock*2.4f;
        float attack=active?Mathf.Sin(Mathf.Clamp(age/.95f,0,1)*Mathf.Pi):0;
        bool controlled=ShownFreeze(u)>0||ShownStun(u)>0;
        if(!controlled&&alive){at.Y-=phased?5+Mathf.Sin(phase)*3:0;lean+=(friendly?1:-1)*attack*.09f;}
        if(active&&alive&&!controlled){DrawAkurothAction(u,at,h,friendly,age,tint,lean,stretch);}
        else {
        DrawSetTransform(at,lean,new Vector2((friendly?-1:1)*stretch.X,stretch.Y));
        float w=h*box.Size.X/box.Size.Y;
        if(alive&&!controlled&&!active){
            // Foot-anchored strip rig: continuous hair, cloak and shoulder movement.
            for(int n=0;n<24;n++){
                float a=n/24f,b=(n+1)/24f;
                float sway=Mathf.Sin(phase+a*4)*UpperBodyWeight(a)*(phased?3.2f:1.7f);
                DrawTextureRectRegion(akurothArt,new Rect2(-w/2+sway,-h+h*a,w,h*(b-a)+.3f),new Rect2(box.Position+new Vector2(0,box.Size.Y*a),new Vector2(box.Size.X,box.Size.Y/24)),tint);
            }
        }else DrawTextureRectRegion(akurothArt,new Rect2(-w/2,-h,w,h),box,tint);
        DrawSetTransform(Vector2.Zero);
        }
        if(alive&&akurothTransformations.TryGetValue(u,out changed)){
            float t=(float)(clock-changed);
            if(t>=0&&t<1.8f){
                var color=PowerColor(u);float fade=1-t/1.8f;
                DrawArc(at-new Vector2(0,h*.55f),25+t*90,0,Mathf.Tau,64,new Color(color,fade),3,true);
                Sparks(at-new Vector2(0,h*.55f),color,t/1.8f,24,180);
                Text("SOBERANO DO ABSOLUTO",at-new Vector2(125,h+28),new Color("f2d2ff"),16);
            }
        }
    }
    private void DrawAkurothHud(){
        var u=Battle?.Enemies.FirstOrDefault(u=>u.Id=="akuroth");if(u==null||EntranceActive)return;
        float width=Mathf.Min(520,Size.X*.46f);var origin=new Vector2((Size.X-width)/2,54);
        DrawRect(new Rect2(origin-new Vector2(10,0),new Vector2(width+20,64)),new Color(.035f,.006f,.025f,.92f));
        Text("AKUROTH — O Fim das Realidades",origin+new Vector2(4,17),new Color("ff9eb0"),16);
        Text(u.AkurothTitle,origin+new Vector2(4,34),new Color("eadcf2"),13);
        float ratio=(float)(DisplayHp(u)/u.MaxHp);
        DrawRect(new Rect2(origin+new Vector2(0,42),new Vector2(width,8)),new Color("301222"));
        DrawRect(new Rect2(origin+new Vector2(0,42),new Vector2(width*ratio,8)),PowerColor(u));
        DrawLine(origin+new Vector2(width*.3f,39),origin+new Vector2(width*.3f,53),Colors.White,2);
        Text($"{DisplayHp(u):F0} / {u.MaxHp:F0}  •  {ratio:P0}",origin+new Vector2(4,63),Colors.White,11);
        if(u.AnnihilationCharging&&u.Alive)Text("ANIQUILAÇÃO NA PRÓXIMA AÇÃO — PROTEJA A EQUIPE!",origin+new Vector2(0,85),new Color("ffd1e0"),16);
    }
    private void DrawAkurothPower(float age)=>DrawAkurothCinematicPower(age);
}
