namespace Vaelorn;

public partial class Unit
{
    public bool Ascended {get;private set;}
    public double DamageOverTimeMultiplier=>Ascended?1-Math.Clamp(Definition.Eclipse?.DotResistance??0,0,.8):1;
    public int Carnage {get;private set;}
    public int ConsecutiveHits {get;private set;}
    public int LastSuccessfulRound {get;private set;}
    public double BleedingSpeed {get;set;}
    public int FocusBleeds {get;set;}
    public int BleedCount=>Statuses.Count(s=>s.Effect.Id=="bleed"||s.Effect.Family=="bleed");
    public double ControlResistance=>Math.Clamp(((Ascended?.8:Crimson?Definition.Floral?.CrimsonControl??Definition.ControlResistance:Definition.ControlResistance)+StatusReduction(e=>e.ControlBonus)-Statuses.Sum(s=>s.Effect.ControlPenalty*s.Magnitude))*(1-StatusReduction(e=>e.ControlReduction)),0,1);
    public double CriticalChance=>(Crimson?Definition.Floral?.CrimsonCritChance??Definition.CritChance:Definition.CritChance)+(Id=="tenrin"?.02*DivineFocus:0)+(Ascended?.05:0)+Carnage*(Ascended?Definition.Eclipse?.AscendedCritPerStack??0:Definition.Eclipse?.CritPerStack??0)+FocusBleeds*(Definition.Scarlet?.TargetCritPerBleed??0);
    public double CriticalMultiplier=>(Crimson?Definition.Floral?.CrimsonCritMultiplier??Definition.CritMultiplier:Definition.CritMultiplier)+(Ascended?.3:0)+Carnage*(Ascended?Definition.Eclipse?.AscendedCritDamagePerStack??0:Definition.Eclipse?.CritDamagePerStack??0);
    public double ProtectionFrom(Unit attacker)=>1-(1-(Definition.Scarlet is {} k&&Layer==0&&attacker.Hp/attacker.MaxHp>k.HealthyThreshold?k.FrontProtection:0))*(1-(Id=="tenrin"&&Layer==0?.2:0))*(1-(Id=="hajun"&&Ascended&&Carnage>=7?.3:0));
    public void RegisterHit(int round){LastSuccessfulRound=round;ConsecutiveHits++;if(Definition.Eclipse is {} k)Carnage=Math.Min(Ascended?k.AscendedCap:k.StackCap,Carnage+1);}
    public void MissAttack(){ConsecutiveHits=0;}
    private void UpdateAscension(){if(Alive&&Definition.Eclipse is {} k&&Hp/MaxHp<=k.Threshold+1e-12)Ascended=true;}
    private void EclipseRound(int round){if(Definition.Eclipse!=null&&round-LastSuccessfulRound>=2){Carnage=Math.Max(0,Carnage-2);ConsecutiveHits=0;LastSuccessfulRound=round;}if(Alive)Hp=Math.Min(MaxHp,Hp+MaxHp*StatusReduction(e=>e.Regeneration));}
}

