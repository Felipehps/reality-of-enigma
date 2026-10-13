using Godot;
using Vaelorn;
using System;
using System.Linq;
public partial class Battlefield
{
    public double ImpactDelay(CombatImpact hit)=>Battle?.LastSkill?.Id is {} id&&id.StartsWith("akuroth_")&&hit.Attacker?.Id=="akuroth"?AkurothImpact(id,hit.HitIndex):Battle?.LastSkill?.Id=="gale_fangs"?.75+hit.HitIndex*.46:HitMoment+(Battle?.LastSkill?.Operator=="horizon"?Battle.LastImpacts.IndexOf(hit)*.23:hit.HitIndex*.095);
    private bool IsSpiral(Unit unit)=>Battle?.LastSkill?.Id=="gale_fangs"&&(unit==actor||unit.Origin==actor)&&clock-actionAt>=.18&&clock-actionAt<3.24;
    private Vector2 SpiralPosition(Unit unit,Vector2 home,float age){
        var impacts=Battle!.LastImpacts.Where(h=>h.Attacker==unit).ToArray();
        if(impacts.Length==0)return home;
        Vector2 Target(int i){var t=impacts[Math.Clamp(i,0,impacts.Length-1)].Target;return CellFoot(Battle.Allies.Contains(t.Origin??t),t.Column,t.Layer)+new Vector2(Mathf.Cos(i*1.7f+(unit.Origin==null?0:Mathf.Pi))*48,Mathf.Sin(i*1.7f+(unit.Origin==null?0:Mathf.Pi))*24);}
        Vector2 from=home,to=Target(0);float start=.18f,end=.75f;
        for(int i=1;i<impacts.Length&&age>end;i++){from=to;to=Target(i);start=end;end=.75f+i*.46f;}
        if(age>end){from=to;to=home;start=end;end=3.24f;}
        float t=Mathf.Clamp((age-start)/(end-start),0,1);
        // One continuous curve through impact positions; no vertical reset at each hit.
        int segment=age<=.75f?0:Math.Min(impacts.Length,(int)Mathf.Ceil((age-.75f)/.46f));
        Vector2 Point(int n)=>n<=0||n>impacts.Length?home:Target(n-1);
        var before=Point(segment-1);var after=Point(segment+2);
        var m0=(to-before)*.35f;var m1=(after-from)*.35f;
        return (2*t*t*t-3*t*t+1)*from+(t*t*t-2*t*t+t)*m0+(-2*t*t*t+3*t*t)*to+(t*t*t-t*t)*m1;
    }
    private bool DrawSpiral(Unit unit,Vector2 at,float height,float age){
        if(!IsSpiral(unit)||!frameAtlases.TryGetValue("fenrath_spiral",out var atlas))return false;
        int frame=(int)(age*24)%8+(unit.Origin!=null?8:0);
        var b=atlas.Data.Frames[frame];var size=new Vector2(height*1.6f,height*.95f);
        var center=at-new Vector2(0,height*.5f);
        var home=CellFoot(Battle!.Allies.Contains(unit.Origin??unit),unit.Column,unit.Layer);
        if(unit.Origin!=null)home+=new Vector2(Battle.Allies.Contains(unit.Origin)?-70:70,62);
        var tangent=SpiralPosition(unit,home,Mathf.Min(3.239f,age+.025f))-SpiralPosition(unit,home,Mathf.Max(.18f,age-.025f));
        float angle=tangent.LengthSquared()>1?tangent.Angle():0;
        DrawSetTransform(center,angle);
        for(int i=3;i>=1;i--)DrawTextureRectRegion(atlas.Texture,new Rect2(-size/2-new Vector2(i*12,0),size),new Rect2(b[0],b[1],b[2],b[3]),new Color(1,1,1,.05f*(4-i)));
        DrawTextureRectRegion(atlas.Texture,new Rect2(-size/2,size),new Rect2(b[0],b[1],b[2],b[3]));
        DrawSetTransform(Vector2.Zero);
        return true;
    }
}
