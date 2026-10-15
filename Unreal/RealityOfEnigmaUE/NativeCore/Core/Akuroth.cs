namespace Vaelorn;

public partial class Unit
{
    public bool AnnihilationCharging {get;internal set;}
    public int AkurothPhase => Ascended?2:1;
    public string AkurothTitle => Ascended?"Soberano do Absoluto":"O Portador do Vazio";
}

public partial class Battle
{
    private readonly HashSet<Unit> awakenedAkuroth=new();
    private void SyncAkurothPhases(int round)
    {
        foreach(var u in Allies.Concat(Enemies).Where(u=>u.Id=="akuroth"&&u.Alive&&u.Ascended))
            if(awakenedAkuroth.Add(u)){
                foreach(var s in SkillsFor(u))u.ReadyRound[s.Id]=s.Cooldown==0?round:round+1+s.InitialCooldown;
                Log.Add("AKUROTH: Soberano do Absoluto — a realidade se rompe!");
            }
    }
    private Skill ChooseAkurothSkill(Unit u,Skill[] available)
    {
        if(u.AnnihilationCharging)return catalog.Ability("akuroth_annihilation");
        // Priorities + independent cooldowns create a deterministic rotation with a basic fallback.
        string[] priority=u.Ascended
            ?new[]{"akuroth_annihilation","akuroth_rewrite","akuroth_rain","akuroth_collapse","akuroth_chaos"}
            :new[]{"akuroth_rift","akuroth_meteor","akuroth_prison","akuroth_waves","akuroth_cut"};
        return priority.Select(id=>available.FirstOrDefault(s=>s.Id==id)).First(s=>s!=null)!;
    }
    private void ActAkuroth(Unit u,Unit target,List<Unit> own,List<Unit> foes,Skill s)
    {
        if(s.Id=="akuroth_annihilation"&&!u.AnnihilationCharging){
            u.AnnihilationCharging=true;LastAction="Aniquilação Absoluta • PREPARANDO";
            LastBuffTargets.Add(u);Log.Add("AKUROTH concentra todos os mundos: Aniquilação na próxima ação. Proteja a equipe ou interrompa!");return;
        }
        u.AnnihilationCharging=false;
        if(s.Id=="akuroth_rewrite"){
            u.Statuses.RemoveAll(st=>st.Effect.DamageReduction>0||st.Effect.DefenseReduction>0||st.Effect.HealReduction>0||st.Effect.SourceStrengthDamage>0||st.Effect.TargetHpDamage>0||st.Effect.Sleep);
            LastBuffTargets.Add(u);
        }
        var living=Targets(foes);
        var victims=s.Operator=="random"?Enumerable.Range(0,s.Hits).Select(_=>target).ToArray()
            :s.Operator=="aoe"?living.ToArray():s.Operator=="line"?living.Where(t=>t.Column==target.Column).ToArray():new[]{target};
        int index=0,targetOrdinal=0;
        foreach(var candidate in victims){
            int ordinal=targetOrdinal++;
            living=Targets(foes);if(living.Count==0)break;
            var t=s.Operator=="random"?living[rng.Next(living.Count)]:candidate;
            if(!living.Contains(t))continue;
            for(int h=0;h<(s.Operator=="random"?1:s.Hits)&&t.Alive;h++){
                double stat=s.Scale=="essence"?u.Essence:u.Strength;
                var bonus=Rules.Fighters(own,u);bool critical=rng.NextDouble()<u.CriticalChance+bonus.Crit;
                double multiplier=s.Id=="akuroth_annihilation"?(h==0?.35:1.25):s.Multiplier;
                double hpDamage=s.Id=="akuroth_annihilation"?(h==0?.01:.05):s.TargetHpDamage;
                double raw=(stat*multiplier+t.MaxHp*hpDamage)*u.OutgoingDamage*(1+t.VulnerabilityFrom(u))*(1+bonus.Damage)*(critical?u.CriticalMultiplier:1);
                double dealt=Rules.Hit(t,raw,1-(1-Rules.Fighters(foes,t).Reduction)*(1-t.ProtectionFrom(u)));
                LastImpacts.Add(new(t,dealt,s.Scale=="essence"?"essence":"physical",critical,s.Id=="akuroth_annihilation"?ordinal*2+h:index++,u));
                if(dealt>0&&s.ApplyStatus is {} status)t.AddStatus(catalog.StatusEffects.Single(st=>st.Id==status),u);
                Log.Add($"AKUROTH > {t.CombatName}: {s.Name} {dealt:F0}");
            }
            if(s.Stun>0&&rng.NextDouble()>=t.ControlResistance)t.ApplyStun(s.Stun);
            if(s.Slow>0)t.ApplySlow(s.Slow,s.Duration);
        }
    }
}