public partial class Battle
{
    private readonly HashSet<(Guid Hero,Guid Victim)> scarletDefeats=new();
    private void AwardBleedingDefeats(){
        foreach(var hero in Allies.Concat(Enemies).Where(u=>u.Alive&&u.Definition.Scarlet!=null)){
            var kit=hero.Definition.Scarlet!;var foes=Allies.Contains(hero)?Enemies:Allies;
            foreach(var fallen in foes.Where(t=>!t.Alive&&t.DiedBleeding))if(scarletDefeats.Add((hero.InstanceId,fallen.InstanceId))){
                double heal=Math.Min(hero.MaxHp-hero.Hp,hero.MaxHp*kit.KillHeal*(1-hero.StatusReduction(e=>e.HealReduction)));hero.Hp+=heal;
                if(heal>0)LastImpacts.Add(new(hero,heal,"heal",false,0,hero));
                foreach(var key in hero.ReadyRound.Keys.ToArray())hero.ReadyRound[key]-=kit.KillCooldown;
            }
        }
    }
    private void UpdateScarletSpeed(){foreach(var u in Allies.Concat(Enemies).Where(u=>u.Definition.Scarlet!=null)){
        var k=u.Definition.Scarlet!;var foes=Targets(Allies.Contains(u)?Enemies:Allies);
        u.BleedingSpeed=Math.Min(k.SpeedCap,foes.Sum(t=>t.BleedCount)*k.SpeedPerBleed);
        u.FocusBleeds=Math.Min(k.StackCap,foes.Select(t=>t.BleedCount).DefaultIfEmpty().Max());
    }}
    private void KitStatus(Unit target,Unit source,string id){if(target.AddStatus(catalog.StatusEffects.Single(s=>s.Id==id),source)&&target==source)target.Statuses.Last().RemainingTurns++;}
    private void ActExtended(Unit u,Unit target,List<Unit> own,List<Unit> foes,Skill s){
        string kind=u.Id=="hajun"?s.Id.Replace("hajun_","eclipse_"):s.Id;
        if(kind=="eclipse_body"){
            KitStatus(u,u,"demon_guard");
            u.AddStatus(new("wounded_armor",1,3,DefenseBonus:Math.Min(.3,Math.Floor((1-u.Hp/u.MaxHp)*10)*.03)),u);
            if(u.Ascended)KitStatus(u,u,u.Id=="hajun"?"hajun_regeneration":"eclipse_regeneration");LastBuffTargets.Add(u);return;
        }
        var victims=kind is "eclipse_judgment" or "eclipse_final" or "scarlet_ultimate"?Targets(foes):kind=="scarlet_front"?Targets(foes).Where(t=>t.Column==target.Column).ToList():new List<Unit>{target};
        int index=0;bool killed=false;
        foreach(var t in victims.Where(t=>t.Alive)){
            int bleeding=t.BleedCount;int hits=kind=="scarlet_quick"&&bleeding>0?2:s.Hits;
            u.FocusBleeds=Math.Min(u.Definition.Scarlet?.StackCap??0,bleeding);
            for(int h=0;h<hits&&t.Alive;h++){
                double multiplier=kind=="scarlet_quick"&&h==1?1.2:s.Multiplier;
                double raw=multiplier*u.Strength+s.TargetHpDamage*t.MaxHp;
                if(kind=="eclipse_axe")raw*=1+Math.Min(.45,u.ConsecutiveHits*.15);
                if(kind=="eclipse_bone")raw*=1+Math.Min(.7,u.Carnage*.1);
                if(kind=="eclipse_judgment"){if(u.Ascended)raw=3*u.Strength+.12*t.MaxHp;raw*=1+Math.Min(.5,u.Carnage*.1);}
                if(kind=="scarlet_blood"&&bleeding>=3)raw*=1.4;
                if(kind=="scarlet_ultimate")raw*=1+Math.Min(.4,bleeding*.08);
                var synergy=Rules.Fighters(own,u);bool crit=rng.NextDouble()<u.CriticalChance+synergy.Crit;
                double dealt=Rules.Hit(t,raw*u.OutgoingDamage*(1+t.VulnerabilityFrom(u))*(1+synergy.Damage)*(crit?u.CriticalMultiplier:1),1-(1-Rules.Fighters(foes,t).Reduction)*(1-t.ProtectionFrom(u)),u.Ascended?.2:0);
                LastImpacts.Add(new(t,dealt,"physical",crit,index++,u));
                if(dealt<=0){u.MissAttack();continue;}
                int oldCarnage=u.Carnage;u.RegisterHit(Round);
                if(u.Id!="hajun"&&u.Ascended&&oldCarnage<7&&u.Carnage==7)KitStatus(u,u,"eclipse_pressure");
                double heal=dealt*(u.Id=="hajun"&&u.Ascended&&u.Carnage>=7?.1:u.StatusReduction(e=>e.LifeSteal))*(1-u.StatusReduction(e=>e.HealReduction));if(heal>0)u.Hp=Math.Min(u.MaxHp,u.Hp+heal);
                if(u.Definition.Scarlet is {} sk&&!(kind=="scarlet_quick"&&h==1))KitStatus(t,u,"scarlet_bleed");
                if(kind=="scarlet_quick"&&h==0&&rng.NextDouble()<.7)KitStatus(t,u,"scarlet_bleed");
                if(kind is "scarlet_blood" or "scarlet_ultimate")KitStatus(t,u,"scarlet_bleed_ii");
                if(kind=="scarlet_blood")KitStatus(t,u,"scarlet_weakness");
                if(kind=="scarlet_front")KitStatus(t,u,"scarlet_front_weakness");
                if(kind=="scarlet_ultimate")KitStatus(t,u,"scarlet_ultimate_weakness");
                if(crit&&kind=="eclipse_axe")KitStatus(t,u,u.Id=="hajun"?"hajun_axe_bleed":"axe_bleed");
                if(crit&&kind=="eclipse_bone")KitStatus(t,u,"fracture");
                if(kind=="eclipse_crush"){
                    if(rng.NextDouble()<(u.Ascended?.5:.3)*(1-t.ControlResistance)&&t.ApplyStun(2))t.PendingWakeDebuff=true;
                    if(u.Ascended)KitStatus(t,u,"crush_fracture");
                }
                if(kind=="eclipse_triple"){
                    if(h==2&&rng.NextDouble()<.4*(1-t.ControlResistance))t.ApplyStun(1);
                    if(rng.NextDouble()<.15)KitStatus(t,u,"debilitation");
                }
                if(kind=="eclipse_judgment")KitStatus(t,u,u.Ascended?"execution_curse_ii":"execution_curse");
                if(kind=="eclipse_final"){KitStatus(t,u,"inevitable_end");t.ApplySlow(.2,2);}
                if(!t.Alive)killed=true;
                Log.Add($"{u.CombatName} > {t.CombatName}: {s.Name} {dealt:F0}");
            }
        }
        if(killed&&kind=="eclipse_final"){u.Hp=Math.Min(u.MaxHp,u.Hp+u.MaxHp*.15*(1-u.StatusReduction(e=>e.HealReduction)));u.RegisterHit(Round);}
    }
}
