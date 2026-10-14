namespace Vaelorn;

public record CombatBuff(double Strength,double Essence,double Speed,int ExpiresRound);

public partial class Unit
{
    public Character Definition{get;}
    public string Id=>Definition.Id;
    public int Level{get;}
    public bool Boss{get;}
    public int Column{get;private set;}
    public int Layer{get;private set;}
    public string Path{get;}
    public string[] Archetypes=>Id=="kael"&&Path=="martial" ? new[]{"fighter"}:Definition.Archetypes;
    public double OriginalMaxHp{get;}
    public double MaxHp=>OriginalMaxHp*(Duality!=null?BodyMaxShare:Origin!=null?FormMaxShare:1);
    public double Hp{get;set;}
    private double Growth=>1+.03*(Level-1);
    public double Defense=>Definition.Stats.Defense*Growth*(1-StatusReduction(e=>e.DefenseReduction))*(1+StatusReduction(e=>e.DefenseBonus))*(Ascended?Definition.Eclipse?.Defense??1:1);
    public bool Crimson{get;private set;}
    public bool Blooming{get;private set;}
    public double OutgoingDamage=>(1-StatusReduction(e=>e.DamageReduction))*(Crimson?1+(Definition.Floral?.CrimsonDamage??0):1);
    public int Gate{get;private set;}
    public bool ExtremeUsed{get;private set;}
    private readonly HashSet<int> crossed=new();
    private int completedRounds;
    public int Ice{get;private set;}
    public int Freeze{get;private set;}
    public int Stun{get;private set;}
    public bool StunImmune{get;}
    public string SkippedBy{get;private set;}="";
    private int immunityAtStart,slowAtStart,techniqueSlowAtStart;
    public int Immunity{get;private set;}
    public int SlowTurns{get;private set;}
    public int TechniqueSlowTurns{get;private set;}
    public double TechniqueSlow{get;private set;}
    public int NextAction{get;set;}
    public Dictionary<string,int> ReadyRound{get;}=new();
    public bool Alive=>Hp>0;
    public bool DiedBleeding {get;private set;}
    public bool SurvivesDefeat=>Boss||Definition.StoryProtected;
    private readonly Dictionary<string,CombatBuff> buffs=new();
    public Dictionary<string,CombatBuff> Buffs=>Origin?.Buffs??buffs;
    public double BuffStrength=>Buffs.Values.Sum(b=>b.Strength);
    public double BuffEssence=>Buffs.Values.Sum(b=>b.Essence);
    public double BuffSpeed=>Buffs.Values.Sum(b=>b.Speed);
    public void ApplyBuff(string id,CombatBuff buff){if(Alive)Buffs[id]=buff;}
    public double Strength=>Definition.Stats.Strength*Growth*(Ascended?Definition.Eclipse?.Strength??1:1)*(1+BuffStrength)*(1+.08*Gate+(Gate==8?.25:0));
    public double Essence=>Definition.Stats.Essence*Growth*(1+BuffEssence);
    public double Speed=>Definition.Stats.Speed*(Ascended?Definition.Eclipse?.Speed??1:1)*(1+BleedingSpeed+FocusBleeds*(Definition.Scarlet?.TargetSpeedPerBleed??0))*(Crimson?1+(Definition.Floral?.CrimsonSpeed??0):1)*(1+BuffSpeed)*(1+.04*Gate+(Gate==8?.30:0))*(Gate==8?1:1-Math.Min(.5,.03*Ice+(SlowTurns>0?.15:0)+(TechniqueSlowTurns>0?TechniqueSlow:0)));
    public Unit(Character definition,int level=1,bool boss=false,int column=0,int layer=0,string path="base",bool? stunImmune=null)
    {
        Definition=definition;Level=level;Boss=boss;Column=column;Layer=layer;Path=path;StunImmune=stunImmune??(boss&&definition.Floral==null&&definition.Eclipse==null);
        OriginalMaxHp=definition.Stats.Hp*Growth;Hp=MaxHp;
    }
    internal void Reposition(int cell){Column=cell%3;Layer=cell/3;}
    public void OpenGate(int count=1)
    {if(Id=="kael"&&Path=="martial"&&Alive)Gate=Math.Min(Level>=70?8:7,Gate+count);}
    public void EndRound(int round){EclipseRound(round);completedRounds++;if(completedRounds%2==0)OpenGate();foreach(var key in Buffs.Where(b=>b.Value.ExpiresRound<=round).Select(b=>b.Key).ToArray())Buffs.Remove(key);}
    public double LoseHp(double damage)
    {
        var before=Hp;Hp=Math.Max(0,Hp-Math.Max(0,damage));
        foreach(var threshold in new[]{75,50,25})
            if(before/MaxHp*100>threshold&&Hp/MaxHp*100<=threshold&&crossed.Add(threshold))OpenGate();
        double lost=before-Hp;UpdateAscension();
        if(Definition.Floral is {} flower){if(Hp/MaxHp<flower.CrimsonThreshold)Crimson=true;if(Hp/MaxHp<flower.BloomThreshold)Blooming=true;}
        if(Hp<=0){DiedBleeding=BleedCount>0;(Origin??this).EndDuality();if(Alive)DiedBleeding=false;}
        return lost;
    }
    public void BreakLimits(){OpenGate();LoseHp(Math.Min(Hp-1,Hp*.08));}
    public bool CanExtreme=>Id=="kael"&&Path=="martial"&&Level>=70&&Gate==8&&!ExtremeUsed&&Alive;
    public void PayExtreme(){ExtremeUsed=true;LoseHp(MaxHp*.5);}
    public void EndOwnTurn()
    {
        if(immunityAtStart>0&&Immunity>0)Immunity--;
        if(slowAtStart>0&&SlowTurns>0)SlowTurns--;
        if(techniqueSlowAtStart>0&&TechniqueSlowTurns>0)TechniqueSlowTurns--;
        immunityAtStart=0;slowAtStart=0;techniqueSlowAtStart=0;
        if(Gate==8&&Alive)LoseHp(MaxHp*.1);
    }
    public bool ApplyStun(int turns=1)
    {
        if(!Alive||StunImmune||turns<=0)return false;
        Stun=Math.Max(Stun,turns);return true;
    }
    public void ApplySlow(double amount,int duration)
    {
        if(!Alive||Gate==8||amount<=0||duration<=0)return;
        TechniqueSlow=TechniqueSlowTurns>0?Math.Max(TechniqueSlow,amount):amount;
        TechniqueSlowTurns=Math.Max(TechniqueSlowTurns,duration);
    }
    public void ApplyIce(int count)
    {
        if(!Alive||count<=0||Immunity>0||Freeze>0||Gate==8)return;
        Ice=Math.Min(5,Ice+count);
        if(Ice<5)return;
        Ice=0;
        if(Boss){SlowTurns=2;Immunity=2;}else Freeze=1;
    }
    public bool PendingWakeDebuff {get;set;}
    public int PendingCooldownRefund {get;set;}
    public bool StartOwnTurn()
    {
        if(Stun==0&&PendingWakeDebuff){AddStatus(new StatusEffect("wake_weakness",1,1,DamageReduction:.2),this);PendingWakeDebuff=false;}
        immunityAtStart=Immunity;slowAtStart=SlowTurns;techniqueSlowAtStart=TechniqueSlowTurns;SkippedBy="";
        if(Statuses.Any(s=>s.Effect.Sleep))SkippedBy="sono";
        if(Freeze>0){Freeze--;SkippedBy="congelamento";if(Freeze==0)Immunity=2;}
        if(Stun>0){Stun--;SkippedBy=SkippedBy==""?"stun":"stun e congelamento";}
        return SkippedBy=="";
    }
}
public record Synergy(double Damage,double Crit,double Reduction,double Speed);
public static class Rules
{
    public static Dictionary<string,int> Counts(IEnumerable<Unit> team)=>team.Select(u=>u.Origin??u).Distinct().SelectMany(u=>u.Archetypes.Distinct()).GroupBy(x=>x).ToDictionary(x=>x.Key,x=>x.Count());
    public static Synergy Fighters(IEnumerable<Unit> team,Unit u)
    {
        var n=Counts(team).GetValueOrDefault("fighter");var member=u.Archetypes.Contains("fighter");
        return new(n>=2?.10:0,n>=3&&member?.30:0,n>=4&&member?.12:0,n>=5&&member?.15:0);
    }
    public static double Hit(Unit target,double raw,double reduction=0,double ignoreDefense=0)=>target.LoseHp(Math.Max(0,raw)*100/(100+target.Defense*(1-ignoreDefense))*(1-reduction)*(1+Math.Clamp(target.Statuses.Sum(s=>s.Effect.IncomingDamage*s.Magnitude),-.8,2)));
    public static double[] Horizon(Unit caster,IEnumerable<Unit> targets)
    {
        double carry=0;var result=new List<double>();
        foreach(var t in targets.Where(t=>t.Alive).OrderBy(t=>t.Layer).Take(3))
        {carry=Hit(t,1.5*caster.Essence+carry,t.ProtectionFrom(caster));result.Add(carry);}
        return result.ToArray();
    }
}
