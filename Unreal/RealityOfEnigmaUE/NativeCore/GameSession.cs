using System.Text.Json;
using System.Text.Json.Nodes;
using Vaelorn;

// This library owns game rules only. Unreal owns input, rendering, sound and menus.
public sealed class GameSession
{
    Game? game;
    Battle? battle;
    TrainingRun? training;
    JsonNode? catalogJson,regions,stories,opening;
    string savePath="",mode="",notice="";
    int stage;
    bool awarded;
    public JsonObject Execute(JsonObject r)
    {
        string command=r["command"]?.GetValue<string>()??"state";
        string id=r["id"]?.GetValue<string>()??"";
        int number=r["number"]?.GetValue<int>()??0;
        bool persist=false;
        if(command=="init") {
            string data=r["data"]!.GetValue<string>();
            savePath=r["save"]!.GetValue<string>();
            var text=File.ReadAllText(Path.Combine(data,"catalog.json"));
            catalogJson=JsonNode.Parse(text);
            regions=JsonNode.Parse(File.ReadAllText(Path.Combine(data,"regions.json")));
            stories=JsonNode.Parse(File.ReadAllText(Path.Combine(data,"hero-stories.json")));
            opening=JsonNode.Parse(File.ReadAllText(Path.Combine(data,"opening.json")));
            game=new Game(Catalog.Load(text),Persistence.Read(savePath));
            notice="Jornada carregada.";
        }
        if(game==null)throw new InvalidOperationException("Jogo não inicializado");
        switch(command) {
            case "level": notice=game.LevelUp(id)?"Nível aumentado":"XP insuficiente ou limite de estrela";persist=true;break;
            case "promote":notice=game.Promote(id)?"Estrela aumentada":"Requer nível máximo e cópia/Coringa";persist=true;break;
            case "reconstruct":notice=game.Reconstruct(id)?"Cópia reconstruída":"Fragmentos insuficientes";persist=true;break;
            case "path":notice=game.ChangePath(id)?"Caminho de Kael alterado":"Requer Kael nível 30";persist=true;break;
            case "summon":var pulled=game.Summon(number);notice=pulled.Length==0?"Gold insuficiente":string.Join(", ",pulled.Select(x=>game.Catalog.Hero(x).Name));persist=true;break;
            case "collect":game.Collect(DateTimeOffset.UtcNow);notice="Recompensas offline coletadas";persist=true;break;
            case "simulate":notice=game.TrainingReward(id,true,true)?"Simulação concluída":"Requer treino vencido e ticket";persist=true;break;
            case "team":
                if(number<0||number>=game.Save.Team.Count||!game.Save.Roster.Any(x=>x.Id==id))throw new InvalidOperationException("Slot ou personagem inválido");
                int previous=game.Save.Team.IndexOf(id);if(previous>=0)game.Save.Team[previous]=game.Save.Team[number];
                game.Save.Team[number]=id;notice="Equipe atualizada";persist=true;break;
            case "formation":
                var cells=r["cells"]!.AsArray().Select(x=>x!.GetValue<int>()).ToArray();
                game.Save.FormationCells=Formation.Normalize(cells,game.Save.Team.Count).ToList();notice="Formação salva";persist=true;break;
            case "opening":game.Save.OpeningSeen=true;game.Save.ForestEncounterSeen=true;persist=true;break;
            case "start":Start(id,number,r["boss"]?.GetValue<string>()??"varkhan");break;
            case "move":
                if(battle==null)throw new InvalidOperationException("Sem batalha");
                var moving=FindUnit(id);notice=moving!=null&&battle.TryMoveAlly(moving,number)?"Formação atualizada":"Formação bloqueada";
                if(mode is "campaign" or "training") {game.Save.FormationCells=battle.Allies.Select(x=>x.Layer*3+x.Column).ToList();persist=true;}break;
            case "begin":battle?.LockFormation();notice="Combate iniciado";break;
            case "act":
                if(battle==null)throw new InvalidOperationException("Sem batalha");
                if(!battle.FormationLocked)throw new InvalidOperationException("Inicie o combate depois de organizar a formação");
                if(battle.Finished) {if(training!=null&&training.NextWave()){battle=training.Current;battle.LockFormation();notice=$"Onda {training.Wave}/3";}break;}
                if(id.Length==0)battle.Advance();
                else if(!battle.TryAct(id,FindUnit(r["target"]?.GetValue<string>()??""),out var error))throw new InvalidOperationException(error);
                notice=battle.LastAction;Award();break;
            case "leave":battle=null;training=null;mode="";break;
            case "save":persist=true;break;
        }
        if(persist)Persistence.Write(savePath,game.Save);
        return State();
    }
    Unit? FindUnit(string key)=>battle?.AlliedTargets.Concat(battle.EnemyTargets).FirstOrDefault(x=>x.InstanceId.ToString()==key);
    void Start(string kind,int requested,string bossId)
    {
        var g=game!;var c=g.Catalog;
        var cells=Formation.Normalize(g.Save.FormationCells,g.Save.Team.Count);
        List<Unit> Team(IEnumerable<string> ids,int? level=null)=>ids.Select((x,i)=> {
            var owner=g.Save.Roster.FirstOrDefault(o=>o.Id==x)??g.Save.CampaignGuests.FirstOrDefault(o=>o.Id==x);
            return new Unit(c.Hero(x),level??owner?.Level??1,false,cells[i]%3,cells[i]/3,owner?.Path??"base");
        }).ToList();
        List<Unit> allies=Team(g.Save.Team),enemies;
        TrainingRun? nextTraining=null;
        switch(kind) {
            case "story":case "campaign":
                if(kind=="story")requested=1;
                if(kind=="campaign"&&!g.CanEnterStage(requested))throw new InvalidOperationException("Fase bloqueada");
                var definition=c.Stages.Single(x=>x.Number==requested);
                if(requested<=10)allies=Team(new[]{"kael","savor","aelia","brakk","raizen"},kind=="story"?1:null);
                enemies=definition.Enemies.Select((e,i)=>new Unit(c.Hero(e.CharacterId),e.Level,e.IsBoss,e.CharacterId is "nerathis" or "maltherion"?1:i%3,e.CharacterId is "nerathis" or "maltherion"?1:i/3)).ToList();break;
            case "training":
                nextTraining=g.BeginTraining(bossId)??throw new InvalidOperationException("Treino bloqueado");
                allies=nextTraining.Current.Allies;enemies=nextTraining.Current.Enemies;break;
            case "akuroth":allies=Team(new[]{"sevrin","kael","aelia","brakk","lyra"},70);enemies=new(){new(c.Hero("akuroth"),70,true,1,1)};break;
            case "eclipse":allies=Team(new[]{"sevrin","kael","aelia","brakk","lyra"},60);enemies=new(){new(c.Hero("maltherion"),60,true,1,1)};break;
            case "floral":allies=Team(new[]{"kael","savor","aelia","brakk","lyra"},50);enemies=new(){new(c.Hero("nerathis"),50,true,1,1)};break;
            case "cast":allies=Team(new[]{"savor","aelia","brakk","lyra","raizen"},20);enemies=new(){new(c.Hero("varkhan"),20,true),new(c.Hero("solarius"),20,true,1)};break;
            case "sandbox":allies=new(){new(c.Hero("kael"),70,false,0,0,"martial")};allies[0].OpenGate(8);enemies=new(){new(c.Hero("solarius"),70,true)};break;
            case "varkas":case "sevrin":
                allies=new(){new(c.Hero(kind),30,false,1),new(c.Hero("aelia"),30,false,0,1)};
                enemies=Enumerable.Range(0,3).Select(i=>new Unit(c.Hero("enemy_fenda") with{Stats=new(20000,25,10,15,70)},20,kind=="varkas"&&i==0,kind=="sevrin"?1:i,kind=="sevrin"?i:0)).ToList();break;
            case "demo":
                allies=Team(g.Save.Team,20);enemies=new(){new(c.Hero("varkhan"),20,true)};
                for(int i=1;i<5;i++)enemies.Add(new(c.Hero("enemy_fenda") with{Name="Eco da Fenda",Stats=new(2100,80,40,20,90)},20,false,i%3,i/3));
                enemies[1].ApplyStun();enemies[2].ApplyIce(5);enemies[3].ApplyIce(3);break;
            default:throw new InvalidOperationException("Modo desconhecido");
        }
        mode=kind;stage=requested;awarded=false;training=nextTraining;
        battle=training?.Current??new Battle(c,allies,enemies);notice="Organize a formação e inicie o combate";
    }
    void Award()
    {
        if(battle==null||!battle.Finished||awarded||training!=null&&battle.Won&&training.Wave<3)return;
        awarded=true;
        if(mode=="campaign") {game!.RewardCampaignEncounter(stage,battle.Won,DateTimeOffset.UtcNow,battle.Allies.Select(x=>x.Id));if(battle.Won)notice=game.LastCampaignReward;Persistence.Write(savePath,game.Save);}
        if(training!=null){notice=training.ClaimReward()?"Três ondas vencidas: recompensas concedidas":"Treino encerrado sem recompensa";Persistence.Write(savePath,game!.Save);}
    }
    JsonObject UnitState(Unit u)=>new() {
        ["key"]=u.InstanceId.ToString(),["id"]=u.Id,["name"]=u.CombatName,["hp"]=u.Hp,["maxHp"]=u.MaxHp,
        ["level"]=u.Level,["column"]=u.Column,["layer"]=u.Layer,["alive"]=u.Alive,["boss"]=u.Boss,
        ["ascended"]=u.Ascended,["gate"]=u.Gate,["ice"]=u.Ice,["freeze"]=u.Freeze,["stun"]=u.Stun,
        ["charging"]=u.AnnihilationCharging,["strength"]=u.Strength,["essence"]=u.Essence,["defense"]=u.Defense,["speed"]=u.Speed,
        ["buffs"]=new JsonArray(u.Buffs.Select(p=>(JsonNode)new JsonObject{["id"]=p.Key,["strength"]=p.Value.Strength,["essence"]=p.Value.Essence,["speed"]=p.Value.Speed,["expires"]=p.Value.ExpiresRound}).ToArray()),
        ["statuses"]=new JsonArray(u.Statuses.Select(s=>(JsonNode)new JsonObject{["id"]=s.Effect.Id,["turns"]=s.RemainingTurns}).ToArray())
    };
    JsonObject State()
    {
        var result=new JsonObject {
            ["ok"]=true,["notice"]=notice,["mode"]=mode,["stage"]=stage,
            ["save"]=JsonSerializer.SerializeToNode(game!.Save,GameJsonContext.Default.SaveData),
            ["catalog"]=catalogJson!.DeepClone(),["regions"]=regions!.DeepClone(),["stories"]=stories!.DeepClone(),["opening"]=opening!.DeepClone()
        };
        if(battle is {} b) {
            var actor=b.NextActor;
            result["battle"]=new JsonObject {
                ["round"]=b.Round,["finished"]=b.Finished,["won"]=b.Won,["locked"]=b.FormationLocked,
                ["wave"]=training?.Wave??1,["actor"]=actor?.InstanceId.ToString(),["allyTurn"]=actor!=null&&b.Allies.Contains(actor),
                ["allies"]=new JsonArray(b.AlliedTargets.Select(x=>(JsonNode)UnitState(x)).ToArray()),
                ["enemies"]=new JsonArray(b.EnemyTargets.Select(x=>(JsonNode)UnitState(x)).ToArray()),
                ["turns"]=new JsonArray(b.TurnPreview.Select(x=>(JsonNode)JsonValue.Create(x.CombatName)!).ToArray()),
                ["skills"]=new JsonArray(actor==null?Array.Empty<JsonNode>():b.SkillsFor(actor).Select(s=>(JsonNode)new JsonObject {
                    ["id"]=s.Id,["name"]=s.Name,["operator"]=s.Operator,["description"]=s.Description??s.Name,
                    ["reason"]=b.UnavailableReason(actor,s)
                }).ToArray()),
                ["lastTarget"]=b.LastTarget?.InstanceId.ToString(),["buffTargets"]=new JsonArray(b.LastBuffTargets.Select(u=>(JsonNode)JsonValue.Create(u.InstanceId.ToString())!).ToArray()),["lastOperator"]=b.LastSkill?.Operator,["lastActor"]=b.LastActor?.InstanceId.ToString(),["lastSkill"]=b.LastSkill?.Id,["lastAction"]=b.LastAction,
                ["impacts"]=new JsonArray(b.LastImpacts.Select(x=>(JsonNode)new JsonObject{["target"]=x.Target.InstanceId.ToString(),["amount"]=x.Amount,["kind"]=x.Kind,["critical"]=x.Critical,["hit"]=x.HitIndex}).ToArray()),
                ["log"]=new JsonArray(b.Log.TakeLast(12).Select(x=>(JsonNode)JsonValue.Create(x)!).ToArray())
            };
        }
        return result;
    }
}
