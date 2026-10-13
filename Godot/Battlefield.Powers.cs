using Godot;
using Vaelorn;
using System;
using System.Linq;

public partial class Battlefield
{
    private void EffectSprite(int cell,Vector2 at,Vector2 size,float opacity=1,float rotation=0)
    {
        if(opacity<=0)return;
        var region=new Rect2(cell%3*512,cell/3*512,512,512);
        DrawSetTransform(at,rotation);
        DrawTextureRectRegion(effectAtlas,new Rect2(-size/2,size),region,new Color(1,1,1,Mathf.Clamp(opacity,0,1)));
        DrawSetTransform(Vector2.Zero);
    }
    private void Glow(Vector2 at,Color color,float radius,float opacity)
    {
        for(int n=4;n>=1;n--)DrawCircle(at,radius*n/4,new Color(color,opacity*(5-n)*.045f));
    }
    private void Ribbon(Vector2[] path,float width,Color color)
    {
        if(path.Length<2)return;
        var left=new Vector2[path.Length];var right=new Vector2[path.Length];
        for(int n=0;n<path.Length;n++){
            var direction=path[Math.Min(n+1,path.Length-1)]-path[Math.Max(0,n-1)];
            var normal=direction.Normalized().Orthogonal();
            float thickness=width*Mathf.Sin(Mathf.Pi*(n+.5f)/path.Length);
            left[n]=path[n]+normal*thickness;right[n]=path[n]-normal*thickness;
        }
        // Independent triangles also support very short or tightly curved trails.
        for(int n=0;n<path.Length-1;n++){
            DrawPrimitive(new[]{left[n],right[n],left[n+1]},new[]{color,color,color},Array.Empty<Vector2>());
            DrawPrimitive(new[]{right[n],right[n+1],left[n+1]},new[]{color,color,color},Array.Empty<Vector2>());
        }
    }
    private void Sparks(Vector2 at,Color color,float time,int count,float distance=90)
    {
        if(time<0||time>1)return;
        for(int n=0;n<count;n++){
            float angle=n*2.39996f, speed=.45f+(n%7)/10f;
            var ray=new Vector2(Mathf.Cos(angle),Mathf.Sin(angle));
            var p=at+ray*time*distance*speed+new Vector2(0,time*time*28);
            DrawLine(p-ray*(8*(1-time)),p,new Color(color,(1-time)*(1-time)),n%3==0?3:1.5f,true);
        }
    }
    private void Crystal(Vector2 at,float height,float tilt,float opacity)
    {
        if(opacity<=0||height<1)return;
        DrawSetTransform(at,tilt);float w=height*.18f;
        var top=new Vector2(0,-height);var baseLeft=new Vector2(-w,0);var baseRight=new Vector2(w,0);
        DrawColoredPolygon(new[]{top,baseLeft,baseRight},new Color(.12f,.45f,.85f,opacity));
        DrawColoredPolygon(new[]{top,new Vector2(-w*.15f,-height*.15f),baseLeft},new Color(.45f,.9f,1,opacity));
        DrawLine(top,Vector2.Zero,new Color(.9f,1,1,opacity),2,true);DrawSetTransform(Vector2.Zero);
    }
    private void DrawPowers()
    {
        float age=(float)(clock-actionAt);var skill=Battle?.LastSkill;
        if(actor==null||skill==null||age<0||age>EffectDuration||!cards.TryGetValue(actor,out var source))return;
        if(skill.Id=="gale_fangs")return;
        if(actor.Id=="akuroth"){DrawAkurothPower(age);return;}
        if(skill.Operator.StartsWith("floral_")){DrawFloralPower(age);return;}
        bool friendly=Battle!.Allies.Contains(actor);float direction=friendly?1:-1;
        var from=GroundPosition(actor)+new Vector2(direction*25,-source.Size.Y*.60f);
        var color=PowerColor(actor);float after=age-HitMoment;
        float fade=Mathf.Clamp((EffectDuration-age)/.6f,0,1);
        bool healing=skill.Operator=="heal", breaking=skill.Operator=="break";
        var targets=floats.Where(f=>Math.Abs(f.at-actionAt)<.001&&(healing?f.delta>0:f.delta<0)&&f.unit!=actor).Select(f=>f.unit).Distinct().ToList();
        if(Battle.LastTarget is Unit target&&!targets.Contains(target)&&!breaking)targets.Add(target);
        if(breaking){targets.Clear();targets.Add(actor);}
        if(skill.Operator=="buff"){targets.Clear();targets.AddRange(Battle.LastBuffTargets);color=new Color("68bdff");}
        if(skill.Cooldown>0&&age<HitMoment){
            float buildup=age/HitMoment;
            var center=GroundPosition(actor);
            DrawArc(center-new Vector2(0,source.Size.Y*.4f),28+buildup*17,-Mathf.Pi*.85f+age*2,Mathf.Pi*.65f+age*2,32,new Color(color,Mathf.Sin(buildup*Mathf.Pi)*.55f),2,true);
            Sparks(from,color,1-buildup,10,45);
        }
        // A compact charge at the hand precedes travel and the common impact frame.
        if(age<HitMoment){
            float charge=Mathf.Clamp(age/.42f,0,1);
            Glow(from,color,12+charge*25,1);
            for(int n=0;n<8;n++){
                float a=n*Mathf.Tau/8+age*3;float radius=38*(1-charge)+6;
                var p=from+new Vector2(Mathf.Cos(a),Mathf.Sin(a))*radius;
                DrawLine(p,p+new Vector2(-direction*6,2),new Color(color,charge),2,true);
            }
        }
        if(skill.Operator=="horizon"&&targets.Count>0){
            var ordered=Battle.LastImpacts.Select(i=>i.Target).Distinct().OrderBy(u=>u.Layer).ToArray();
            if(ordered.Length>0&&age>=.35f){
                var origin=from;
                for(int i=0;i<ordered.Length;i++){
                    var end=cards[ordered[i]].GetCenter();
                    float arrival=HitMoment+i*.23f;
                    float start=i==0?.35f:arrival-.23f;
                    float progress=Mathf.Clamp((age-start)/(arrival-start),0,1);
                    if(age<start)break;
                    var tip=origin.Lerp(end,progress);
                    var directionLine=tip-origin;
                    var normal=directionLine.Normalized().Orthogonal();
                    var path=Enumerable.Range(0,25).Select(n=>origin+directionLine*n/24+normal*Mathf.Sin(n*.8f-age*30)*2).ToArray();
                    Ribbon(path,13,new Color(color,.20f*fade));Ribbon(path,4,new Color(color,.85f*fade));Ribbon(path,1.4f,new Color(.97f,.86f,1,fade));
                    EffectSprite(4,tip,new Vector2(38,38),fade,age*3);
                    origin=end;
                }
            }
        }
        if(skill.Operator=="row"&&Battle.LastTarget is Unit rowTarget&&after>=0)DrawRowTrail(rowTarget,after,fade);
        foreach(var u in targets){
            if(!cards.TryGetValue(u,out var dest))continue;
            var to=dest.GetCenter();var ground=new Vector2(to.X,dest.End.Y-22);
            float travel=Mathf.Clamp((age-.35f)/.3f,0,1);
            if(skill.Operator=="buff"){
                if(after>=0){
                    Glow(to,blue,60,fade);
                    DrawBuffIcon(skill.Scale=="speed"?"speed":"damage",to-new Vector2(12,25+after*45));
                    Sparks(to,blue,after,10,55);
                }
                continue;
            }
            if(healing){
                var path=Enumerable.Range(0,28).Select(n=>{
                    float t=n/27f;return from.Lerp(to,t*travel)+new Vector2(0,-Mathf.Sin(t*Mathf.Pi)*45+Mathf.Sin(t*12-age*8)*6);
                }).ToArray();
                Ribbon(path,2,new Color("eace8e",fade*.7f));
                if(after>=0){DrawArc(to,30+after*26,0,Mathf.Tau,48,new Color(.55f,1,.75f,fade*.55f),2,true);Glow(to,new Color("86ffc0"),65,fade);for(int n=0;n<14;n++){float t=(after+n*.071f)%1;Star(ground+new Vector2(Mathf.Sin(n*3.4f)*42,-t*125),2,new Color(.65f,1,.8f,Mathf.Sin(t*Mathf.Pi)*fade));}}
                continue;
            }
            if(actor.Id=="lyra"){
                if(age>=.35f&&after<0){
                    var p=from.Lerp(to,travel);Crystal(p+new Vector2(0,12),28,(to-from).Angle()+Mathf.Pi/2,1);
                    Ribbon(new[]{from.Lerp(to,Math.Max(0,travel-.25f)),p},4,new Color(.4f,.9f,1,.45f));
                }
                if(after>=0){
                    float grow=1-Mathf.Pow(1-Mathf.Clamp(after/.22f,0,1),3);
                    float size=skill.Id=="ice"?90:skill.Id=="winter"?185:150;
                    ImpactBloom(to,after,blue,size*.48f);
                    for(int shard=0;shard<12;shard++){float a=shard*2.39996f;var v=new Vector2(Mathf.Cos(a),Mathf.Sin(a));float t=Mathf.Min(after,.7f);Crystal(to+v*t*95+new Vector2(0,t*t*28),8+shard%4*3,a,Mathf.Max(0,1-after/.7f));}
                    Sparks(to,blue,after,16,65);
                    if(skill.Id=="prison")for(int n=0;n<5;n++){
                        float x=(n-2)*19;Crystal(ground+new Vector2(x,0),(40+16*(1-Math.Abs(n-2)/2f))*grow,(n-2)*.12f,Mathf.Max(0,1-after/.65f)*.7f);
                    }
                    if(skill.Id is "glacial" or "winter")for(int n=0;n<8;n++){
                        float fall=(after*1.8f+n*.17f)%1;
                        Crystal(to+new Vector2((n-3.5f)*17-15*fall,-90+fall*135),18+n%3*6,-.35f,fade);
                    }
                }
            }else if(actor.Id=="kael"&&actor.Path!="martial"||actor.Id.StartsWith("enemy")){
                int count=Math.Clamp(skill.Hits,1,4);
                for(int n=0;n<count;n++){
                    float slash=after-n*.095f;if(slash<0||slash>.5f)continue;
                    float alpha=Mathf.Sin(Mathf.Clamp(slash/.5f,0,1)*Mathf.Pi);
                    SlashArc(to,slash,new Color("95fff1"),n%2==0?-.65f:.65f,skill.Cooldown>0?94:73);
                    ImpactBloom(to,slash,new Color("d0fff6"),40);
                }
                Sparks(to,new Color("d8fff7"),after,18,85);
            }else if(skill.Operator=="horizon"){
                int sequence=Battle.LastImpacts.FindIndex(hit=>hit.Target==u);
                float impact=after-Math.Max(0,sequence)*.23f;
                if(impact>=0){ImpactBloom(to,impact,color,55);SlashArc(to,impact,color,Mathf.Pi/2,62);}
            }else if(actor.Id is "sevrin" or "maltherion"){
                var slashColor=new Color(actor.Id=="sevrin"?"ef789b":"b990f1");
                foreach(var impact in Battle.LastImpacts.Where(i=>i.Target==u)){
                    float elapsed=age-(float)ImpactDelay(impact);
                    if(elapsed<0||elapsed>.65f)continue;
                    SlashArc(to,elapsed,slashColor,impact.HitIndex%2==0?-.65f:.65f,skill.Cooldown>0?112:70);
                    ImpactBloom(to,elapsed,new Color("ffe3e7"),45);Sparks(to,slashColor,elapsed,12,100);
                }
                if(skill.Id=="eclipse_final"&&age<HitMoment){
                    var eclipseAt=new Vector2(Size.X*.5f,Size.Y*.24f);
                    DrawCircle(eclipseAt,60,new Color("100517"));DrawArc(eclipseAt,64,0,Mathf.Tau,64,new Color("e166a2"),4,true);
                    Glow(eclipseAt,new Color("c167de"),90,1);
                }
            }else if(actor.Id=="savor"){
                if(after>=0){SlashArc(to,after,new Color("ff9b4b"),-.4f,95);ImpactBloom(to,after,new Color("ffcf81"),65);}
            }else if(actor.Id=="brakk"){
                if(after>=0){
                    float ring=after*110;DrawEllipse(ground,new Vector2(ring,ring*.3f),new Color(.95f,.65f,.3f,.18f*fade));
                    ImpactBloom(to,after,new Color("ffdf9d"),84);
                    for(int n=0;n<9;n++){float a=n*Mathf.Tau/9;var p=ground+new Vector2(Mathf.Cos(a)*ring,Mathf.Sin(a)*ring*.3f);DrawLine(p,p+new Vector2(Mathf.Cos(a)*16,Mathf.Sin(a)*5),new Color(.85f,.7f,.5f,fade),3,true);}
                }
            }else if(actor.Id=="solarius"){
                if(age<HitMoment)EffectSprite(2,to-new Vector2(0,90),new Vector2(95,95)*Mathf.Clamp(age/.5f,0,1));
                else{EffectSprite(2,to,new Vector2(190,220),fade);Sparks(to,gold,after,24,130);}
            }else if(actor.Id=="kael"){
                if(after>=0){float size=skill.Operator=="extreme"?250:150;EffectSprite(5,to,new Vector2(size,size)*(.8f+after*.35f),fade);Sparks(to,color,after,30,size*.6f);}
            }else{
                if(age>=.35f&&after<0)EffectSprite(4,from.Lerp(to,travel),new Vector2(60,60),1,age*3);
                if(after>=0){ImpactBloom(to,after,color,72);SlashArc(to,after,color,.8f,60);}
            }
        }
        DrawRect(new Rect2(Size.X/2-190,Size.Y-29,380,32),new Color(.025f,.04f,.065f,.88f));
        DrawLine(new Vector2(Size.X/2-190,Size.Y-29),new Vector2(Size.X/2+190,Size.Y-29),new Color(color,.55f),1);
        Text(skill.Name,new Vector2(Size.X/2-172,Size.Y-7),new Color(color,Math.Min(1,fade*2)),17);
    }
}
