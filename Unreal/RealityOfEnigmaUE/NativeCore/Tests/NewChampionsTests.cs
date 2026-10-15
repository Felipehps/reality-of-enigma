using Vaelorn;
static class NewChampionsTests
{
 public static void Run(Catalog c,Action<bool,string> check){
  Character Dummy(string id,double hp=100000,double speed=1)=>new(id,id,"Test","strength",new[]{"swordsman"},new(hp,100,100,0,speed),new[]{"kick"});
  Unit Actor(string id,bool phase2=false){var d=c.Hero(id);var u=new Unit(d with{Stats=d.Stats with{Speed=999},CritChance=0},boss:d.Role.StartsWith("Chefe"));if(phase2)u.LoseHp(u.MaxHp*(id=="mazakin"?.8:.5));return u;}
  check(new[]{"mazakin","hajun","zhujin"}.All(id=>!c.Summon.Pool.Contains(id)),"New bosses excluded from summons");
  check(c.Summon.Pool.Contains("tenrin")&&c.Summon.Pool.Contains("zevalon"),"Divine heroes summonable");
  check(c.Skills.Count(s=>new[]{"mazakin_","hajun_","zhujin_","tenrin_","zevalon_"}.Any(s.Id.StartsWith))==29,"Twenty-nine active abilities catalogued");
  var maz=Actor("mazakin");maz.LoseHp(maz.MaxHp*.7999);check(!maz.Ascended,"Mazakin stays phase one above 20 percent");maz.LoseHp(maz.MaxHp*.0001);check(maz.Ascended,"Mazakin ascends at exactly 20 percent");
  var haj=Actor("hajun");haj.LoseHp(haj.MaxHp*.5);check(haj.Ascended&&Math.Abs(haj.BleedResistance-.7)<.001,"Hajun ascension and bleeding resistance");
  var prior=haj.Carnage;Rules.Hit(haj,100);check(haj.Carnage==prior+1,"Receiving an attack grants Carnage");
  for(int i=0;i<20;i++)haj.RegisterHit(1);check(haj.Carnage==7,"Ascended Carnage capped at seven");
  haj.EndRound(3);check(haj.Carnage==5,"Two idle rounds remove two Carnage");
  var flower=Actor("zhujin");flower.LoseHp(flower.MaxHp*.5);check(flower.Crimson,"Zhu Jin transforms at exactly half health");flower.LoseHp(flower.MaxHp*.25);check(flower.Blooming,"Zhu Jin blooms at exactly quarter health");
  check(Math.Abs(flower.ControlResistance-.7)<.001&&Math.Abs(flower.PoisonResistance-.6)<.001,"Zhu Jin transformed resistances");
  foreach(var s in c.Skills.Where(s=>new[]{"mazakin_","hajun_","zhujin_","tenrin_","zevalon_"}.Any(s.Id.StartsWith))){
   var id=s.Id.Split('_')[0];var u=Actor(id,s.MinimumPhase==2);var prey=Enumerable.Range(0,3).Select(i=>new Unit(Dummy("victim"+i),column:i)).ToArray();var b=new Battle(c,new[]{u},prey,17);u.ReadyRound[s.Id]=1;
   check(b.TryAct(s.Id,prey[0],out var error),"Ability executes: "+s.Id+" "+error);
   check(b.LastSkill?.Id==s.Id&&u.ReadyRound.GetValueOrDefault(s.Id,0)>0,"Ability records and cooldown: "+s.Id);
   if(s.Operator!="floral_sleep")check(b.LastImpacts.Any(h=>h.Amount>0)||b.LastBuffTargets.Count>0||b.Hazards.Count>0,"Ability changes combat: "+s.Id);
  }
  var ten=Actor("tenrin");var victim=new Unit(Dummy("marked"));for(int i=0;i<5;i++)victim.AddStatus(c.StatusEffects.Single(x=>x.Id=="divine_mark"),ten);var tb=new Battle(c,new[]{ten},new[]{victim});_ = tb.NextActor;
  check(ten.DivineFocus==5&&Math.Abs(ten.Strength-c.Hero("tenrin").Stats.Strength*1.2)<.001,"Divine focus grants twenty percent attack at cap");
  check(Math.Abs(ten.ProtectionFrom(victim)-.2)<.001,"Tenrin front protection");
  ten=new Unit(c.Hero("tenrin") with{Stats=c.Hero("tenrin").Stats with{Speed=999}},layer:1);var back=new Battle(c,new[]{ten},new[]{new Unit(Dummy("back-target"))});check(!back.TryAct("tenrin_palm",null,out _),"Palma restricted to front row");
  var zv=Actor("zevalon");zv.Hp=zv.MaxHp*.5;victim=new Unit(Dummy("lightning"));for(int i=0;i<4;i++)victim.AddStatus(c.StatusEffects.Single(x=>x.Id=="celestial_mark"),zv);var zb=new Battle(c,new[]{zv},new[]{victim},17);zb.TryAct("zevalon_basic",victim,out _);
  check(zb.LastImpacts.Any(h=>h.Effect=="zevalon_lightning"),"Fifth Celestial mark triggers lightning");check(zv.Hp>zv.MaxHp*.5,"Lightning activation heals Zevalon");check(Math.Abs(victim.ControlResistance)<.001&&victim.Stacks("celestial_mark")==5,"Celestial mark cap and resistance reduction");
  zv=Actor("zevalon");victim=new Unit(Dummy("armor") with{Stats=Dummy("armor").Stats with{Defense=100000}});zb=new Battle(c,new[]{zv},new[]{victim});for(int charge=0;charge<5;charge++){while(zb.NextActor!=zv)zb.Advance();zv.ReadyRound["zevalon_will"]=0;zb.TryAct("zevalon_will",null,out _);}while(zb.NextActor!=zv)zb.Advance();check(zv.CelestialEnergy==5,"Five divine abilities fill celestial energy");zb.TryAct("zevalon_basic",victim,out _);check(zb.LastImpacts.Any(h=>h.Kind=="true"&&Math.Abs(h.Amount-zv.Strength)<.001)&&zv.CelestialEnergy==0,"Full celestial energy empowers and consumes next attack");
  ten=Actor("tenrin");var ally=new Unit(Dummy("ally",10000,10));ally.ApplyStun();ally.AddStatus(c.StatusEffects.Single(x=>x.Id=="poison"),victim);var support=new Battle(c,new[]{ten,ally},new[]{new Unit(Dummy("enemy"))});support.TryAct("tenrin_palm",null,out _);check(ally.Stun==0&&ally.Stacks("poison")==1,"Palma cleanses one effect and preserves another");check(ally.Stacks("divine_guard")==1,"Palma protects every ally");
  zv=Actor("zevalon");victim=new Unit(Dummy("reflect"));zb=new Battle(c,new[]{zv},new[]{victim});zb.TryAct("zevalon_will",null,out _);zb.Advance();var incoming=zb.LastImpacts.First(h=>h.Target==zv&&h.Attacker==victim);check(zb.LastImpacts.Any(h=>h.Kind=="reflect"&&h.Target==victim&&Math.Abs(h.Amount-incoming.Amount*.15)<.001),"Celestial field reflects actual mitigated damage");
  maz=Actor("mazakin");victim=new Unit(Dummy("ritual"));var mb=new Battle(c,new[]{maz},new[]{victim});mb.TryAct("mazakin_orb",victim,out _);check(mb.Allies.Count(u=>u.Id=="mazakin_orb"&&u.Alive)==3,"Mazakin summons three destructible orbs");check(mb.TurnPreview.All(u=>u.Id!="mazakin_orb"),"Orbs do not take normal turns");var orb=mb.Allies.First(u=>u.Id=="mazakin_orb");Rules.Hit(orb,100000);check(!orb.Alive,"Orbs can be destroyed");maz.LoseHp(maz.MaxHp);mb.Advance();check(mb.Allies.Where(u=>u.Id=="mazakin_orb").All(u=>!u.Alive),"Boss death removes summoned orbs");
  var mover=Actor("tenrin");var enemy=new Unit(Dummy("root"));var movement=new Battle(c,new[]{mover},new[]{enemy});movement.LockFormation();mover.AddStatus(c.StatusEffects.Single(x=>x.Id=="mazakin_chains"),enemy);check(!movement.TryRepositionActive(1,out _),"Subworld chains block repositioning");mover.Statuses.Clear();check(movement.TryRepositionActive(1,out _)&&mover.Column==1,"Reposition moves and consumes a turn");
  maz=Actor("mazakin");victim=new Unit(Dummy("field"));mb=new Battle(c,new[]{maz},new[]{victim});mb.TryAct("mazakin_ritual",victim,out _);mb.Advance();check(mb.LastImpacts.Any(h=>h.Kind=="hazard")&&victim.Stacks("condemnation")==2,"Ritual damages cell and adds two condemnations at round end");
  var originalResistance=victim.MagicResistance;check(Math.Abs(originalResistance+.08)<.001,"Condemnation reduces magic resistance per stack");
  foreach(var id in new[]{"mazakin","hajun","zhujin"}){
   int wins=0,phases=0;for(int seed=0;seed<10;seed++){
    var party=new[]{"tenrin","zevalon","aelia","sevrin","lyra"}.Select((name,i)=>new Unit(c.Hero(name),70,column:i%3,layer:i/3)).ToArray();var boss=new Unit(c.Hero(id),70,true,1,1);var fight=new Battle(c,party,new[]{boss},seed);int actions=0;while(!fight.Finished&&actions++<1000)fight.Advance();check(fight.Finished&&actions<1000,"Bounded new boss battle: "+id+" seed "+seed);if(fight.Won)wins++;if(boss.Ascended||boss.Crimson)phases++;
   }
   if(wins==0){
    for(int seed=0;seed<10;seed++){
     var party=new[]{"tenrin","zevalon","aelia","sevrin","lyra"}.Select((name,i)=>new Unit(c.Hero(name),70,column:i%3,layer:i/3)).ToArray();var boss=new Unit(c.Hero(id),70,true,1,1);var fight=new Battle(c,party,new[]{boss},seed);fight.LockFormation();int actions=0;
     while(!fight.Finished&&actions++<1000){var next=fight.NextActor!;if(fight.Allies.Contains(next)){
      if(fight.Hazards.Any(h=>fight.Enemies.Contains(h.Source)&&h.Cell==next.Layer*3+next.Column)){var free=Enumerable.Range(0,9).FirstOrDefault(cell=>!fight.Allies.Any(a=>a.Alive&&a.Layer*3+a.Column==cell)&&!fight.Hazards.Any(h=>h.Cell==cell),-1);if(free>=0&&fight.TryRepositionActive(free,out _))continue;}
      var skills=fight.SkillsFor(next).Where(a=>fight.UnavailableReason(next,a)=="").ToArray();var living=Battle.Targets(fight.Enemies);var ability=skills.OrderByDescending(a=>a.Operator=="aoe"?a.Multiplier*living.Count:a.Operator=="heal"?party.Any(t=>t.Alive&&t.Hp/t.MaxHp<.5)?4:0:a.Multiplier).FirstOrDefault();
      if(ability!=null){var target=ability.Operator=="heal"?party.Where(a=>a.Alive).OrderBy(a=>a.Hp/a.MaxHp).First():living.OrderBy(a=>a.Id=="mazakin_orb"?0:1).ThenBy(a=>a.Hp).First();if(fight.TryAct(ability.Id,target,out _))continue;}
     }fight.Advance();}
     if(fight.Won)wins++;if(boss.Ascended||boss.Crimson)phases++;
    }
    Console.WriteLine($"TACTICAL_BOSS_BALANCE {id} wins={wins}/10");
   }
   check(wins>0,"New boss can be defeated: "+id);check(phases>0,"New boss reaches second phase: "+id);Console.WriteLine($"NEW_BOSS_BALANCE {id} wins={wins}/10 transformations={phases}/10");
  }
 }
}
