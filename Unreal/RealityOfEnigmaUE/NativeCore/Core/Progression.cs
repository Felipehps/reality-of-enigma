using System.Text.Json;
namespace Vaelorn;

public class Game
{
    public Catalog Catalog{get;}
    public SaveData Save{get;}
    private readonly Random rng;
    public string LastCampaignReward{get;private set;}="";
    public Game(Catalog catalog,SaveData save,int seed=17){Catalog=catalog;Save=save;rng=new(seed);}
    public static SaveData NewSave()=>new(){Roster=new(){new(){Id="kael",Stars=3},new(){Id="savor",Stars=3},new(){Id="aelia",Stars=3},new(){Id="brakk",Stars=3},new(){Id="lyra",Stars=2}}};
    public static int LevelCap(int stars)=>10+10*stars;
    public bool LevelUp(string id)
    {
        var c=Save.Roster.Single(x=>x.Id==id);var cost=c.Level*25;
        if(c.Level>=LevelCap(c.Stars)||Save.Inventory.Xp<cost)return false;
        Save.Inventory.Xp-=cost;c.Level++;return true;
    }
    public bool Promote(string id)
    {
        var c=Save.Roster.Single(x=>x.Id==id);
        if(c.Stars>=6||c.Level<LevelCap(c.Stars)||(c.Copies==0&&Save.Inventory.Jokers==0))return false;
        if(c.Copies>0)c.Copies--;else Save.Inventory.Jokers--;
        c.Stars++;return true;
    }
    public void GrantCopy(string id)
    {
        Catalog.Hero(id);var c=Save.Roster.SingleOrDefault(x=>x.Id==id);
        if(c==null){var guest=Save.CampaignGuests.SingleOrDefault(x=>x.Id==id);Save.Roster.Add(guest??new(){Id=id});if(guest!=null)Save.CampaignGuests.Remove(guest);}else if(c.Stars==6)Save.Inventory.Jokers++;else c.Copies++;
    }
    public string[] Summon(int count)
    {
        if(count is not(1 or 10))throw new ArgumentOutOfRangeException(nameof(count));
        var banner=Catalog.Summon;var cost=count==1?banner.SingleCost:banner.TenCost;
        if(Save.Inventory.Gold<cost)return Array.Empty<string>();
        var result=new List<string>();
        for(int n=0;n<count;n++)
        {var roll=rng.Next(banner.Weights.Sum());int i=0;while(roll>=banner.Weights[i])roll-=banner.Weights[i++];result.Add(banner.Pool[i]);}
        Save.Inventory.Gold-=cost;foreach(var id in result)GrantCopy(id);return result.ToArray();
    }
    public bool CompleteStage(int stage,bool won,DateTimeOffset now)
    {
        if(!won||stage!=Save.CompletedStage+1)return false;
        var def=Catalog.Stages.SingleOrDefault(x=>x.Number==stage);if(def==null)return false;
        Accrue(now);Save.CompletedStage=stage;Save.Inventory.Gold+=def.Gold;
        if(stage==70)Save.ActComplete=true;
        foreach(var boss in Catalog.Bosses.Where(x=>x.Stage==stage&&x.Recruitable&&(!x.RequiresActComplete||Save.ActComplete)))Save.Training.Add(boss.Id);
        return true;
    }
    public bool CanEnterStage(int stage)=>Catalog.Stages.Any(s=>s.Number==stage)&&stage<=Save.CompletedStage+1&&(stage>Save.CompletedStage||stage%10!=0);
    public bool RewardCampaignEncounter(int stage,bool won,DateTimeOffset now,IEnumerable<string>? participants=null)
    {
        LastCampaignReward="";
        if(!won||!CanEnterStage(stage))return false;
        var definition=Catalog.Stages.Single(s=>s.Number==stage);
        if(stage==Save.CompletedStage+1)CompleteStage(stage,true,now);
        else Save.Inventory.Gold+=definition.Gold;
        var team=(participants??Save.Team).Distinct().Select(id=>{
            var owned=Save.Roster.SingleOrDefault(x=>x.Id==id)??Save.CampaignGuests.SingleOrDefault(x=>x.Id==id);
            if(owned==null){Catalog.Hero(id);owned=new OwnedCharacter{Id=id,Stars=3};Save.CampaignGuests.Add(owned);}
            return owned;
        }).ToArray();
        decimal share=team.Length==0?0:(decimal)definition.Xp/team.Length;
        var lines=new List<string>();
        foreach(var member in team){
            int before=member.Level;member.BattleXp+=share;
            while(member.Level<LevelCap(member.Stars)&&member.BattleXp>=member.Level*25){member.BattleXp-=member.Level*25;member.Level++;}
            lines.Add(Catalog.Hero(member.Id).Name+" +"+share.ToString("0.##")+" XP"+(member.Level>before?$" • Nv.{before} → {member.Level}":""));
        }
        LastCampaignReward=$"+{definition.Gold:N0} Gold  •  {definition.Xp} XP da equipe\n"+string.Join("   |   ",lines);
        return true;
    }
    public bool TrainingReward(string bossId,bool won,bool simulate=false)
    {
        var b=Catalog.Bosses.Single(x=>x.Id==bossId);
        if(!won||!b.Recruitable||!Save.Training.Contains(bossId)||(b.RequiresActComplete&&!Save.ActComplete))return false;
        if(simulate&&(!Save.TrainingWins.Contains(bossId)||Save.Inventory.Tickets<=0))return false;
        if(simulate)Save.Inventory.Tickets--;
        Save.TrainingWins.Add(bossId);
        AddFragment(b.CharacterId);if(bossId=="varkhan")AddFragment(Catalog.Summon.Pool[rng.Next(Catalog.Summon.Pool.Length)]);
        Save.Inventory.Xp+=b.Stage*5;Save.Inventory.Gold+=b.Stage*2;return true;
    }
    public TrainingRun? BeginTraining(string bossId)
    {
        var boss=Catalog.Bosses.SingleOrDefault(b=>b.Id==bossId);
        if(boss==null||!boss.Recruitable||!Save.Training.Contains(bossId)||(boss.RequiresActComplete&&!Save.ActComplete))return null;
        var cells=Formation.Normalize(Save.FormationCells,Save.Team.Count);
        var team=Save.Team.Select(id=>Save.Roster.Single(o=>o.Id==id)).Select((o,i)=>new Unit(Catalog.Hero(o.Id),o.Level,false,cells[i]%3,cells[i]/3,o.Path));
        return new TrainingRun(this,bossId,team);
    }
    private void AddFragment(string id)=>Save.Inventory.Fragments[id]=Save.Inventory.Fragments.GetValueOrDefault(id)+1;
    public bool Reconstruct(string id)
    {
        Catalog.Hero(id);var cost=Catalog.Bosses.SingleOrDefault(x=>x.CharacterId==id)?.FragmentCost??10;
        if(Save.Inventory.Fragments.GetValueOrDefault(id)<cost)return false;
        Save.Inventory.Fragments[id]-=cost;GrantCopy(id);return true;
    }
    public void Accrue(DateTimeOffset now)
    {
        var minutes=(long)Math.Floor((now-Save.IdleAt).TotalMinutes);if(minutes<=0)return;
        Save.PendingGold+=minutes*(2m+Save.CompletedStage);
        Save.PendingXp+=minutes*(1m+Save.CompletedStage/2m);
        Save.IdleAt=Save.IdleAt.AddMinutes(minutes);
    }
    public void Collect(DateTimeOffset now)
    {Accrue(now);Save.Inventory.Gold+=Save.PendingGold;Save.Inventory.Xp+=Save.PendingXp;Save.PendingGold=0;Save.PendingXp=0;}
    public bool ChangePath(string path)
    {
        var kael=Save.Roster.Single(x=>x.Id=="kael");if(kael.Level<30||path is not("martial" or "arsenal"))return false;
        kael.Path=path;return true;
    }
    public bool CanEquip(OwnedCharacter owner,Equipment item)
    {var a=owner.Id=="kael"&&owner.Path=="martial"?new[]{"fighter"}:Catalog.Hero(owner.Id).Archetypes;return Save.ActComplete&&item.AllowedArchetypes.Intersect(a).Any();}
}
public static class Persistence
{
    public static SaveData Read(string path)
    {
        if(!File.Exists(path))return Game.NewSave();
        var s=JsonSerializer.Deserialize(File.ReadAllText(path),GameJsonContext.Default.SaveData)??throw new InvalidDataException("Save vazio");
        if(s.Version!=1||s.Inventory.Gold<0||s.Roster.Any(x=>x.Stars is <1 or >6||x.Level<1||x.Level>Game.LevelCap(x.Stars)))throw new InvalidDataException("Save incompatível");
        return s;
    }
    public static void Write(string path,SaveData save)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        var temp=path+".tmp";File.WriteAllText(temp,JsonSerializer.Serialize(save,GameJsonContext.Default.SaveData));
        if(File.Exists(path))File.Replace(temp,path,path+".bak");else File.Move(temp,path);
    }
}
