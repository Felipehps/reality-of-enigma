using Godot;
using Vaelorn;
using System;
using System.Linq;

public partial class Battlefield
{
    private void DrawCombatant(Unit u,Rect2 r,bool friendly)
    {
        var foot=GroundPosition(u);
        var accent=PowerColor(u);
        // Contact shadows stay on the ground, below the sprite and selection outline.
        for(int n=4;n>=1;n--)DrawEllipse(foot+new Vector2(0,2),new Vector2(34+n*6,3+n*2),new Color(.015f,.02f,.03f,.10f));
        DrawEllipse(foot,new Vector2(34,5),new Color(0,0,0,.58f));
        DrawEllipse(foot+new Vector2(-17,0),new Vector2(12,3),new Color(0,0,0,.55f));
        DrawEllipse(foot+new Vector2(17,0),new Vector2(12,3),new Color(0,0,0,.55f));
        if(Battle?.Finished==false&&(IsHighlightedTarget(u)||u==DisplayActor)){
            var color=IsHighlightedTarget(u)?TargetIntent is "heal" or "support" or "self"?new Color("6fd7ab"):new Color("e292a2"):gold;

            DrawPolyline(Enumerable.Range(0,33).Select(n=>foot+new Vector2(Mathf.Cos(n*Mathf.Tau/32)*53,Mathf.Sin(n*Mathf.Tau/32)*13)).ToArray(),color,2);
        }
        float age=(float)(clock-actionAt);bool active=(u==actor||u.Origin==actor)&&Battle?.LastSkill!=null&&age<EffectDuration;
        bool visibleAlive=VisualHp(u)>.01;
        float direction=friendly?1:-1;
        bool canMove=visibleAlive&&ShownFreeze(u)==0&&ShownStun(u)==0;
        float breath=canMove?(float)Math.Sin(clock*2.7+u.Layer+u.Column):0;
        float bob=0; // Breathing belongs to the upper body; the foot anchor never bobs.
        var tint=visibleAlive?(u.Origin!=null?new Color(.50f,.92f,1,.84f):Colors.White):new Color(.68f,.70f,.76f,1);
        var lastHit=damageNumbers.LastOrDefault(n=>n.Unit==u&&n.Kind!="heal"&&clock>=n.At);
        float hitAge=lastHit==null?10:(float)(clock-lastHit.At);
        float recoil=hitAge<.65f?Mathf.Sin(Mathf.Min(hitAge/.14f,1)*Mathf.Pi/2)*Mathf.Exp(-hitAge*5):0;
        if(u.Boss)recoil*=.55f;
        bool essence=lastHit?.Kind=="essence";
        if(hitAge<.10f)tint=essence?new Color(1,.72f,1):new Color(1,.87f,.65f);
        var at=foot+new Vector2(-direction*recoil*(essence?13:35),bob-recoil*(essence?12:5));
        float lean=-direction*recoil*.23f;
        var stretch=new Vector2(1+recoil*.11f,1-recoil*.08f);
        if(active&&canMove){
            if(IsRunning(u,age)){at.Y-=Mathf.Abs(Mathf.Sin(age*32))*4;lean+=direction*.04f;}
            else if(age>=HitMoment&&age<1.08f){float snap=Mathf.Sin((age-HitMoment)/.43f*Mathf.Pi);at.X+=direction*snap*7;lean+=direction*snap*.035f;}
        }
        if(visibleAlive&&ShownFreeze(u)>0){at=foot;lean=0;stretch=Vector2.One;tint=new Color(.34f,.70f,1);active=false;}
        else if(visibleAlive&&ShownStun(u)>0){
            float dizzy=(float)Math.Sin(clock*3.4+u.Column);
            at=foot;lean=0;stretch=Vector2.One;active=false;
            tint=new Color(.88f,.84f,.72f);
        }
        if(active&&canMove&&Battle?.LastSkill is {Cooldown: >0} special&&age<HitMoment){
            float charge=Mathf.Sin(Mathf.Clamp(age/HitMoment,0,1)*Mathf.Pi);
            stretch=new Vector2(1+charge*.035f,1-charge*.06f);
            lean+=direction*charge*(special.Scale=="essence"?-.04f:.08f);
        }
        if(visibleAlive&&ActiveBuffs(u).Count>0){
            float pulse=.5f+.5f*Mathf.Sin((float)clock*2+u.Column);
            Glow(foot-new Vector2(0,r.Size.Y*.42f),blue,50,.22f+pulse*.12f);
            for(int n=0;n<8;n++){
                float t=((float)clock*.23f+n*.127f)%1;
                var mote=foot+new Vector2(Mathf.Sin(n*4.2f)*30,-t*(r.Size.Y-35));
                DrawCircle(mote,1.2f,new Color(.45f,.8f,1,Mathf.Sin(t*Mathf.Pi)*.45f));
            }
        }
        if(!visibleAlive){
            float fall=Mathf.SmoothStep(0,1,Mathf.Clamp((float)(clock-defeatedAt.GetValueOrDefault(u,clock-1))/.65f,0,1));
            lean=-direction*fall*Mathf.Pi/2;stretch=Vector2.One*(1-fall*.18f);
            at.Y+=fall*8;
            at.X+=direction*fall*(r.Size.Y-32)*.41f;
        }
        Texture2D texture=combatants;Rect2 region;
        bool art=true;
        if(u.Id=="kael"&&u.Path!="martial"){
            texture=poses;
            int frame=!active?0:age<.46f?1:age<.98f?2:age<1.35f?3:0;
            // Atlas regions follow the authored poses, which have different sword reach.
            region=frame switch{0=>new Rect2(0,0,540,724),1=>new Rect2(540,0,510,724),2=>new Rect2(1050,0,640,724),_=>new Rect2(1690,0,482,724)};
        }else region=u.Id switch{
            "savor"=>new Rect2(0,0,550,512),"aelia"=>new Rect2(550,0,475,512),
            "lyra"=>new Rect2(1025,0,511,512),"brakk"=>new Rect2(0,512,580,512),
            "varkhan"=>new Rect2(580,512,440,512),_=>new Rect2(1020,512,516,512)};
        if(u.Id is "raizen" or "solarius"||(u.Id=="kael"&&u.Path=="martial"))art=false;
        float h=(r.Size.Y-32)*(1-u.Layer*.045f)*(u.Id=="akuroth"?2.4f:u.Id=="nerathis"?2.1f:u.Id=="maltherion"?1.65f:1)*AkurothCameraZoom,w=h*region.Size.X/region.Size.Y;
        var sprite=new Rect2(at-new Vector2(w/2,h),new Vector2(w,h));
        if(EntranceActive&&clock-entranceAt>=1.5){
            float retreat=Mathf.Clamp((float)(clock-entranceAt-1.5)/1.3f,0,1);
            at.Y-=Mathf.Sin(Mathf.Clamp((retreat-.1f)/.63f,0,1)*Mathf.Pi)*62;
            lean=0;stretch=Vector2.One;
        }
        var frameAt=visibleAlive?at:foot;
        if(visibleAlive&&u.Id=="varkas"&&u.Origin!=null){
            DrawEllipse(foot,new Vector2(35,7),new Color(.22f,.8f,1,.20f));
            for(int n=0;n<7;n++){
                float phase=((float)clock*.35f+n/7f)%1;
                var spark=frameAt+new Vector2(Mathf.Sin(n*2.7f+phase)*30,-phase*h);
                DrawCircle(spark,1.5f,new Color(.4f,.92f,1,Mathf.Sin(phase*Mathf.Pi)*.55f));
            }
        }
        bool shattered=!visibleAlive&&frozenDefeats.Contains(u);
        bool showBody=!shattered||(u.SurvivesDefeat||friendly)&&clock-defeatedAt[u]>1.1;
        if(u.Id=="akuroth")DrawAkuroth(u,at,h,friendly,visibleAlive,active,age,tint,lean,stretch);
        else if(DrawSpiral(u,frameAt,h,age)){}
        else if(!showBody){}
        else if(DrawFrame(u,frameAt,h,friendly,visibleAlive,active,age,hitAge,tint,visibleAlive?lean:0,visibleAlive?stretch:Vector2.One)){}
        else if(art){
            bool flip=u.Id=="varkhan"?friendly:!friendly&&!u.Id.StartsWith("enemy");
            DrawSetTransform(at,lean,new Vector2((flip?-1:1)*stretch.X,stretch.Y));
            DrawTextureRectRegion(texture,new Rect2(sprite.Position-at,sprite.Size),region,tint);
            DrawSetTransform(Vector2.Zero);
        }else{
            DrawFighter(u,new Rect2(at-new Vector2(65,120),new Vector2(130,100)),friendly,tint);
        }
        if(visibleAlive&&ShownFreeze(u)>0)DrawFrozenShell(foot,h);
        if(shattered)DrawIceShatter(foot,h,(float)(clock-defeatedAt[u]));
        if(visibleAlive&&ShownStun(u)>0)for(int n=0;n<3;n++){
            float a=(float)clock*2+n*Mathf.Tau/3;
            Star(at+new Vector2(Mathf.Cos(a)*27,-h+Mathf.Sin(a)*7),6,gold);
        }
        if(visibleAlive&&ShownIce(u)>0&&ShownFreeze(u)==0)for(int n=0;n<ShownIce(u);n++)Snow(at+new Vector2(-25+n*12,-h+8),4);
        if(visibleAlive&&u.Gate>0)DrawArc(at-new Vector2(0,h*.45f),h*.5f,-Mathf.Pi,0,36,new Color(accent,.75f),3);
        DrawEvolutionAura(u,foot);
        if(EntranceActive||ExitActive||!visibleAlive||IsSpiral(u))return;
        var nameAt=foot+new Vector2(-65,-h-14);
        if(u.Stacks("poison")>0)Text("VENENO ×"+u.Stacks("poison"),foot+new Vector2(-43,-h-34),new Color("c8e66d"),13);
        if(u.Statuses.Any(s=>s.Effect.Sleep)){Text("Z z z",foot+new Vector2(-15,-h-52),new Color("e5b5ff"),18);}
        if(u.Id=="maltherion")Text((u.Ascended?"ASCENSÃO • ":"CARNIFICINA • ")+u.Carnage,foot+new Vector2(-75,-h-35),new Color("e77cab"),14);
        if(u.Id=="nerathis")Text(u.Blooming?"FLORESCIMENTO":u.Crimson?"FLOR CARMESIM":"FLOR DO VENENO",foot+new Vector2(-75,-h-35),new Color("e77cab"),14);
        if(u.BleedCount>0){
            var pos=foot+new Vector2(-18,-h-49);
            DrawStyleBox(new StyleBoxFlat{BgColor=new Color("321322"),CornerRadiusTopLeft=9,CornerRadiusTopRight=9,CornerRadiusBottomLeft=9,CornerRadiusBottomRight=9},new Rect2(pos-new Vector2(8,14),new Vector2(58,26)));
            DrawCircle(pos+new Vector2(5,2),5,new Color("e96785"));
            DrawColoredPolygon(new[]{pos+new Vector2(0,0),pos+new Vector2(5,-10),pos+new Vector2(10,0)},new Color("e96785"));
            Text("×"+u.BleedCount,pos+new Vector2(16,7),new Color("ffc4d0"),16);
        }
        if((u.Boss||Selected==u)&&!damageNumbers.Any(n=>n.Unit==u&&clock>=n.At&&clock-n.At<1.3)){
            DrawStringOutline(Font,nameAt,u.CombatName,HorizontalAlignment.Left,-1,13,4,new Color(.02f,.04f,.07f,.9f));
            if(IsHighlightedTarget(u)){
                var width=Font.GetStringSize(u.CombatName,fontSize:13).X;
                DrawRect(new Rect2(nameAt-new Vector2(5,15),new Vector2(width+10,20)),new Color(.07f,.02f,.12f,.9f));
            }
            Text(u.CombatName,nameAt,IsHighlightedTarget(u)?TargetIntent is "heal" or "support" or "self"?new Color("7de1b8"):new Color("d6a0f0"):gold,13);
        }
        // A single compact plate keeps level, HP and effects separated and inside the arena.
        var plate=new Vector2(Mathf.Clamp(foot.X-65,78,Size.X-145),Mathf.Clamp(foot.Y+8,48,Size.Y-76));
        var outline=new Color("685479");
        var points=new[]{plate,plate+new Vector2(116,0),plate+new Vector2(124,10),plate+new Vector2(116,21),plate+new Vector2(0,21),plate};
        DrawColoredPolygon(points.Take(5).ToArray(),new Color(.035f,.023f,.06f,.82f));DrawPolyline(points,outline,1,true);
        Text($"{u.Level}",plate+new Vector2(7,16),new Color("ddceb0"),14);
        DrawLine(plate+new Vector2(32,5),plate+new Vector2(32,16),outline,1);
        float hp=Mathf.Clamp((float)(VisualHp(u)/u.MaxHp),0,1);
        DrawRect(new Rect2(plate+new Vector2(40,8),new Vector2(75,5)),new Color("261b30"));
        DrawRect(new Rect2(plate+new Vector2(40,8),new Vector2(75*hp,5)),friendly?new Color("81b8aa"):new Color("be7d8c"));
        DrawBuffIcons(u,plate+new Vector2(39,24));
        string label=u.Stacks("exposed")>0?"EXPOSTO":!visibleAlive?(friendly?"CAÍDO":u.SurvivesDefeat?"DERROTADO":"MORTO"):ShownFreeze(u)>0?"GELO":ShownStun(u)>0?"STUN":ShownIce(u)>0?$"{ShownIce(u)}/5":u.Gate>0?$"VIII · {u.Gate}":"";
        if(label!="")Text(label,plate+new Vector2(0,37),ShownStun(u)>0?gold:blue,10);
    }
}
