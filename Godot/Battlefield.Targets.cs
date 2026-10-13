using Godot;
using Vaelorn;
using System.Linq;

public partial class Battlefield
{
    public string TargetIntent{get;private set;}="attack";
    public Unit? DisplayActor=>IsAnimating?actor??Battle?.NextActor:Battle?.NextActor;
    public void SetTargetIntent(Skill? skill){
        TargetIntent=skill?.Operator is "heal"?"heal":skill?.Operator is "buff"?"support":skill?.Operator is "split" or "break" or "eclipse_guard"?"self":skill?.Operator is "aoe" or "random" or "scarlet_aoe" or "eclipse_aoe"?"area":"attack";
        if(Battle==null)return;
        var pool=TargetIntent is "heal" or "support" or "self"?Battle.AlliedTargets:Battle.EnemyTargets;
        if(TargetIntent=="self")Selected=Battle.NextActor;
        if(skill!=null&&(Selected==null||!Selected.Alive||!pool.Contains(Selected)))Selected=pool.FirstOrDefault(u=>u.Alive);
        TooltipText="";QueueRedraw();
    }
    public bool IsHighlightedTarget(Unit unit){
        if(Battle==null||Battle.Finished||FormationEditing||EntranceActive||IsAnimating||DisplayActor==null||!Battle.Allies.Contains(DisplayActor))return false;
        bool ally=Battle.Allies.Contains(unit.Origin??unit);
        return unit.Alive&&(TargetIntent=="self"?unit==DisplayActor:TargetIntent=="support"?ally:TargetIntent=="area"?!ally:Selected==unit&&(TargetIntent=="heal"?ally:!ally));
    }
    private void DrawIdentityMarkers(){
        if(Battle==null||Battle.Finished||FormationEditing||EntranceActive||ExitActive)return;
        foreach(var pair in cards){
            var u=pair.Key;if(!u.Alive)continue;
            bool acting=u==DisplayActor,target=IsHighlightedTarget(u);
            if(!acting&&!target)continue;
            float h=(pair.Value.Size.Y-32)*(1-u.Layer*.045f)*(u.Id=="akuroth"?2.4f:1);
            var foot=GroundPosition(u);bool ally=Battle.Allies.Contains(u.Origin??u);
            var at=foot+new Vector2(ally?-42:42,-h-22-Mathf.Sin((float)clock*3)*3);
            if(acting)EnigmaIdentity.Icon(this,0,new Rect2(at-new Vector2(target?44:24,30),new Vector2(48,60)));
            if(target)EnigmaIdentity.Icon(this,TargetIntent is "heal" or "support" or "self"?2:1,new Rect2(at-new Vector2(acting?0:24,30),new Vector2(48,60)));
        }
    }
}
