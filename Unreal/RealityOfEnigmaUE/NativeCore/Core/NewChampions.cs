namespace Vaelorn;

public sealed record BattleHazard(Unit Source,int Cell,int ExpiresRound,string Kind,double Attack,int Condemnation=0);
public partial class Unit
{
    public int CurrentCombatRound {get;internal set;}
    public int DivineFocus {get;internal set;}
    public int CelestialEnergy {get;internal set;}
    public double ActionAdvance {get;internal set;}
    public Unit? Summoner {get;internal set;}
    public int SummonedRound {get;internal set;}
    public double BleedResistance=>Math.Clamp(Ascended?Definition.Eclipse?.AscendedBleedResistance??Definition.BleedResistance:Definition.BleedResistance,0,.9);
    public double PoisonResistance=>Crimson?Definition.Floral?.CrimsonPoisonResistance??Definition.PoisonResistance:Definition.PoisonResistance;
    public double MagicResistance=>Math.Clamp(Definition.MagicResistance-Statuses.Sum(s=>s.Effect.MagicResistanceReduction*s.Magnitude),-.8,.8);
    public bool RemoveOneNegativeEffect(){
        if(Stun>0){Stun=0;PendingWakeDebuff=false;return true;}
        if(Freeze>0){Freeze=0;Ice=0;return true;}
        if(TechniqueSlowTurns>0){TechniqueSlowTurns=0;TechniqueSlow=0;return true;}
        if(Ice>0){Ice=0;return true;}
        var negative=Statuses.FirstOrDefault(s=>s.Effect.SourceStrengthDamage>0||s.Effect.TargetHpDamage>0||s.Effect.DamageReduction>0||s.Effect.DefenseReduction>0||s.Effect.HealReduction>0||s.Effect.SourceVulnerability>0||s.Effect.IncomingDamage>0||s.Effect.Sleep||s.Effect.MagicResistanceReduction>0||s.Effect.ControlPenalty>0||s.Effect.ControlReduction>0);
        if(negative==null)return false;Statuses.RemoveAll(s=>s.Effect.Id==negative.Effect.Id);return true;
    }
}
public partial class Battle
{
    public List<BattleHazard> Hazards {get;}=new();
    private int repositionCell;
    private readonly HashSet<CombatImpact> processedReflections=new();
    private readonly Dictionary<Guid,int> championAscensionRound=new();
    private readonly HashSet<(Guid Boss,Guid Victim)> condemnedDefeats=new();
    private readonly HashSet<(Guid Hero,Guid Victim)> markedDefeats=new();
    public bool TryRepositionActive(int cell,out string error){
        error="";var actor=NextActor;
        if(!FormationLocked){error="Inicie o combate";return false;}
        if(actor==null||!Allies.Contains(actor)){error="Aguarde a vez de um herói";return false;}
        if(cell<0||cell>8||cell==actor.Layer*3+actor.Column||Allies.Any(u=>u.Alive&&u!=actor&&u.Layer*3+u.Column==cell)){error="Escolha uma casa livre";return false;}
        if(actor.Stacks("mazakin_chains")>0){error="Correntes do Submundo impedem o movimento";return false;}
        var move=new Skill("battle_reposition","Reposicionar","reposition","strength",0,0);
        error=UnavailableReason(actor,move);if(error.Length>0)return false;
        repositionCell=cell;AdvanceInternal(move,actor);return true;
    }
    private void UpdateDivineFocus(){
        foreach(var hero in Allies.Concat(Enemies).Where(u=>u.Id=="tenrin")){
            var foes=Targets(Allies.Contains(hero)?Enemies:Allies);
            hero.DivineFocus=Math.Min(5,foes.Sum(t=>t.Statuses.Count(s=>s.Effect.Id=="divine_mark"&&s.SourceInstance==hero.InstanceId)));
        }
    }
    private void ChampionHeal(Unit unit,double fraction,Unit source){
        if(!unit.Alive)return;double amount=Math.Min(unit.MaxHp-unit.Hp,unit.MaxHp*fraction*(1-unit.StatusReduction(e=>e.HealReduction)));
        unit.Hp+=amount;if(amount>0)LastImpacts.Add(new(unit,amount,"heal",false,0,source));
    }
    private void SyncChampionPhases(){
        foreach(var u in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Ascended&&(u.Id=="mazakin"||u.Id=="hajun")))if(championAscensionRound.TryAdd(u.InstanceId,Round)){
            foreach(var skill in SkillsFor(u).Where(s=>s.MinimumPhase==2))u.ReadyRound[skill.Id]=Round+skill.InitialCooldown;
            if(u.Id=="hajun")KitStatus(u,u,"hajun_regeneration");
            Log.Add(u.Id=="mazakin"?"MAZAKIN: Rei do Abismo — o corcel desperta aos 20%!":"HAJUN: Ascensão Absoluta aos 50%!");
        }
    }
    private void StartChampionRound(){
        SyncChampionPhases();UpdateChampionAuras();
        foreach(var hajun in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Id=="hajun"&&u.Ascended))ChampionHeal(hajun,.03*Math.Min(3,hajun.Stacks("hajun_regeneration")),hajun);
    }
    private void UpdateChampionAuras(){
        foreach(var unit in AlliedTargets.Concat(EnemyTargets))unit.Statuses.RemoveAll(s=>s.Effect.Id=="abyss_aura");
        foreach(var boss in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Id=="mazakin"&&u.Ascended))foreach(var target in Targets(Allies.Contains(boss)?Enemies:Allies))KitStatus(target,boss,"abyss_aura");
    }
    private void ApplyCondemnation(Unit target,Unit source,int count=1){for(int i=0;i<count;i++)KitStatus(target,source,"condemnation");}
    private void ResolveChampionReflections(){
        foreach(var hit in LastImpacts.ToArray().Where(h=>h.Amount>0&&h.Attacker!=null&&(h.Kind=="physical"||h.Kind=="essence"||h.Kind=="true"||h.Kind=="hazard"))){
            if(!processedReflections.Add(hit))continue;double ratio=hit.Target.Statuses.Sum(s=>s.Effect.Reflection*s.Magnitude);if(ratio<=0||!hit.Attacker!.Alive)continue;
            double amount=hit.Attacker.LoseHp(hit.Amount*Math.Min(.5,ratio));if(amount>0)LastImpacts.Add(new(hit.Attacker,amount,"reflect",false,hit.HitIndex+1,hit.Target,"celestial_reflect"));
        }
    }
    private void ResolveChampionDefeats(){
        foreach(var boss in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Id=="mazakin"))foreach(var fallen in (Allies.Contains(boss)?Enemies:Allies).Where(u=>!u.Alive&&u.Stacks("condemnation")>0))if(condemnedDefeats.Add((boss.InstanceId,fallen.InstanceId))){
            boss.ApplyBuff("condemned_soul_"+fallen.InstanceId,new(.1,0,0,Round+2));LastBuffTargets.Add(boss);Log.Add("Alma Condenada: Mazakin recebe +10% de ATK por duas rodadas.");
        }
        foreach(var hero in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Id=="tenrin"))foreach(var fallen in (Allies.Contains(hero)?Enemies:Allies).Where(u=>!u.Alive&&u.Stacks("divine_mark")>=5))if(markedDefeats.Add((hero.InstanceId,fallen.InstanceId)))ChampionHeal(hero,.3,hero);
        foreach(var orb in AlliedTargets.Concat(EnemyTargets).Where(u=>u.Id=="mazakin_orb"&&u.Alive&&u.Summoner is {Alive:false}).ToArray())orb.LoseHp(orb.MaxHp);
        UpdateChampionAuras();
    }
    private void ResolveChampionHazards(){
        foreach(var source in Allies.Concat(Enemies).Where(u=>u.Id=="mazakin"&&u.Alive).ToArray())foreach(var orb in (Allies.Contains(source)?Allies:Enemies).Where(u=>u.Id=="mazakin_orb"&&u.Alive&&u.Summoner==source&&Round>u.SummonedRound&&(Round-u.SummonedRound)%2==1).ToArray()){
            int index=0;foreach(var foe in Targets(Allies.Contains(source)?Enemies:Allies).ToArray())MazakinHit(source,foe,2,true,index++,effect:"mazakin_orb");
        }
        foreach(var hazard in Hazards.Where(h=>h.Source.Alive&&h.ExpiresRound>=Round).ToArray()){
            foreach(var target in Targets(Allies.Contains(hazard.Source)?Enemies:Allies).Where(t=>t.Layer*3+t.Column==hazard.Cell).ToArray()){
                double amount=Rules.Hit(target,hazard.Source.Strength*hazard.Attack,target.ProtectionFrom(hazard.Source),magical:true);
                LastImpacts.Add(new(target,amount,"hazard",false,0,hazard.Source,"mazakin_"+hazard.Kind));
                if(hazard.Condemnation>0){ApplyCondemnation(target,hazard.Source,hazard.Condemnation);foreach(var mark in target.Statuses.Where(s=>s.Effect.Id=="condemnation"))mark.RemainingTurns+=2;}
                if(!target.Alive&&hazard.Kind=="ritual")ChampionHeal(hazard.Source,.15,hazard.Source);
            }
        }
        Hazards.RemoveAll(h=>!h.Source.Alive||h.ExpiresRound<=Round);
        foreach(var boss in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Id=="mazakin"&&u.Ascended))if(championAscensionRound.TryGetValue(boss.InstanceId,out var start)&&(Round-start)>0&&(Round-start)%2==0)foreach(var target in Targets(Allies.Contains(boss)?Enemies:Allies))ApplyCondemnation(target,boss);
    }
    private void MazakinHit(Unit u,Unit t,double multiplier,bool magical,int index,double ignore=0,string effect=""){
        bool crit=rng.NextDouble()<u.CriticalChance;double raw=u.Strength*multiplier*u.OutgoingDamage*(crit?u.CriticalMultiplier:1);
        double damage=Rules.Hit(t,raw,1-(1-Rules.Fighters(Allies.Contains(t)?Allies:Enemies,t).Reduction)*(1-t.ProtectionFrom(u)),ignore,magical);
        LastImpacts.Add(new(t,damage,magical?"essence":"physical",crit,index,u,effect));if(damage>0)ApplyCondemnation(t,u);
    }
    private void ActMazakin(Unit u,Unit target,List<Unit> own,List<Unit> foes,Skill s){
        if(s.Id=="mazakin_orb"){
            own.RemoveAll(t=>t.Id=="mazakin_orb"&&!t.Alive&&t.Summoner==u);
            for(int i=own.Count(t=>t.Id=="mazakin_orb"&&t.Alive&&t.Summoner==u);i<3;i++)own.Add(new Unit(catalog.Hero("mazakin_orb"),u.Level,column:i,layer:2){Summoner=u,SummonedRound=Round});
            LastBuffTargets.AddRange(own.Where(t=>t.Id=="mazakin_orb"&&t.Alive&&t.Summoner==u));return;
        }
        if(s.Id=="mazakin_chains"){
            var effect=catalog.StatusEffects.Single(e=>e.Id=="mazakin_chains");target.AddStatus(effect,u);target.Statuses.Last(s=>s.Effect.Id==effect.Id).Magnitude=target.Stacks("condemnation")>=5?1.5:1;
            target.ApplySlow(.4,2);LastBuffTargets.Add(target);return;
        }
        if(s.Id=="mazakin_ritual"){
            Hazards.RemoveAll(h=>h.Source==u&&h.Kind=="ritual");Hazards.Add(new(u,target.Layer*3+target.Column,Round+2,"ritual",2.5,2));LastBuffTargets.Add(target);return;
        }
        var victims=s.Id=="mazakin_charge"?Targets(foes).Where(t=>t.Column==target.Column).ToArray():s.Id is "mazakin_threat" or "mazakin_judgment"?Targets(foes).ToArray():new[]{target};
        int hit=0;foreach(var victim in victims){bool maxCondemnation=victim.Stacks("condemnation")>=6;MazakinHit(u,victim,s.Multiplier,s.Id=="mazakin_threat",hit++,s.Id=="mazakin_judgment"&&maxCondemnation?.8:0);
            if(s.Id=="mazakin_charge"&&rng.NextDouble()>=victim.ControlResistance)victim.ApplyStun(1);
            if(s.Id=="mazakin_threat"){
                ApplyCondemnation(victim,u);KitStatus(victim,u,"abyss_threat");victim.Reposition(victim.Layer*3+1);
            }
            if(s.Id=="mazakin_judgment"&&!Hazards.Any(h=>h.Source==u&&h.Kind=="flames"&&h.Cell==victim.Layer*3+victim.Column))Hazards.Add(new(u,victim.Layer*3+victim.Column,Round+1,"flames",1.2));
        }
    }
    private void DivineMark(Unit u,Unit t,int count,bool celestial,int hit){
        string id=celestial?"celestial_mark":"divine_mark";int prior=t.Stacks(id);for(int i=0;i<count;i++)KitStatus(t,u,id);
        if(prior>=5)foreach(var mark in t.Statuses.Where(s=>s.Effect.Id==id))mark.RemainingTurns=Math.Max(mark.RemainingTurns,4);
        if(celestial&&prior<5&&t.Stacks(id)>=5&&t.Alive){
            double amount=Rules.Hit(t,1.5*u.Strength*u.OutgoingDamage,t.ProtectionFrom(u),.3,true);LastImpacts.Add(new(t,amount,"essence",false,hit,u,"zevalon_lightning"));
            if(rng.NextDouble()>=t.ControlResistance)t.ApplyStun(1);ChampionHeal(u,.1,u);
        }
        UpdateDivineFocus();
    }
    private bool DivineDamage(Unit u,Unit t,double multiplier,int index,bool trueDamage=false,double ignore=0){
        bool crit=rng.NextDouble()<u.CriticalChance+Rules.Fighters(Allies.Contains(u)?Allies:Enemies,u).Crit;
        double raw=u.Strength*multiplier*u.OutgoingDamage*(crit?u.CriticalMultiplier:1);
        double amount=trueDamage?t.LoseHp(raw):Rules.Hit(t,raw,1-(1-Rules.Fighters(Allies.Contains(t)?Allies:Enemies,t).Reduction)*(1-t.ProtectionFrom(u)),ignore,true);
        if(trueDamage&&amount>0&&t.Id=="hajun")t.RegisterHit(Round);
        LastImpacts.Add(new(t,amount,trueDamage?"true":"essence",crit,index,u));return amount>0;
    }
    private void ActDivine(Unit u,Unit target,List<Unit> own,List<Unit> foes,Skill s){
        bool celestial=u.Id=="zevalon";string mark=celestial?"celestial_mark":"divine_mark";
        if(s.Operator=="divine_guard"){
            foreach(var ally in Targets(own)){KitStatus(ally,u,celestial?"celestial_guard":"divine_guard");ally.RemoveOneNegativeEffect();LastBuffTargets.Add(ally);}
            if(celestial)foreach(var foe in Targets(foes))DivineMark(u,foe,1,true,0);
            else foreach(var foe in Targets(foes).Where(t=>t.Column==u.Column))KitStatus(foe,u,"divine_column");
            if(celestial)u.CelestialEnergy=Math.Min(5,u.CelestialEnergy+1);return;
        }
        bool empowered=celestial&&u.CelestialEnergy>=5;if(empowered)u.CelestialEnergy=0;
        bool aoe=s.Id.EndsWith("judgment")||s.Id.EndsWith("roar")||s.Id.EndsWith("wrath");bool killed=false;int index=0;
        foreach(var foe in (aoe?Targets(foes).ToArray():new[]{target})){
            int prior=foe.Stacks(mark);double multiplier=s.Multiplier*(celestial&&s.Id.EndsWith("judgment")&&prior>=5?1.4:1);
            if(DivineDamage(u,foe,multiplier,index++,empowered,celestial&&s.Id.EndsWith("judgment")&&prior>=5?.2:0)){
                int marks=s.Id.EndsWith("judgment")?3:s.Id.EndsWith("roar")||s.Id.EndsWith("wrath")?5:1;
                DivineMark(u,foe,marks,celestial,index++);
                if(s.Id.EndsWith("lion")&&rng.NextDouble()<.7)DivineMark(u,foe,celestial?2:1,celestial,index++);
            }
            if(s.Id.EndsWith("lion")&&prior>=(celestial?5:3)){
                if(celestial)foreach(var other in Targets(foes).ToArray())DivineDamage(u,other,1.2,index++);
                else if(foe.Alive)DivineDamage(u,foe,1.2,index++);
            }
            if(s.Id.EndsWith("judgment")){foe.ApplySlow(.2,2);if(!celestial&&foe.Stacks(mark)>=5)KitStatus(foe,u,"divine_judgment");}
            if(s.Id.EndsWith("roar")||s.Id.EndsWith("wrath")){KitStatus(foe,u,"divine_roar");if(celestial&&rng.NextDouble()>=foe.ControlResistance)foe.ApplyStun(1);}
            if(!foe.Alive)killed=true;
        }
        if(s.Id.EndsWith("roar"))foreach(var ally in Targets(own)){ally.ApplyBuff(s.Id,new(0,0,.25,Round+2));LastBuffTargets.Add(ally);}
        if(s.Id.EndsWith("wrath")&&killed){ChampionHeal(u,.2,u);u.ActionAdvance=.3;}
        if(celestial&&s.Cooldown>0)u.CelestialEnergy=Math.Min(5,u.CelestialEnergy+1);
    }
}
