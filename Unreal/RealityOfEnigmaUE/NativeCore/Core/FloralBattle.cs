namespace Vaelorn;

public partial class Battle
{
    private void FlowerStatus(Unit target,Unit source,string id,int count=1){
        var effect=catalog.StatusEffects.Single(e=>e.Id==id);
        for(int n=0;n<count;n++){
            if(id=="poison"&&rng.NextDouble()<target.Definition.PoisonResistance)continue;
            if(!target.AddStatus(effect,source))continue;
            var added=target.Statuses.Last();
            if(target.Boss||target.StunImmune){added.Magnitude=.5;if(effect.Sleep)added.RemainingTurns=1;}
        }
    }
    private void ActFloral(Unit u,Unit target,List<Unit> foes,Skill skill){
        var kit=u.Definition.Floral!;
        var targets=skill.Operator=="floral_thorns"||skill.Operator=="floral_basic"?new[]{target}:Targets(foes).ToArray();
        int hit=0;
        foreach(var t in targets.Where(x=>x.Alive)){
            if(skill.Operator=="floral_sleep"){
                if(rng.NextDouble()<kit.SleepChance*(1-t.ControlResistance)){
                    FlowerStatus(t,u,"flower_sleep");Log.Add(t.CombatName+": Sono da Flor do Paraíso");
                }
                continue;
            }
            double raw=u.Strength*skill.Multiplier;
            if(skill.Operator=="floral_petals"){raw+=t.MaxHp*kit.PetalsHp;if(t.Hp/t.MaxHp<kit.PetalsExecuteThreshold)raw*=1+kit.PetalsExecuteBonus;}
            if(skill.Operator=="floral_thorns"&&t.Stacks("poison")>=kit.ThornStackThreshold)raw*=1+kit.ThornStackBonus;
            bool critical=rng.NextDouble()<u.CriticalChance;
            double damage=Rules.Hit(t,raw*u.OutgoingDamage*(1+t.VulnerabilityFrom(u))*(critical?u.CriticalMultiplier:1),t.ProtectionFrom(u));
            LastImpacts.Add(new(t,damage,"essence",critical,hit++));
            if(damage<=0)continue;
            FlowerStatus(t,u,"poison",1+(u.Crimson?kit.CrimsonPoison:0)+(skill.Operator=="floral_thorns"?kit.ThornPoison:0));
            if(t.Stacks("poison")>=kit.PoisonMax)FlowerStatus(t,u,"deep_poison");
            if(skill.Operator=="floral_petals"){
                FlowerStatus(t,u,"venom_burn");
                FlowerStatus(t,u,"venom_suppression");
                if(u.Crimson)FlowerStatus(t,u,"crimson_suppression");
            }
            if(skill.Operator=="floral_thorns")FlowerStatus(t,u,"thorn_wound");
            Log.Add($"{u.CombatName} > {t.CombatName}: {skill.Name} {damage:F0}");
        }
    }
    private void ApplyBloom(){
        foreach(var source in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Blooming).ToArray()){
            var foes=Allies.Contains(source)?Enemies:Allies;
            foreach(var target in Targets(foes)){
                double damage=target.LoseHp(target.MaxHp*source.Definition.Floral!.BloomHp*(target.Boss||target.StunImmune ? .5 : 1));
                LastImpacts.Add(new(target,damage,"poison",false,0));
                Log.Add($"Florescimento > {target.CombatName}: {damage:F0} veneno");
            }
        }
    }
}
