using Godot;
using Vaelorn;
using System;
using System.Linq;
using System.Collections.Generic;
using System.Text.Json;

public partial class Battlefield
{
    private sealed record AkurothActionAtlas(string Texture,int[][] Frames,float ReferenceHeight);
    private readonly Dictionary<int,(Texture2D Texture,AkurothActionAtlas Data)> akurothActionAtlases=new();
    private readonly Dictionary<string,Texture2D> akurothFx=new();
    private Texture2D akurothHell=null!;
    private Texture2D akurothFire=null!;
    public bool AkurothCinematicAssetsReady=>akurothHell!=null&&akurothFire!=null&&akurothFx.Count==10&&akurothActionAtlases.Count==2;
    public bool AkurothInfernoActive=>AkurothInfernoMix()>.99f;
    private static readonly string[] AkurothEffectIds={"cut","waves","prison","meteor","rift","chaos","collapse","rain","rewrite","annihilation"};
    private void LoadAkurothCinema(){
        var atlases=JsonSerializer.Deserialize<Dictionary<int,AkurothActionAtlas>>(Godot.FileAccess.GetFileAsString("res://Data/akuroth-actions.json"))!;
        foreach(var item in atlases){
            if(item.Value.Frames.Length!=8||item.Value.ReferenceHeight<=0)throw new InvalidOperationException("Invalid AKUROTH action atlas");
            akurothActionAtlases[item.Key]=(GD.Load<Texture2D>(item.Value.Texture),item.Value);
        }
        foreach(var id in AkurothEffectIds){var texture=GD.Load<Texture2D>("res://Art/akuroth-fx-"+id+"-v2.png");if(texture==null)throw new InvalidOperationException("Missing AKUROTH FX: "+id);akurothFx[id]=texture;}
        akurothHell=GD.Load<Texture2D>("res://Art/akuroth-inferno-v2.png");
        akurothFire=GD.Load<Texture2D>("res://Art/akuroth-fire-v1.png");
    }
    public static float AkurothDuration(string id)=>id switch{
        "akuroth_cut"=>2.5f,"akuroth_waves"=>3.0f,"akuroth_prison"=>3.2f,"akuroth_meteor"=>3.5f,"akuroth_rift"=>3.2f,
        "akuroth_chaos"=>3.5f,"akuroth_collapse"=>3.8f,"akuroth_rain"=>4.0f,"akuroth_rewrite"=>3.5f,"akuroth_annihilation"=>6.0f,_=>1.85f};
    public static double AkurothImpact(string id,int index)=>id switch{
        "akuroth_cut"=>.65+index*.13,"akuroth_chaos"=>.70+index*.34,"akuroth_meteor"=>.9+index*.47,
        "akuroth_rain"=>.8+index*.43,"akuroth_annihilation"=>index%2==0?1.10:3.0,_=>.95+index*.025};
    private void DrawAkurothAction(Unit u,Vector2 at,float h,bool friendly,float age,Color tint,float lean,Vector2 stretch){
        var atlas=akurothActionAtlases[u.Ascended?2:1];var id=Battle!.LastSkill!.Id;
        float first=(float)AkurothImpact(id,0),last=Battle.LastImpacts.Where(x=>x.Attacker==u).Select(x=>(float)AkurothImpact(id,x.HitIndex)).DefaultIfEmpty(first).Max();
        int frame=age<.12f?0:age<.30f?1:age<first*.7f?2:age<first?3:age<last+.35f?4+(int)((age-first)/.17f)%2:age<EffectDuration-.35f?6:7;
        if(u.AnnihilationCharging)frame=age<.2f?1:age<.6f?2:3;
        var box=atlas.Data.Frames[frame];float scale=h/atlas.Data.ReferenceHeight;
        DrawSetTransform(at,lean,new Vector2(friendly?-stretch.X:stretch.X,stretch.Y));
        DrawTextureRectRegion(atlas.Texture,new Rect2(-box[4]*scale,-box[5]*scale,box[2]*scale,box[3]*scale),new Rect2(box[0],box[1],box[2],box[3]),tint);
        DrawSetTransform(Vector2.Zero);
    }
    private void DrawAkurothFx(string id,Vector2 at,Vector2 size,float alpha=1,float rotation=0){
        if(alpha<=0)return;
        float ratio=akurothFx[id].GetWidth()/(float)akurothFx[id].GetHeight();
        if(size.X/size.Y>ratio)size.X=size.Y*ratio;else size.Y=size.X/ratio;
        DrawSetTransform(at,rotation);
        var tint=new Color(1,1,1,Mathf.Clamp(alpha,0,1));
        float amplitude=id is "waves" or "chaos"?4.5f:id is "cut" or "prison"?1:2.2f;
        Vector2 Vertex(Vector2 uv){float weight=Mathf.Sin(uv.X*Mathf.Pi)*Mathf.Sin(uv.Y*Mathf.Pi);return (uv-new Vector2(.5f,.5f))*size+new Vector2(Mathf.Sin((float)clock*9+uv.Y*12),Mathf.Cos((float)clock*7+uv.X*11))*amplitude*weight;}
        for(int y=0;y<4;y++)for(int x=0;x<8;x++){
            var a=new Vector2(x/8f,y/4f);var b=new Vector2((x+1)/8f,y/4f);var c=new Vector2(x/8f,(y+1)/4f);var d=new Vector2((x+1)/8f,(y+1)/4f);
            DrawPrimitive(new[]{Vertex(a),Vertex(b),Vertex(c)},new[]{tint,tint,tint},new[]{a,b,c},akurothFx[id]);
            DrawPrimitive(new[]{Vertex(b),Vertex(d),Vertex(c)},new[]{tint,tint,tint},new[]{b,d,c},akurothFx[id]);
        }
        DrawSetTransform(Vector2.Zero);
    }
    private float AkurothInfernoMix(){
        var boss=Battle?.Enemies.FirstOrDefault(u=>u.Id=="akuroth"&&u.Ascended);if(boss==null)return 0;
        if(!akurothTransformations.TryGetValue(boss,out var at))return 1;
        return Mathf.SmoothStep(0,1,Mathf.Clamp((float)(clock-at)/1.25f,0,1));
    }
    private float AkurothCameraZoom{
        get{
            float strength=0;float age=(float)(clock-actionAt);
            if(actor?.Id=="akuroth"&&Battle?.LastSkill is {} s&&age>=0&&age<EffectDuration){float end=Mathf.Clamp((EffectDuration-age)/.6f,0,1);strength=Mathf.Min(1,age/.4f)*end*(s.Id=="akuroth_annihilation"?.10f:.045f);}
            foreach(var t in akurothTransformations.Values){float a=(float)(clock-t);if(a>=0&&a<AkurothTransformDuration)strength=Mathf.Max(strength,Mathf.Sin(a/AkurothTransformDuration*Mathf.Pi)*.45f);}
            return 1+strength;
        }
    }
    private Vector2 AkurothCameraShake(){
        float energy=0,age=(float)(clock-actionAt);
        if(actor?.Id=="akuroth"&&Battle?.LastSkill is {} skill&&age>=0&&age<EffectDuration){
            float amplitude=skill.Id switch{"akuroth_annihilation"=>11,"akuroth_collapse"=>8,"akuroth_meteor"=>7,_=>3.5f};
            foreach(var hit in Battle.LastImpacts.Where(x=>x.Attacker==actor)){float t=age-(float)ImpactDelay(hit);if(t>=0&&t<.45f)energy=Mathf.Max(energy,Mathf.Exp(-t*10)*amplitude);}
        }
        return new Vector2(Mathf.Sin((float)clock*89),Mathf.Sin((float)clock*107+.5f))*energy;
    }
    private Vector2 AkurothCameraPoint(Vector2 point){var center=Size*new Vector2(.52f,.48f);return center+(point-AkurothCameraFocus)*AkurothCameraZoom+AkurothCameraShake();}
    private void DrawAkurothArena(){
        float zoom=AkurothCameraZoom;var center=Size*new Vector2(.52f,.48f);var region=new Rect2(center-AkurothCameraFocus*zoom+AkurothCameraShake(),Size*zoom);
        DrawTextureRect(arena,region,false);float inferno=AkurothInfernoMix();
        if(inferno>0){
            // Compress the distant skyline while expanding the playable floor.
            // Every combatant remains grounded on basalt, with the eclipse still visible.
            float horizon=.39f,sourceHorizon=.47f;var tint=new Color(1,1,1,inferno);
            DrawTextureRectRegion(akurothHell,new Rect2(region.Position,new Vector2(region.Size.X,region.Size.Y*horizon)),new Rect2(0,0,akurothHell.GetWidth(),akurothHell.GetHeight()*sourceHorizon),tint);
            DrawTextureRectRegion(akurothHell,new Rect2(region.Position+new Vector2(0,region.Size.Y*horizon),new Vector2(region.Size.X,region.Size.Y*(1-horizon))),new Rect2(0,akurothHell.GetHeight()*sourceHorizon,akurothHell.GetWidth(),akurothHell.GetHeight()*(1-sourceHorizon)),tint);
            for(int n=0;n<36;n++){float t=((float)clock*(.08f+n%4*.01f)+n*.618f)%1;var p=new Vector2((n*173+(float)clock*9)%Size.X,Size.Y*(1-t));DrawCircle(p,.7f+n%3*.4f,new Color(1,.18f,.06f,Mathf.Sin(t*Mathf.Pi)*inferno*.65f));}
        }
    }
    private Vector2 AkurothChaosPosition(Unit u,Vector2 home,float age){
        if(u!=actor||Battle?.LastSkill?.Id!="akuroth_chaos"||age<.25f)return home;
        var hits=Battle.LastImpacts.Where(h=>h.Attacker==u).ToArray();if(hits.Length==0)return home;
        int index=Math.Clamp((int)((age-.4f)/.34f),0,hits.Length-1);var victim=hits[index].Target;
        var end=CellFoot(Battle.Allies.Contains(victim.Origin??victim),victim.Column,victim.Layer)+new Vector2(index%2==0?70:-70,index%2==0?0:-12);
        if(age>2.65f)return end.Lerp(home,Mathf.SmoothStep(0,1,Mathf.Clamp((age-2.65f)/.55f,0,1)));
        if(age<.7f)return home.Lerp(end,Mathf.SmoothStep(0,1,(age-.25f)/.45f));
        return end;
    }
    private Vector2 AkurothVictimPosition(Unit u,Vector2 home,float age){
        if(actor?.Id!="akuroth"||u==actor||Battle?.LastSkill is not {} skill||!Battle.LastImpacts.Any(h=>h.Target==u&&h.Attacker==actor))return home;
        if(skill.Id=="akuroth_annihilation"&&age<EffectDuration){
            var core=Size*new Vector2(.50f,.45f);int slot=u.Column+u.Layer*3;
            float pull=age<3?Mathf.SmoothStep(0,1,Mathf.Clamp((age-.3f)/1.2f,0,1)):1-Mathf.SmoothStep(0,1,Mathf.Clamp((age-3)/1.15f,0,1));
            float angle=slot*2.39996f+age*1.3f;var orbit=core+new Vector2(Mathf.Cos(angle)*48,55+Mathf.Sin(angle)*28);
            var point=home.Lerp(orbit,pull*.97f);
            if(age>=3&&age<4.5f)point+=(home-core).Normalized()*Mathf.Sin(Mathf.Clamp((age-3)/1.5f,0,1)*Mathf.Pi)*30;
            return point;
        }
        if(!u.Alive)return home;
        float t=Mathf.Clamp((age-.7f)/(EffectDuration-1.2f),0,1),lift=Mathf.Sin(t*Mathf.Pi);
        if(skill.Id=="akuroth_collapse")return home+new Vector2(Mathf.Sin(age*4+u.Column)*7*lift,-56*lift);
        if(skill.Id=="akuroth_rift"){
            var victims=Battle.LastImpacts.Where(h=>h.Attacker==actor).Select(h=>h.Target).Distinct().ToArray();
            var center=victims.Select(v=>CellFoot(Battle.Allies.Contains(v.Origin??v),v.Column,v.Layer)).Aggregate(Vector2.Zero,(a,b)=>a+b)/victims.Length;
            return home.Lerp(center,lift*.22f)+new Vector2(0,-20*lift);
        }
        return home;
    }
    private void DrawAkurothSoulChains(){
        if(Battle==null)return;
        foreach(var unit in Battle.AlliedTargets.Concat(Battle.EnemyTargets).Where(u=>u.Alive&&u.Statuses.Any(s=>s.Effect.Id=="akuroth_drain")))
            DrawAkurothFx("prison",GroundPosition(unit)-new Vector2(0,70),new Vector2(155,190),.42f);
    }
    private void DrawAkurothCinematicOverlay(){
        float age=(float)(clock-actionAt);if(actor?.Id!="akuroth"||Battle?.LastSkill is not {} skill||age<0||age>EffectDuration)return;
        float opacity=Mathf.Min(1,age/.25f)*Mathf.Clamp((EffectDuration-age)/.4f,0,1);
        float bar=18*opacity;DrawRect(new Rect2(0,0,Size.X,bar),new Color(0,0,0,.88f));DrawRect(new Rect2(0,Size.Y-bar,Size.X,bar),new Color(0,0,0,.88f));
        string title=actor.AnnihilationCharging?"ANIQUILAÇÃO ABSOLUTA • PREPARAÇÃO":skill.Name.ToUpperInvariant();
        float w=Font.GetStringSize(title,fontSize:20).X;
        DrawStringOutline(Font,new Vector2((Size.X-w)/2,158),title,fontSize:20,size:5,modulate:new Color(0,0,0,opacity));
        Text(title,new Vector2((Size.X-w)/2,158),new Color(1,.83f,.86f,opacity),20);
        // Dark edge bands add contrast without covering targets or the boss HUD.
        for(int n=0;n<5;n++){float k=(5-n)*.012f*opacity;DrawRect(new Rect2(n*12,0,12,Size.Y),new Color(0,0,0,k));DrawRect(new Rect2(Size.X-(n+1)*12,0,12,Size.Y),new Color(0,0,0,k));}
    }
    private void DrawAkurothCinematicPower(float age){
        DrawAkurothFlow(age);
        var skill=Battle!.LastSkill!;var u=actor!;string id=skill.Id[8..];
        var from=GroundPosition(u)-new Vector2(0,110*AkurothCameraZoom);float duration=EffectDuration;
        float fade=Mathf.Clamp((duration-age)/.6f,0,1),charge=Mathf.Clamp(age/.9f,0,1);
        if(age<.9f||u.AnnihilationCharging){
            float power=u.AnnihilationCharging?Mathf.Min(1,age/1.7f):charge;
            DrawAkurothFx("annihilation",from-new Vector2(0,45),Vector2.One*(40+power*(u.AnnihilationCharging?175:48)),fade*.8f,(float)clock*.05f);
        }
        if(u.AnnihilationCharging){
            DrawAkurothFx("collapse",from,Vector2.One*(110+charge*200),fade*.65f,-age*.1f);return;
        }
        var hits=Battle.LastImpacts.Where(h=>h.Attacker==u).ToArray();
        if(id is "waves" or "rift" or "collapse" or "rewrite"){
            var targets=hits.Select(h=>h.Target).Distinct().ToArray();if(targets.Length==0)return;
            var center=targets.Select(t=>GroundPosition(t)-new Vector2(0,48)).Aggregate(Vector2.Zero,(a,b)=>a+b)/targets.Length;
            float t=age-.3f,arrival=.95f;
            if(id=="waves"&&t>=0){
                float travel=Mathf.SmoothStep(0,1,Mathf.Clamp(t/.8f,0,1));var wave=from.Lerp(center,travel);
                DrawAkurothFx(id,wave,new Vector2(300+travel*180,220+travel*120),fade,Mathf.Sin(age)*.1f);
            }else if(id=="rift"&&age>=.5f){
                float open=Mathf.SmoothStep(0,1,Mathf.Clamp((age-.5f)/.5f,0,1));DrawAkurothFx(id,center,new Vector2(520*open,270),fade);
            }else if(id=="collapse"&&age>=.45f){
                float open=Mathf.Clamp((age-.45f)/.6f,0,1);DrawAkurothFx(id,center,Vector2.One*(220+open*270),fade,-age*.13f);
            }else if(id=="rewrite"&&age>=.4f){
                DrawAkurothFx(id,center-new Vector2(0,40),new Vector2(530,350)*Mathf.Clamp((age-.4f)/.5f,0,1),fade);
            }
            if(age>arrival&&id!="waves")foreach(var target in targets)DrawAkurothFx("cut",GroundPosition(target)-new Vector2(0,45),new Vector2(130,60),fade*.45f,(float)clock*.1f);
        }
        foreach(var hit in hits){
            var to=GroundPosition(hit.Target)-new Vector2(0,48);float local=age-(float)ImpactDelay(hit);
            switch(id){
                case "cut":
                    if(local>=-.08f&&local<.55f){float a=Mathf.Clamp((local+.08f)/.18f,0,1),alpha=(1-Mathf.Clamp(local/.55f,0,1));DrawAkurothFx("cut",to+new Vector2(30*(1-a),0),new Vector2(260,95)*(.7f+.3f*a),alpha,(hit.HitIndex%3-1)*.32f);}break;
                case "prison":
                    if(local>=-.3f){float open=Mathf.Clamp((local+.3f)/.35f,0,1);DrawAkurothFx("prison",to,new Vector2(190,245)*open,fade);DrawAkurothFx("rift",to-new Vector2(0,45),new Vector2(80,160),fade*.35f,-Mathf.Pi/2);}break;
                case "meteor":
                    if(local>=-.65f&&local<.2f){float t=Mathf.Clamp((local+.65f)/.65f,0,1);var p=(to+new Vector2(140,-260)).Lerp(to,t);DrawAkurothFx("meteor",p,new Vector2(235,265),fade);}
                    if(local>=0&&local<1.2f)DrawAkurothFx("annihilation",to,Vector2.One*(90+Mathf.Min(local,.45f)*280),(1-local/1.2f)*.7f);break;
                case "chaos":
                    if(local>=-.2f&&local<.75f){float alpha=Mathf.Sin(Mathf.Clamp((local+.2f)/.95f,0,1)*Mathf.Pi);DrawAkurothFx("chaos",to,Vector2.One*230,alpha,(hit.HitIndex%2==0?1:-1)*local*.8f);DrawAkurothFx("cut",to,new Vector2(240,85),alpha,(hit.HitIndex-2)*.48f);}break;
                case "rain":
                    if(local>=-.55f&&local<1f){float t=Mathf.Clamp((local+.55f)/.55f,0,1);DrawAkurothFx("rain",to+new Vector2(0,-135+90*t),new Vector2(195,280),fade*(local>.4f?Mathf.Clamp(1-local,0,1):1));}break;
                case "annihilation":
                    break;
            }
        }
        if(id=="annihilation"){
            float t=age-3.0f;var center=Size*new Vector2(.5f,.45f);
            if(t<0)DrawAkurothSingularity(center,40+Mathf.Min(age/2.4f,1)*112,fade);
            else {float expansion=1-Mathf.Exp(-t*3);DrawAkurothFx(id,center,new Vector2(520+expansion*580,450+expansion*360),fade);DrawRect(new Rect2(Vector2.Zero,Size),new Color(.7f,.035f,.09f,Mathf.Max(0,.1f-t*.22f)));}
        }
    }
}
