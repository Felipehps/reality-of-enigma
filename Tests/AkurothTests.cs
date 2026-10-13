using Vaelorn;

static class AkurothTests
{
    public static void Run(Catalog c,Action<bool,string> check)
    {
        Unit Boss(bool ascended=false){var u=new Unit(c.Hero("akuroth"),70,true);if(ascended)u.LoseHp(u.MaxHp*.7);return u;}
        Character Dummy(string id,double speed=1)=>new(id,id,"test","strength",new[]{"swordsman"},new(100000,20,20,0,speed),new[]{"slash"});
        var boss=Boss();boss.LoseHp(boss.MaxHp*.6999);check(!boss.Ascended,"AKUROTH remains phase 1 above 30%");
        boss.LoseHp(boss.MaxHp*.0001);check(boss.Ascended&&boss.AkurothPhase==2,"AKUROTH transforms at exactly 30%");
        boss.Hp=boss.MaxHp;boss.LoseHp(1);check(boss.Ascended,"AKUROTH transformation survives healing");
        var dotBoss=Boss(true);var dotSource=new Unit(Dummy("dot-source"));dotBoss.AddStatus(c.StatusEffects.Single(s=>s.Id=="scarlet_bleed_ii"),dotSource);
        var dot=dotBoss.TickDamageStatuses().Single();check(Math.Abs(dot.Damage-dotBoss.MaxHp*.08*.35)<.001,"Sovereign mitigates continuous damage by 65 percent");
        check(new Unit(c.Hero("maltherion")).DamageOverTimeMultiplier==1,"Existing bosses retain continuous damage rules");
        var lethal=Boss();lethal.LoseHp(lethal.MaxHp*2);check(!lethal.Alive,"AKUROTH can die across threshold without resurrection");
        check(!c.Summon.Pool.Contains("akuroth")&&!c.Bosses.Single(b=>b.Id=="akuroth").Recruitable,"AKUROTH excluded from gacha and fragment training");
        var b=new Battle(c,new[]{new Unit(Dummy("hero",999))},new[]{Boss()});
        check(b.SkillsFor(b.Enemies[0]).Length==5&&!b.SkillsFor(b.Enemies[0]).Any(s=>s.MinimumPhase==2),"Phase 1 exposes exactly its five skills");
        var u=b.Enemies[0];u.LoseHp(u.MaxHp*.7);b.Advance();
        check(b.SkillsFor(u).Length==5&&b.SkillsFor(u).All(s=>s.MinimumPhase==2),"Phase 2 replaces action list");
        check(u.ReadyRound["akuroth_annihilation"]==1+c.Ability("akuroth_annihilation").InitialCooldown,"Phase 2 ultimate receives fresh initial cooldown");
        foreach(var s in c.Skills.Where(s=>s.Id.StartsWith("akuroth_"))){
            var a=new Unit(c.Hero("akuroth") with {Stats=c.Hero("akuroth").Stats with {Speed=999}},70,true);
            if(s.MinimumPhase==2)a.LoseHp(a.MaxHp*.7);
            var victims=Enumerable.Range(0,3).Select(i=>new Unit(Dummy("victim"+i),column:i==2?1:0,layer:i==1?1:0)).ToArray();
            var fight=new Battle(c,new[]{a},victims,17);a.ReadyRound[s.Id]=1;
            check(fight.TryAct(s.Id,victims[0],out _),"AKUROTH skill executable: "+s.Id);
            if(s.Id=="akuroth_annihilation"){
                check(a.AnnihilationCharging&&fight.LastImpacts.Count==0,"Annihilation telegraphs with no immediate damage");
                check(!fight.TryAct("akuroth_chaos",victims[0],out _),"Charging cannot switch skills");
                while(fight.NextActor!=a)fight.Advance();
                check(fight.TryAct(s.Id,victims[0],out _)&&!a.AnnihilationCharging,"Annihilation releases on next own turn");
                check(a.ReadyRound[s.Id]==fight.Round+s.Cooldown,"Annihilation cooldown begins after discharge");
                check(fight.LastImpacts.Count==6&&fight.LastImpacts.GroupBy(h=>h.Target).All(g=>g.Count()==2),"Annihilation compresses and explodes once per living hero");
                check(fight.LastImpacts.Select(h=>h.HitIndex).SequenceEqual(Enumerable.Range(0,6)),"Annihilation pairs have distinct compression and blast timing indexes");
            }
            check(fight.LastImpacts.Any(h=>h.Amount>0)&&fight.LastImpacts.All(h=>h.Attacker==a),"Skill reports real attributed damage: "+s.Id);
            if(s.Id=="akuroth_cut")check(fight.LastImpacts.Count==6&&victims[2].Hp==victims[2].MaxHp,"Void cuts hit only the selected column, three times");
            if(s.Operator=="random")check(fight.LastImpacts.Count==s.Hits,"Random skill has configured hit count: "+s.Id);
            if(s.Id=="akuroth_prison")check(victims[0].Stun==1&&victims[0].Stacks("akuroth_drain")==1,"Soul prison applies control and bounded drain");
            if(s.Id=="akuroth_rewrite"){
                var victim=victims[0];
                check(victim.Stacks("akuroth_erasure")==1,"Rewrite applies erasure to living targets");
                check(fight.UnavailableReason(victim,c.Ability("rush")).Contains("Reescrita")&&fight.UnavailableReason(victim,c.Ability("slash"))=="","Erasure blocks specials while permitting basic attacks");
                victim.EndStatusTurn();check(victim.Stacks("akuroth_erasure")==0,"Erasure expires after one victim action");
            }
        }
        var charge=Boss(true);var prey=new Unit(Dummy("prey",1));var interrupted=new Battle(c,new[]{charge},new[]{prey});charge.ReadyRound["akuroth_annihilation"]=1;
        check(interrupted.TryAct("akuroth_annihilation",prey,out _),"Charge begins in interruption test");
        while(interrupted.NextActor!=charge)interrupted.Advance();charge.ApplyStun();interrupted.Advance();
        check(!charge.AnnihilationCharging&&interrupted.LastImpacts.Count==0,"Control interrupts charge without hidden discharge");
        var doomed=Boss(true);var deathBattle=new Battle(c,new[]{doomed},new[]{new Unit(Dummy("survivor"))});doomed.ReadyRound["akuroth_annihilation"]=1;deathBattle.TryAct("akuroth_annihilation",deathBattle.Enemies[0],out _);doomed.LoseHp(doomed.MaxHp);
        check(deathBattle.Finished&&!deathBattle.Won,"Death during charge ends combat normally");
        var results=new List<(bool Won,int Round,bool Phase)>();
        int annihilations=0;var aiSkills=new HashSet<string>();
        for(int seed=0;seed<20;seed++){
            var allies=new[]{"sevrin","kael","aelia","brakk","lyra"}.Select((id,i)=>new Unit(c.Hero(id),70,column:i%3,layer:i/3)).ToArray();
            var enemy=Boss();var fight=new Battle(c,allies,new[]{enemy},seed);
            while(!fight.Finished){fight.Advance();if(fight.LastActor==enemy&&fight.LastSkill is {} used){aiSkills.Add(used.Id);if(used.Id=="akuroth_annihilation"&&!enemy.AnnihilationCharging&&fight.LastImpacts.Any(h=>h.Attacker==enemy))annihilations++;}}
            results.Add((fight.Won,fight.Round,enemy.Ascended));
        }
        check(results.All(r=>r.Round<100),"Twenty seeded AKUROTH fights terminate below round cap");
        check(results.Any(r=>r.Phase),"Reference team reaches transformation in balance simulations");
        check(results.Any(r=>r.Won),"AKUROTH is defeatable by the reference level 70 team");
        check(annihilations>0,"AI completes a telegraphed Annihilation in reference fights");
        Console.WriteLine($"AKUROTH balance: {results.Count(r=>r.Won)}/20 wins; rounds {results.Min(r=>r.Round)}–{results.Max(r=>r.Round)}; phase 2 {results.Count(r=>r.Phase)}/20");
        Console.WriteLine($"AKUROTH AI: {aiSkills.Count}/10 skills observed; {annihilations} Annihilation discharges");
    }
}
