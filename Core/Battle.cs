namespace Vaelorn;

public record CombatImpact(Unit Target,double Amount,string Kind,bool Critical,int HitIndex,Unit? Attacker=null);

public partial class Battle
{
    public List<Unit> Allies{get;}
    public List<Unit> Enemies{get;}
    public List<Unit> AlliedTargets=>Allies.SelectMany(u=>u.TargetForms).ToList();
    public List<Unit> EnemyTargets=>Enemies.SelectMany(u=>u.TargetForms).ToList();
    public static List<Unit> Targets(IEnumerable<Unit> team)=>team.Select(u=>u.Origin??u).Distinct().SelectMany(u=>u.TargetForms).Where(u=>u.Alive).ToList();
    public List<string> Log{get;}=new();
    public List<CombatImpact> LastImpacts{get;}=new();
    public List<Unit> LastBuffTargets{get;}=new();
    private readonly Queue<Unit> turns=new();
    public Unit? LastActor{get;private set;}
    public Skill? LastSkill{get;private set;}
    public Unit? LastTarget{get;private set;}
    public string LastAction{get;private set;}="";
    public int Round{get;private set;}
    private readonly int firstRound;
    public bool Finished=>!Allies.Any(x=>x.Alive)||!Enemies.Any(x=>x.Alive)||(Round>=firstRound+100&&turns.Count==0);
    public bool Won=>Enemies.All(x=>!x.Alive)&&Allies.Any(x=>x.Alive);
    public int ActionRound=>turns.Count==0?Round+1:Round;
    public Unit? NextActor=>Finished?null:turns.Count>0?turns.Peek():Order().FirstOrDefault();
    public IReadOnlyList<Unit> TurnPreview=>Finished?Array.Empty<Unit>():(turns.Count>0?turns.Where(u=>u.Alive):Order()).Take(8).ToArray();
    private readonly Catalog catalog;
    private readonly Random rng;
    public Battle(Catalog catalog,IEnumerable<Unit> allies,IEnumerable<Unit> enemies,int seed=42,int initialRound=0,bool initializeAllies=true)
    {
        this.catalog=catalog;Allies=allies.ToList();Enemies=enemies.ToList();rng=new(seed);Round=firstRound=initialRound;
        foreach(var u in (initializeAllies?Allies:Enumerable.Empty<Unit>()).Concat(Enemies))
            foreach(var skill in SkillsFor(u))u.ReadyRound[skill.Id]=initialRound+1+skill.InitialCooldown;
        SyncAkurothPhases(initialRound);
    }
    private IEnumerable<Unit> Order(){UpdateScarletSpeed();return Allies.Concat(Enemies).Where(u=>u.Alive)
        .OrderByDescending(u=>u.Speed*(1+Rules.Fighters(Allies.Contains(u)?Allies:Enemies,u).Speed)).ThenBy(u=>u.Layer).ThenBy(u=>u.Column);}
    public Skill[] SkillsFor(Unit unit)
    {
        if(unit.Id=="kael"&&unit.Path=="martial")
            return new[]{"martial_basic",unit.Gate==8?"martial_extreme":"martial_break","martial_impact","martial_beast"}.Select(catalog.Ability).ToArray();
        return (unit.Ascended?unit.Definition.Eclipse?.AscendedSkills??unit.Definition.Skills:unit.Definition.Skills).Select(catalog.Ability).ToArray();
    }
    public string UnavailableReason(Unit actor,Skill skill)
    {
        if(!actor.Alive)return "Fora de combate";
        if(actor.Id=="akuroth"&&actor.AnnihilationCharging&&skill.Id!="akuroth_annihilation")return "Aniquilação em preparação";
        if(skill.MinimumPhase>(actor.Ascended?2:1))return "Requer segunda fase";
        if(skill.Cooldown>0&&actor.Stacks("akuroth_erasure")>0)return "Reescrita: habilidades especiais bloqueadas por uma ação";
        if(skill.FrontOnly&&actor.Layer!=0)return "Requer fileira da frente";
        if(skill.Operator=="split"&&(actor.Duality!=null||actor.Hp/actor.MaxHp<skill.MinimumHpRatio))return "Requer forma original e 70% de vida";
        if(skill.UsesPerBattle>0&&actor.SkillUses.GetValueOrDefault(skill.Id)>=skill.UsesPerBattle)return "Limite de usos nesta batalha";
        if(actor.Stun>0||actor.Freeze>0||actor.Statuses.Any(s=>s.Effect.Sleep))return "Controle ativo: avance a ação perdida";
        if(actor.Level<skill.MinimumLevel)return $"Requer nível {skill.MinimumLevel}";
        if(actor.Gate<skill.MinimumGate)return $"Requer estágio {skill.MinimumGate}";
        if(skill.Operator=="extreme"&&!actor.CanExtreme)return "Extremo já usado ou indisponível";
        int remaining=actor.ReadyRound.GetValueOrDefault(skill.Id,1+skill.InitialCooldown)-ActionRound;
        return remaining>0?$"Recarga: {remaining} rodada(s)":"";
    }
    public bool TryAct(string skillId,Unit? target,out string error)
    {
        error="";var actor=NextActor;
        if(actor==null){error="Combate encerrado";return false;}
        if(!Allies.Contains(actor)){error="É a vez do inimigo";return false;}
        var skill=SkillsFor(actor).SingleOrDefault(s=>s.Id==skillId);
        if(skill==null){error="Habilidade não pertence ao personagem";return false;}
        error=UnavailableReason(actor,skill);if(error!="")return false;
        if(skill.Operator is not ("break" or "buff" or "split" or "random" or "eclipse_guard"))
        {
            var allowed=skill.Operator=="heal"?AlliedTargets:EnemyTargets;
            if(target==null||!target.Alive||!allowed.Contains(target)){error=skill.Operator=="heal"?"Selecione um aliado vivo":"Selecione um inimigo vivo";return false;}
        }
        if(skill.Operator=="random")target=null;
        AdvanceInternal(skill,target);return true;
    }
    public bool FormationLocked{get;private set;}
    public void LockFormation()=>FormationLocked=true;
    public bool TryMoveAlly(Unit unit,int cell)
    {
        if(FormationLocked||Round!=0||LastActor!=null||!Allies.Contains(unit)||cell<0||cell>8)return false;
        var other=Allies.FirstOrDefault(u=>u!=unit&&u.Layer*3+u.Column==cell);
        int previous=unit.Layer*3+unit.Column;unit.Reposition(cell);other?.Reposition(previous);return true;
    }
    public void Step(){if(Finished)return;do{Advance();}while(turns.Count>0&&!Finished);}
    public void Advance()=>AdvanceInternal(null,null);
    private void AdvanceInternal(Skill? chosen,Unit? selected)
    {
        if(Finished)return;
        UpdateScarletSpeed();
        SyncAkurothPhases(Round);
        LockFormation();
        if(turns.Count==0){Round++;Log.Add($"Rodada {Round}");foreach(var u in Order())turns.Enqueue(u);}
        var actor=turns.Dequeue();LastActor=actor;LastImpacts.Clear();LastBuffTargets.Clear();LastAction="";LastSkill=null;LastTarget=null;
        if(actor.Alive)
        {
            foreach(var tick in actor.TickDamageStatuses()){LastImpacts.Add(new(tick.Target,tick.Damage,tick.Kind,false,0));Log.Add($"{(tick.Kind=="bleed"?"Sangramento":"Veneno")} > {tick.Target.CombatName}: {tick.Damage:F0}");}
            bool spiritCanAct=actor.Duality?.Spirit.StartOwnTurn()??true;
            if(!actor.Alive){LastAction="Sangramento";}
            else if(!actor.StartOwnTurn()){actor.AnnihilationCharging=false;LastAction=actor.SkippedBy;Log.Add($"{actor.Definition.Name}: perde ação por {actor.SkippedBy}");}
            else
            {
                var own=Allies.Contains(actor)?Allies:Enemies;var foes=Allies.Contains(actor)?Enemies:Allies;
                SyncAkurothPhases(Round);
                // Check against the current round after dequeue, never the next action's round.
                var available=SkillsFor(actor).Where(s=>(s.Cooldown==0||actor.Stacks("akuroth_erasure")==0)&&s.MinimumPhase<=actor.AkurothPhase&&(!s.FrontOnly||actor.Layer==0)&&actor.Level>=s.MinimumLevel&&actor.Gate>=s.MinimumGate&&actor.ReadyRound.GetValueOrDefault(s.Id,1+s.InitialCooldown)<=Round&&(s.Operator!="extreme"||actor.CanExtreme)&&(s.UsesPerBattle==0||actor.SkillUses.GetValueOrDefault(s.Id)<s.UsesPerBattle)&&(s.Operator!="split"||actor.Duality==null&&actor.Hp/actor.MaxHp>=s.MinimumHpRatio)).ToArray();
                var skill=chosen??(actor.Id=="akuroth"?ChooseAkurothSkill(actor,available):null)??available.FirstOrDefault(s=>s.Operator=="extreme")??available.LastOrDefault(s=>s.Cooldown>0)??available[0];
                var target=selected??(skill.Operator is "buff" or "split" or "eclipse_guard"?actor:skill.Operator=="heal"?own.Where(t=>t.Alive).OrderBy(t=>t.Hp/t.MaxHp).First():Targets(foes).OrderBy(t=>t.Layer).ThenBy(t=>t.Column).First());
                LastSkill=skill;LastTarget=target;LastAction=skill.Name;
                int stacks=skill.DamageStackStatus==null?0:target.Stacks(skill.DamageStackStatus);
                Act(actor,target,own,foes,skill,1,stacks);
                if(spiritCanAct&&actor.Duality is {} linked&&skill.Operator is not ("split" or "buff" or "heal" or "break"))Act(linked.Spirit,target,own,foes,skill,linked.CopyDamage,stacks);
                if(skill.Cooldown>0&&!actor.AnnihilationCharging)actor.ReadyRound[skill.Id]=Round+skill.Cooldown-actor.PendingCooldownRefund;
                actor.PendingCooldownRefund=0;
            }
            actor.Duality?.Spirit.EndOwnTurn();actor.EndOwnTurn();actor.EndStatusTurn();
        }
        AwardBleedingDefeats();
        SyncAkurothPhases(Round);
        // Dead units do not leave empty opportunities between the player's clicks.
        while(turns.Count>0&&!turns.Peek().Alive)turns.Dequeue();
        if(turns.Count==0){ApplyBloom();foreach(var u in Allies.Concat(Enemies))u.EndRound(Round);}
        if(Finished)Log.Add(Won?"Vitória":"Derrota ou limite de 100 rodadas");
    }
    private void Act(Unit u,Unit target,List<Unit> own,List<Unit> foes,Skill s,double damageScale=1,int damageStacks=0)
    {
        if(s.Id.StartsWith("akuroth_")){ActAkuroth(u,target,own,foes,s);return;}
        if(s.Operator.StartsWith("eclipse_")||s.Operator.StartsWith("scarlet_")){ActExtended(u,target,own,foes,s);return;}
        if(s.Operator.StartsWith("floral_")){ActFloral(u,target,foes,s);return;}
        if(s.Operator=="split"){u.Split(s);Log.Add("Corpo e Espírito despertaram");return;}
        if(s.Operator=="break"){u.BreakLimits();Log.Add($"Quebrar Limites: estágio {u.Gate}");return;}
        if(s.Operator=="buff")
        {
            var living=own.Where(t=>t.Alive).ToArray();
            int expires=Round+s.Duration-1;
            if(s.Scale=="speed")foreach(var ally in living){ally.ApplyBuff(s.Id,new(0,0,s.Multiplier,expires));LastBuffTargets.Add(ally);}
            else if(u.Strength>=living.Max(t=>t.Strength))foreach(var ally in living){ally.ApplyBuff(s.Id,new(s.Multiplier,s.Multiplier,0,expires));LastBuffTargets.Add(ally);}
            else{
                double MainStat(Unit t)=>t.Definition.Scale=="essence"?t.Essence:t.Strength;
                double maximum=living.Max(MainStat);
                var strongest=living.Where(t=>Math.Abs(MainStat(t)-maximum)<.0001).ToArray();
                var ally=strongest[rng.Next(strongest.Length)];bool essence=ally.Definition.Scale=="essence";
                ally.ApplyBuff(s.Id,new(essence?0:s.Multiplier,essence?s.Multiplier:0,0,expires));LastBuffTargets.Add(ally);
            }
            Log.Add($"{s.Name}: bônus por {s.Duration} rodadas > {string.Join(", ",LastBuffTargets.Select(t=>t.Definition.Name))}");return;
        }
        double stat=s.Scale=="essence"?u.Essence:u.Strength;
        if(s.Operator=="heal")
        {var amount=Math.Min(target.MaxHp-target.Hp,stat*s.Multiplier*(1-target.StatusReduction(e=>e.HealReduction)));target.Hp+=amount;LastImpacts.Add(new(target,amount,"heal",false,0));Log.Add($"{s.Name} > {target.Definition.Name}: cura {amount:F0}");return;}
        if(s.Operator=="horizon")
        {var line=foes.Where(t=>t.Column==target.Column&&t.Alive).OrderBy(t=>t.Layer).Take(3).ToArray();var hits=Rules.Horizon(u,line);
         for(int i=0;i<hits.Length;i++)LastImpacts.Add(new(line[i],hits[i],"essence",false,0));Log.Add($"Lança do Horizonte: {string.Join(" / ",hits.Select(d=>d.ToString("F0")))}");return;}
        foes=Targets(foes);
        var targets=s.Operator=="random"?Enumerable.Range(0,s.Hits).Select(_=>target).ToArray():s.Operator=="aoe"?foes.Where(t=>t.Alive).ToArray():s.Operator=="row"?foes.Where(t=>t.Alive&&t.Layer==target.Layer).ToArray():s.Operator=="line"?foes.Where(t=>t.Alive&&t.Column==target.Column).ToArray():new[]{target};
        int hitCount=s.Operator=="martial_basic"?(u.Gate>=7?3:u.Gate>=4?2:1):s.Operator=="random"?1:s.Hits;
        double multiplier=s.Operator switch
        {
            "martial_basic"=>u.Gate>=7?.55:u.Gate>=4?.65:1.1,
            "gate_impact"=>s.Multiplier+.2*u.Gate,
            "gate_beast"=>u.Gate>=8?5.5:u.Gate>=7?4.5:3.5,
            _=>s.Multiplier
        };
        int actionHit=0;
        foreach(var candidate in targets)
        {
            var living=Targets(foes);if(living.Count==0)break;
            var t=s.Operator=="random"?living[rng.Next(living.Count)]:candidate;
            if(!living.Contains(t))continue;
            for(int hit=0;hit<hitCount&&t.Alive;hit++)
            {
                var bonus=Rules.Fighters(own,u);var defense=Rules.Fighters(foes,t);
                var crit=rng.NextDouble()<u.CriticalChance+bonus.Crit+(u.Gate==8?.20:0);
                var d=Rules.Hit(t,stat*multiplier*damageScale*u.OutgoingDamage*(1+t.VulnerabilityFrom(u))*(1+s.DamagePerStack*damageStacks)*(1+bonus.Damage)*(crit?u.CriticalMultiplier:1),1-(1-defense.Reduction)*(1-t.ProtectionFrom(u)),s.Operator=="extreme"?.3:0);
                LastImpacts.Add(new(t,d,s.Scale=="essence"?"essence":"physical",crit,actionHit++,u));
                if(d>0&&u.Definition.AttackStatus is {} statusId){var effect=catalog.StatusEffects.Single(e=>e.Id==statusId);if(rng.NextDouble()<t.IncomingStatusChance(statusId,u.Definition.AttackStatusChance*s.StatusChanceMultiplier))t.AddStatus(effect,u);}
                if(d>0&&s.ApplyStatus is {} applied)t.AddStatus(catalog.StatusEffects.Single(e=>e.Id==applied),u);
                Log.Add($"{u.Definition.Name} > {t.Definition.Name}: {s.Name} {d:F0}{(crit?" crítico":"")}");
            }
            if(s.Ice>0&&rng.NextDouble()>=t.ControlResistance)t.ApplyIce(s.Ice);
            if(s.Slow>0)t.ApplySlow(s.Slow,s.Duration);
            if(s.Stun>0)Log.Add(rng.NextDouble()>=t.ControlResistance&&t.ApplyStun(s.Stun)?$"{t.Definition.Name}: stun {s.Stun}":$"{t.Definition.Name}: imune a stun ou fora de combate");
        }
        if(s.Operator=="extreme")u.PayExtreme();
    }
}

