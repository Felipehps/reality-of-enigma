using Godot;
using Vaelorn;
using System;
using System.Linq;
using System.Collections.Generic;

public partial class Battlefield
{
    private record ArenaImpact(Unit Target,Unit? Source,double Amount,string Kind,string Skill,int Index,bool Critical,double At);
    private record ArenaDecal(Vector2 At,float Radius,bool Blood,bool Infernal,int Seed);
    private sealed class ArenaFragment {
        public Vector2 At,Velocity;public float Ground,Radius,Spin;public double Born;public bool Blood,Marked;
    }
    private readonly List<ArenaImpact> arenaImpacts=new();
    private readonly List<ArenaDecal> arenaDecals=new();
    private readonly List<ArenaFragment> arenaFragments=new();
    private readonly Random arenaRandom=new(88041);
    private bool previewTimeline;
    private double hitstopRemaining;
    public int ArenaImpactsPlayed{get;private set;}
    public int ArenaBloodBursts{get;private set;}
    public int ArenaCracksCreated{get;private set;}
    public void SetPreviewTime(double elapsed){previewTimeline=true;clock=actionAt+elapsed;UpdateArenaImpacts();UpdateActionSound();QueueRedraw();}
    private void ResetArena(){arenaImpacts.Clear();arenaDecals.Clear();arenaFragments.Clear();previewTimeline=false;hitstopRemaining=0;}
    private void ScheduleArenaImpacts(){
        foreach(var hit in Battle!.LastImpacts.Where(h=>h.Amount>0&&h.Kind!="heal"))
            arenaImpacts.Add(new(hit.Target,hit.Attacker,hit.Amount,hit.Kind,Battle.LastSkill?.Id??"",hit.HitIndex,hit.Critical,clock+ImpactDelay(hit)));
    }
    private void UpdateArenaImpacts(){
        foreach(var hit in arenaImpacts.Where(h=>h.At<=clock).ToArray()){
            arenaImpacts.Remove(hit);ArenaImpactsPlayed++;
            var foot=CellFoot(Battle!.Allies.Contains(hit.Target.Origin??hit.Target),hit.Target.Column,hit.Target.Layer);
            var visible=AkurothVictimPosition(hit.Target,RawGroundPosition(hit.Target),(float)(clock-actionAt));float severity=Mathf.Clamp((float)(hit.Amount/hit.Target.MaxHp),0,.35f);
            bool explosive=hit.Skill is "akuroth_meteor" or "akuroth_collapse" or "akuroth_annihilation";
            if(hit.Source!=null&&(hit.Kind=="physical"||explosive)){
                ArenaBloodBursts++;int count=hit.Critical?13:8;
                float side=visible.X-RawGroundPosition(hit.Source).X>=0?1:-1;
                for(int n=0;n<count;n++)arenaFragments.Add(new(){At=visible-new Vector2(0,45),Velocity=new Vector2(side*(55+(float)arenaRandom.NextDouble()*90),-55-(float)arenaRandom.NextDouble()*95),Ground=visible.Y,Radius=1.3f+severity*8,Blood=true,Born=clock,Spin=(float)arenaRandom.NextDouble()*6});
            }
            if(explosive&&!(hit.Skill=="akuroth_annihilation"&&hit.Index%2==0)){
                ArenaCracksCreated++;AddArenaDecal(new(foot,hit.Skill=="akuroth_annihilation"?92:45+severity*60,false,AkurothInfernoMix()>.5f,arenaRandom.Next()));
                for(int n=0;n<15;n++)arenaFragments.Add(new(){At=foot-new Vector2(0,3),Velocity=new Vector2(-120+(float)arenaRandom.NextDouble()*240,-100-(float)arenaRandom.NextDouble()*180),Ground=foot.Y,Radius=3+(float)arenaRandom.NextDouble()*7,Born=clock,Spin=-4+(float)arenaRandom.NextDouble()*8});
            }
            if(hit.Source!=null)hitstopRemaining=Math.Max(hitstopRemaining,hit.Skill=="akuroth_annihilation"&&hit.Index%2==1?.075:hit.Critical?.045:.025);
        }
        arenaFragments.RemoveAll(p=>clock-p.Born>4.5);
        if(arenaFragments.Count>500)arenaFragments.RemoveRange(0,arenaFragments.Count-500);
    }
    private void AddArenaDecal(ArenaDecal decal){arenaDecals.Add(decal);if(arenaDecals.Count>72)arenaDecals.RemoveAt(0);}
    private void DrawArenaDamage(){
        foreach(var decal in arenaDecals){
            var at=AkurothCameraPoint(decal.At);float radius=decal.Radius*AkurothCameraZoom;
            if(decal.Blood){DrawEllipse(at,new Vector2(radius*1.5f,radius*.38f),new Color(.25f,.012f,.025f,.62f));continue;}
            DrawEllipse(at,new Vector2(radius,radius*.25f),new Color(.025f,.015f,.022f,.55f));
            for(int n=0;n<7;n++){
                float a=n*Mathf.Tau/7+decal.Seed%9*.1f;var direction=new Vector2(Mathf.Cos(a),Mathf.Sin(a)*.28f);
                var mid=at+direction*radius*.65f;var end=at+direction*radius;
                DrawLine(at,mid,new Color(.012f,.006f,.01f,.8f),3,true);DrawLine(mid,end,new Color(.04f,.015f,.025f,.7f),1.7f,true);
                if(decal.Infernal)DrawLine(at,mid,new Color(.8f,.065f,.018f,.48f),1,true);
            }
        }
    }
    private void DrawArenaFragments(){
        foreach(var part in arenaFragments){
            float t=(float)(clock-part.Born);if(t<0)continue;
            const float gravity=490;
            float land=(-part.Velocity.Y+Mathf.Sqrt(part.Velocity.Y*part.Velocity.Y+2*gravity*(part.Ground-part.At.Y)))/gravity;
            var point=part.At+part.Velocity*t+new Vector2(0,.5f*gravity*t*t);
            if(t>land){float after=t-land;float bounce=(part.Velocity.Y+gravity*land)*.26f;point=new Vector2(part.At.X+part.Velocity.X*(land+after*.4f),Mathf.Min(part.Ground,part.Ground-bounce*after+.5f*gravity*after*after));
                if(part.Blood&&!part.Marked){part.Marked=true;AddArenaDecal(new(point,part.Radius*1.1f,true,false,0));}
            }
            point=AkurothCameraPoint(point);float alpha=Mathf.Clamp((4.5f-t)/.7f,0,1);
            if(part.Blood){DrawLine(point-new Vector2(part.Velocity.X*.018f,-2),point,new Color(.65f,.012f,.025f,alpha),part.Radius,true);continue;}
            DrawSetTransform(point,t*part.Spin);float r=part.Radius;
            DrawColoredPolygon(new[]{new Vector2(-r,-r*.3f),new Vector2(-r*.4f,-r),new Vector2(r*.7f,-r*.6f),new Vector2(r,r*.5f),new Vector2(0,r),new Vector2(-r,r*.5f)},new Color(.20f,.16f,.17f,alpha));
            DrawColoredPolygon(new[]{new Vector2(-r,-r*.3f),new Vector2(-r*.4f,-r),new Vector2(r*.7f,-r*.6f),Vector2.Zero},new Color(.42f,.32f,.30f,alpha));
            DrawSetTransform(Vector2.Zero);
        }
    }
    private void DrawAkurothSingularity(Vector2 center,float radius,float alpha){
        DrawVortexFlow(center,radius,alpha);
        DrawCircle(center,radius*.91f,new Color(.005f,.002f,.009f,alpha));
        var texture=akurothFx["annihilation"];var uvCenter=new Vector2(.5f,.49f);
        for(int n=0;n<48;n++){
            float a=n*Mathf.Tau/48,b=(n+1)*Mathf.Tau/48;
            Vector2 Dir(float angle)=>new(Mathf.Cos(angle),Mathf.Sin(angle));
            Vector2 V(float angle,float r)=>center+Dir(angle)*radius*r*(1+Mathf.Sin(angle*9+(float)clock*4)*.025f);
            Vector2 UV(float angle,float r)=>uvCenter+Dir(angle)*new Vector2(.145f,.255f)*r;
            Color inner=new(1,1,1,alpha),outer=new(1,1,1,alpha*.2f);
            DrawPrimitive(new[]{V(a,.84f),V(b,.84f),V(a,1.22f)},new[]{inner,inner,outer},new[]{UV(a,.84f),UV(b,.84f),UV(a,1.22f)},texture);
            DrawPrimitive(new[]{V(b,.84f),V(b,1.22f),V(a,1.22f)},new[]{inner,outer,outer},new[]{UV(b,.84f),UV(b,1.22f),UV(a,1.22f)},texture);
        }
        for(int n=0;n<20;n++){
            float t=((float)clock*.24f+n/20f)%1,a=n*2.39996f+t*5;var direction=new Vector2(Mathf.Cos(a),Mathf.Sin(a));
            var p=center+direction*radius*(1+2.6f*(1-t));
            DrawLine(p,p-direction*(8+18*t),new Color(1,.11f,.25f,Mathf.Sin(t*Mathf.Pi)*alpha*.6f),1.5f,true);
        }
    }
}
