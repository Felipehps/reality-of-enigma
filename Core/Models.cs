using System.Text.Json;
namespace Vaelorn;

public record Stats(double Hp, double Strength, double Essence, double Defense, double Speed);
public record FloralKit(int PoisonMax=5,double CrimsonThreshold=.5,double CrimsonDamage=.2,double CrimsonSpeed=.15,double BloomThreshold=.25,double BloomHp=.02,double SleepChance=.35,double PetalsHp=.08,double PetalsExecuteThreshold=.4,double PetalsExecuteBonus=.3,int ThornStackThreshold=3,double ThornStackBonus=.4,int ThornPoison=3,int CrimsonPoison=1);
public record EclipseKit(double Threshold=.5,double Strength=1.4,double Defense=1.333333333,double Speed=1.222222222,int StackCap=5,int AscendedCap=7,double CritPerStack=.05,double AscendedCritPerStack=.06,double CritDamagePerStack=.08,double AscendedCritDamagePerStack=.10,string[]? AscendedSkills=null,double DotResistance=0);
public record ScarletKit(double FrontProtection=.25,double HealthyThreshold=.7,double SpeedPerBleed=.05,double SpeedCap=.25,double TargetSpeedPerBleed=.08,double TargetCritPerBleed=.06,int StackCap=5,double KillHeal=.15,int KillCooldown=1);
public record Character(string Id, string Name, string Role, string Scale, string[] Archetypes, Stats Stats, string[] Skills, bool StoryProtected=false, string? AttackStatus=null, double AttackStatusChance=0, FloralKit? Floral=null,double CritChance=.05,double ControlResistance=0,double PoisonResistance=0,double CritMultiplier=1.5,EclipseKit? Eclipse=null,ScarletKit? Scarlet=null);
public record Skill(string Id, string Name, string Operator, string Scale, double Multiplier, int Cooldown, int InitialCooldown=0, int Ice=0, int Stun=0, double Slow=0, int Duration=0, int Hits=1, int MinimumGate=0, int MinimumLevel=1, double StatusChanceMultiplier=1, string? DamageStackStatus=null, double DamagePerStack=0, string? ApplyStatus=null, double MinimumHpRatio=0, int UsesPerBattle=0, double BodyHpShare=.6, double SpiritHpShare=.4, double CopyDamage=.6, double ReturnHpRatio=.3,double TargetHpDamage=0,bool FrontOnly=false,string? Description=null,int MinimumPhase=1);
public record StatusEffect(string Id, int MaxStacks, int Duration, double SourceStrengthDamage=0, string? ChanceBonusStatus=null, double ChanceBonus=0,double TargetHpDamage=0,double DamageReduction=0,double DefenseReduction=0,double HealReduction=0,double SourceVulnerability=0,bool Sleep=false,string? Family=null,double IncomingDamage=0,double DefenseBonus=0,double ControlBonus=0,double LifeSteal=0,double Regeneration=0);
public record Enemy(string CharacterId, int Level, bool IsBoss=false);
public record Stage(int Number, Enemy[] Enemies, int Gold, string? BossId=null, int Xp=500);
public record Boss(string Id, string CharacterId, int Stage, bool Recruitable, bool RequiresActComplete, int FragmentCost);
public record Equipment(string Id, string Slot, string[] AllowedArchetypes, Stats Bonus);
public record Faction(string Id, string Name);
public record Archetype(string Id, string Name, string Icon);
public record Summon(string[] Pool, int[] Weights, int SingleCost, int TenCost);
public record Catalog(Character[] Characters, Skill[] Skills, StatusEffect[] StatusEffects, Enemy[] Enemies, Stage[] Stages, Boss[] Bosses, Equipment[] Equipment, Faction[] Factions, Archetype[] Archetypes, Summon Summon)
{
    public Character Hero(string id)=>Characters.Single(x=>x.Id==id);
    public Skill Ability(string id)=>Skills.Single(x=>x.Id==id);
    public static Catalog Load(string json)
    {
        var c=JsonSerializer.Deserialize<Catalog>(json,new JsonSerializerOptions{PropertyNameCaseInsensitive=true}) ?? throw new InvalidDataException("Catálogo vazio");
        if(c.Characters.Select(x=>x.Id).Distinct().Count()!=c.Characters.Length) throw new InvalidDataException("ID duplicado");
        foreach(var h in c.Characters)
        {
            if(h.Archetypes.Distinct().Count()!=h.Archetypes.Length || h.Archetypes.Length is <1 or >3 || h.Stats.Hp<=0) throw new InvalidDataException(h.Id);
            foreach(var s in h.Skills)c.Ability(s);
            foreach(var a in h.Archetypes)if(!c.Archetypes.Any(x=>x.Id==a))throw new InvalidDataException(a);
        }
        if(c.Summon.Pool.Length==0 || c.Summon.Pool.Length!=c.Summon.Weights.Length || c.Summon.Weights.Any(x=>x<=0))throw new InvalidDataException("Pesos");
        foreach(var id in c.Summon.Pool)
        {c.Hero(id);if(c.Bosses.Any(x=>x.CharacterId==id))throw new InvalidDataException("Boss no gacha");}
        foreach(var s in c.Stages)foreach(var e in s.Enemies)c.Hero(e.CharacterId);
        return c;
    }
}
public class OwnedCharacter
{
    public string Id{get;set;}="";
    public int Level{get;set;}=1;
    public int Stars{get;set;}=1;
    public int Copies{get;set;}
    public decimal BattleXp{get;set;}
    public string Path{get;set;}="base";
}
public class Inventory
{
    public decimal Gold{get;set;}=10000;
    public decimal Xp{get;set;}=5000;
    public int Jokers{get;set;}
    public int Tickets{get;set;}=3;
    public Dictionary<string,int> Fragments{get;set;}=new();
}
public class SaveData
{
    public int Version{get;set;}=1;
    public int CompletedStage{get;set;}
    public bool ActComplete{get;set;}
    public bool OpeningSeen{get;set;}
    public bool ForestEncounterSeen{get;set;}
    public Inventory Inventory{get;set;}=new();
    public List<OwnedCharacter> Roster{get;set;}=new();
    public List<OwnedCharacter> CampaignGuests{get;set;}=new();
    public List<int> FormationCells{get;set;}=new(){0,1,3,4,6};
    public List<string> Team{get;set;}=new(){"kael","savor","aelia","brakk","lyra"};
    public HashSet<string> Training{get;set;}=new();
    public HashSet<string> TrainingWins{get;set;}=new();
    public DateTimeOffset IdleAt{get;set;}=DateTimeOffset.UtcNow;
    public decimal PendingGold{get;set;}
    public decimal PendingXp{get;set;}
}

