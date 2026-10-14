namespace Vaelorn;

// An encounter owns all three waves and its single reward claim.
public sealed class TrainingRun
{
    internal Game Owner{get;}
    public string BossId{get;}
    public int Wave{get;private set;}=1;
    public Battle Current{get;private set;}
    public bool RewardClaimed{get;private set;}
    public bool Won=>Wave==3&&Current.Won;
    public bool Finished=>Current.Finished&&(!Current.Won||Wave==3);
    private readonly Boss boss;
    private readonly Catalog catalog;
    private readonly List<Unit> allies;
    internal TrainingRun(Game owner,string bossId,IEnumerable<Unit> team)
    {
        Owner=owner;BossId=bossId;catalog=owner.Catalog;boss=catalog.Bosses.Single(b=>b.Id==bossId);allies=team.ToList();
        Current=CreateBattle(0,true);
    }
    private Battle CreateBattle(int round,bool initialize)
    {
        var enemies=Wave==3?new List<Unit>{new(catalog.Hero(boss.CharacterId),boss.Stage,true)}:
            Enumerable.Range(0,Wave==1?3:5).Select(i=>new Unit(catalog.Hero("enemy_fenda"),boss.Stage,false,i%3,i/3)).ToList();
        return new Battle(catalog,allies,enemies,42+Wave,round,initialize);
    }
    public bool NextWave()
    {
        if(!Current.Won||Wave>=3)return false;
        var round=Current.Round;Wave++;Current=CreateBattle(round,false);return true;
    }
    public bool ClaimReward()
    {
        if(!Won||RewardClaimed)return false;
        if(!Owner.TrainingReward(BossId,true))return false;
        RewardClaimed=true;return true;
    }
}
