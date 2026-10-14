namespace Vaelorn;

// Each independent stack owns its source snapshot and its own victim-turn lifetime.
public sealed class StatusStack
{
    public StatusEffect Effect{get;}
    public Guid SourceInstance{get;}
    public string SourceCharacter{get;}
    public double SourceStrength{get;}
    public int RemainingTurns{get;set;}
    public double Magnitude{get;set;}=1;
    public StatusStack(StatusEffect effect,Unit source){Effect=effect;var owner=source.Origin??source;SourceInstance=owner.InstanceId;SourceCharacter=owner.Id;SourceStrength=owner.Strength;RemainingTurns=effect.Duration;}
}
public sealed record LinkedForm(Unit Spirit,double CopyDamage,double ReturnHpRatio);
public partial class Unit
{
    private double BodyMaxShare=1,FormMaxShare=1;
    public Guid InstanceId{get;}=Guid.NewGuid();
    public Unit? Origin{get;private set;}
    public LinkedForm? Duality{get;private set;}
    public List<StatusStack> Statuses{get;}=new();
    public Dictionary<string,int> SkillUses{get;}=new();
    public double StatusReduction(Func<StatusEffect,double> selector)=>Math.Clamp(Statuses.Sum(s=>selector(s.Effect)*s.Magnitude),0,.8);
    public double VulnerabilityFrom(Unit source)=>Statuses.Where(s=>s.SourceInstance==(source.Origin??source).InstanceId).Sum(s=>s.Effect.SourceVulnerability*s.Magnitude);
    public IEnumerable<Unit> TargetForms=>Duality is {} split?new[]{this,split.Spirit}:new[]{this};
    public string CombatName=>Definition.Name+(Origin!=null?" • Espírito":Duality!=null?" • Corpo":"");
    public int Stacks(string id)=>Statuses.Count(s=>s.Effect.Id==id||(id=="bleed"&&s.Effect.Family=="bleed"));
    public double IncomingStatusChance(string id,double chance)=>Math.Clamp(chance+Statuses.Where(s=>s.Effect.ChanceBonusStatus==id).Sum(s=>s.Effect.ChanceBonus),0,1);
    public bool AddStatus(StatusEffect effect,Unit source){
        if(!Alive)return false;
        if(effect.Id.StartsWith("scarlet_bleed")){
            var family=Statuses.Where(s=>s.Effect.Id.StartsWith("scarlet_bleed")&&s.SourceInstance==(source.Origin??source).InstanceId).ToArray();
            if(family.Length>=5){
                // Upgrade/refresh the oldest charge instead of growing beyond this kit's cap.
                var oldest=family.OrderBy(s=>s.RemainingTurns).First();Statuses.Remove(oldest);
            }
        }
        if(Stacks(effect.Id)>=effect.MaxStacks){
            if(effect.MaxStacks==1){Statuses.RemoveAll(s=>s.Effect.Id==effect.Id);}else return false;
        }
        Statuses.Add(new(effect,source));return true;
    }
    public bool Split(Skill skill){
        if(Origin!=null||Duality!=null||!Alive||Hp/MaxHp<skill.MinimumHpRatio||skill.UsesPerBattle>0&&SkillUses.GetValueOrDefault(skill.Id)>=skill.UsesPerBattle)return false;
        double current=Hp;var spirit=new Unit(Definition,Level,Boss,Column,Layer,Path){Origin=this,FormMaxShare=skill.SpiritHpShare,Hp=current*skill.SpiritHpShare};
        BodyMaxShare=skill.BodyHpShare;Hp=current*skill.BodyHpShare;Duality=new(spirit,skill.CopyDamage,skill.ReturnHpRatio);SkillUses[skill.Id]=SkillUses.GetValueOrDefault(skill.Id)+1;return true;
    }
    public void EndDuality(){
        if(Duality is not {} split)return;
        Duality=null;
        foreach(var stack in split.Spirit.Statuses)if(Stacks(stack.Effect.Id)<stack.Effect.MaxStacks)Statuses.Add(stack);
        split.Spirit.Statuses.Clear();split.Spirit.Hp=0;Hp=OriginalMaxHp*split.ReturnHpRatio;
    }
    public List<(Unit Target,double Damage,string Kind)> TickDamageStatuses(){
        var results=new List<(Unit,double,string)>();
        foreach(var form in TargetForms.ToArray())foreach(var stack in form.Statuses.Where(s=>s.Effect.SourceStrengthDamage>0||s.Effect.TargetHpDamage>0).ToArray()){
            if(!form.Alive)break;
            double amount=form.LoseHp((stack.SourceStrength*stack.Effect.SourceStrengthDamage+form.MaxHp*stack.Effect.TargetHpDamage)*stack.Magnitude*form.DamageOverTimeMultiplier);results.Add((form,amount,(stack.Effect.Id=="bleed"||stack.Effect.Family=="bleed")?"bleed":"poison"));
            stack.RemainingTurns--;if(stack.RemainingTurns<=0){form.Statuses.Remove(stack);Statuses.Remove(stack);}
        }
        return results;
    }
    public void EndStatusTurn(){foreach(var form in TargetForms.ToArray())foreach(var stack in form.Statuses.Where(s=>s.Effect.SourceStrengthDamage==0&&s.Effect.TargetHpDamage==0).ToArray()){stack.RemainingTurns--;if(stack.RemainingTurns<=0)form.Statuses.Remove(stack);}}
}
