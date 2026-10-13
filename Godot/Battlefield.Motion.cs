using Godot;
using Vaelorn;
using System;

public partial class Battlefield
{
    public static Vector2 MeleePosition(Vector2 home,Vector2 target,float direction,float age)
    {
        var contact=target-new Vector2(direction*82,0);
        if(age<.12f)return home-new Vector2(direction*6*Mathf.Sin(Mathf.Pi*Mathf.Max(0,age)/.12f),0);
        if(age<.53f)return home.Lerp(contact,Mathf.SmoothStep(0,1,(age-.12f)/.41f));
        if(age<1.08f)return contact;
        if(age<1.67f)return contact.Lerp(home,Mathf.SmoothStep(0,1,(age-1.08f)/.59f));
        return home;
    }
    private bool IsMelee(Unit unit)=>(unit==actor||unit.Origin==actor)&&Battle?.LastSkill is { } skill&&skill.Scale!="essence"&&!skill.Operator.StartsWith("floral_")&&skill.Operator is not ("break" or "buff" or "heal" or "split" or "eclipse_guard");
    private Vector2 GroundPosition(Unit unit)
        =>AkurothCameraPoint(AkurothVictimPosition(unit,RawGroundPosition(unit),(float)(clock-actionAt)));
    private Vector2 RawGroundPosition(Unit unit)
    {
        bool friendly=Battle!.Allies.Contains(unit.Origin??unit);var home=CellFoot(friendly,unit.Column,unit.Layer);
        if(EntranceActive)return EntrancePosition(friendly,unit.Column,unit.Layer,home);
        if(unit.Origin!=null)home+=new Vector2(friendly?-70:70,62);
        if(ExitActive&&friendly&&unit.Alive)return home+new Vector2(Size.X*ExitProgress,0);
        float age=(float)(clock-actionAt);
        if(unit.Id=="akuroth"&&Battle.LastSkill?.Id=="akuroth_chaos")return AkurothChaosPosition(unit,home,age);
        if(IsSpiral(unit))return SpiralPosition(unit,home,age);
        if(ShownFreeze(unit)>0||ShownStun(unit)>0||!IsMelee(unit)||age<0||age>=EffectDuration||Battle.LastTarget is not Unit target)return home;
        var goal=CellFoot(Battle.Allies.Contains(target.Origin??target),target.Column,target.Layer);
        return MeleePosition(home,goal,friendly?1:-1,age);
    }
    private bool IsRunning(Unit unit,float age)=>IsMelee(unit)&&(age>=.12f&&age<.48f||age>=1.12f&&age<1.63f);
}

