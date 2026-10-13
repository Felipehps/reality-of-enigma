using Godot;
using System;
using System.Linq;

public partial class Battlefield
{
    // Ribbons have independently moving heads and tails, with a soft luminous core.
    private void DrawEnergyRibbon(Func<float,Vector2> path,float head,float length,float width,Color color,float opacity){
        if(head<=0||head>=1+length||opacity<=0)return;
        float start=Mathf.Max(0,head-length),end=Mathf.Min(1,head);
        Vector2 Edge(float t,float side){var tangent=(path(Mathf.Min(1,t+.002f))-path(Mathf.Max(0,t-.002f))).Normalized();return path(t)+new Vector2(-tangent.Y,tangent.X)*width*side;}
        for(int n=0;n<16;n++){
            float a=Mathf.Lerp(start,end,n/16f),b=Mathf.Lerp(start,end,(n+1)/16f);
            float aa=Mathf.Sin((a-start)/Mathf.Max(.001f,end-start)*Mathf.Pi)*opacity,ba=Mathf.Sin((b-start)/Mathf.Max(.001f,end-start)*Mathf.Pi)*opacity;
            Color coreA=new(color,aa),coreB=new(color,ba),edge=new(color,0);
            for(int side=-1;side<=1;side+=2){
                DrawPrimitive(new[]{path(a),path(b),Edge(a,side)},new[]{coreA,coreB,edge},Array.Empty<Vector2>());
                DrawPrimitive(new[]{path(b),Edge(b,side),Edge(a,side)},new[]{coreB,edge,edge},Array.Empty<Vector2>());
            }
        }
    }
    private void DrawImpactWave(Vector2 at,float elapsed,float size,Color color){
        if(elapsed<0||elapsed>1)return;
        float radius=size*(1-Mathf.Exp(-elapsed*6)),alpha=(1-elapsed)*(1-elapsed);
        var ring=Enumerable.Range(0,49).Select(n=>at+new Vector2(Mathf.Cos(n*Mathf.Tau/48)*radius,Mathf.Sin(n*Mathf.Tau/48)*radius*.33f)).ToArray();
        DrawPolyline(ring,new Color(color,alpha*.4f),5,true);DrawPolyline(ring,new Color(1,.8f,.9f,alpha*.6f),1.2f,true);
        for(int n=0;n<12;n++){
            float a=n*2.39996f;var p=at+new Vector2(Mathf.Cos(a),Mathf.Sin(a)*.45f)*radius;
            DrawLine(p,p-new Vector2(Mathf.Cos(a),Mathf.Sin(a)*.45f)*(4+elapsed*14),new Color(color,alpha*.6f),1.5f,true);
        }
    }
    private void DrawVortexFlow(Vector2 center,float radius,float opacity){
        for(int strand=0;strand<6;strand++){
            float seed=strand*2.39996f;float speed=.5f+strand%3*.11f;
            Vector2 Spiral(float t){float a=seed+t*6.2f-(float)clock*speed,r=radius*(3.1f-2.25f*t);return center+new Vector2(Mathf.Cos(a)*r,Mathf.Sin(a)*r*.55f);}
            float head=((float)clock*speed+strand*.618f)%1.4f;
            DrawEnergyRibbon(Spiral,head,.38f,5+strand%3*2,new Color(strand%2==0?"f52e65":"aa3ddd"),opacity*.4f);
        }
    }
    private void DrawAkurothFlow(float age){
        if(actor?.Id!="akuroth"||Battle?.LastSkill is not {} skill||actor.AnnihilationCharging)return;
        string id=skill.Id[8..];var color=PowerColor(actor);var from=GroundPosition(actor)-new Vector2(0,110);
        foreach(var hit in Battle.LastImpacts.Where(h=>h.Attacker==actor)){
            float local=age-(float)ImpactDelay(hit);var to=GroundPosition(hit.Target)-new Vector2(0,52);
            if(id is "cut" or "chaos"){
                float direction=hit.HitIndex%2==0?1:-1;
                Vector2 Arc(float t)=>to+new Vector2(Mathf.Cos((t-.5f)*Mathf.Pi)*direction*65,Mathf.Sin((t-.5f)*Mathf.Pi)*78);
                DrawEnergyRibbon(Arc,(local+.17f)/.36f,.55f,15,color,1);
            }else if(id is "meteor" or "rain"){
                var start=to+new Vector2(id=="meteor"?140:35,-310);
                Vector2 Fall(float t)=>start.Lerp(to,t)+new Vector2(Mathf.Sin(t*Mathf.Pi)*25,0);
                DrawEnergyRibbon(Fall,(local+.65f)/.65f,.48f,16,color,.9f);
            }else if(id is "waves" or "prison" or "rewrite"){
                var normal=(to-from).Orthogonal().Normalized();
                Vector2 Path(float t)=>from.Lerp(to,t)+normal*Mathf.Sin(t*Mathf.Pi)*(hit.HitIndex%2==0?50:-50);
                DrawEnergyRibbon(Path,(local+.65f)/.65f,.42f,id=="waves"?27:9,color,.65f);
            }
            if(id!="annihilation")DrawImpactWave(GroundPosition(hit.Target),local,id=="collapse"?110:55,color);
        }
        if(id=="annihilation"&&age>=3){
            var center=Size*new Vector2(.5f,.45f);DrawImpactWave(center+new Vector2(0,80),age-3,Size.X*.55f,color);
            DrawImpactWave(center+new Vector2(0,80),age-3.16f,Size.X*.5f,new Color("ff99bb"));
        }
    }
}
