using Godot;
using Vaelorn;
using System;
using System.Linq;
using System.Collections.Generic;

public partial class Main:Control
{
    private bool IsQa=>OS.GetCmdlineUserArgs().Any(a=>a=="--qa"||a=="--qa-v23"||a=="--qa-v24"||a=="--qa-v25"||a=="--qa-v26"||a=="--qa-v29"||a=="--qa-v30"||a=="--qa-akuroth");
    private Game game=null!;
    private Battle? battle;
    private Label summary=null!;
    private Label notice=null!;
    private Label units=null!;
    private RichTextLabel log=null!;
    private OptionButton selection=null!;
    private string savePath="";
    private bool auto;
    private double elapsed;
    private string mode="sandbox";
    private string trainingId="varkhan";
    private bool awarded;
    private Battlefield field=null!;
    private Label status=null!;
    private SkillWheel skillBar=null!;
    private HBoxContainer skillActions=null!;
    private Control skillsPanel=null!;
    private PartyHud partyHud=null!;
    private CheckButton repeatTraining=null!;
    private TrainingRun? training;
    private bool preparing;
    private string? pendingSkill;
    private Label skillDescription=null!;
    private EnigmaPanel descriptionFrame=null!;
    public override void _Ready()
    {
        DisplayServer.WindowSetTitle("Reality Of Enigma");
        ApplyVisualTheme();LoadPreferences();
        savePath=ProjectSettings.GlobalizePath("user://save-v1.json");
        var margin=new MarginContainer();margin.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);AddChild(margin);
        foreach(var s in new[]{"margin_left","margin_top","margin_right","margin_bottom"})margin.AddThemeConstantOverride(s,24);
        var box=new VBoxContainer();margin.AddChild(box);
        var title=new Label{Text="P R O J E C T   V A E L O R N"};title.AddThemeFontSizeOverride("font_size",27);box.AddChild(title);
        title.AddThemeColorOverride("font_color",new Color("e4c893"));title.Visible=false;
        summary=new Label{Visible=false};box.AddChild(summary);
        var nav=new HFlowContainer();box.AddChild(nav);
        var row=new HFlowContainer{Visible=false};box.AddChild(row);
        Button(nav,"Início",ShowHomeMenu);Button(nav,"Campanha",ShowCampaignMenu);Button(row,"Treinar Varkhan",()=>Start("training","varkhan"));Button(row,"Treinar Solarius",()=>Start("training","solarius"));
        Button(nav,"Batalha de demonstração",()=>Start("demo"));Button(nav,"Prévia do elenco",()=>Start("cast"));Button(nav,"Fenrath • teste",()=>Start("varkas"));Button(nav,"Velmora • teste",()=>Start("floral"));Button(nav,"Maltherion • teste",()=>Start("eclipse"));Button(row,"Teste Kael VIII",()=>Start("sandbox"));Button(row,"Avançar / ação automática",Step);Button(row,"Auto / parar",()=>{auto=!auto;Refresh();});
        repeatTraining=new CheckButton{Text="Repetir treino ao vencer"};row.AddChild(repeatTraining);
        var economy=new HFlowContainer{Visible=false};box.AddChild(economy);
        Button(economy,"Invocar 1 • 1.000",()=>Invoke(1));Button(economy,"Invocar 10 • 9.000",()=>Invoke(10));Button(economy,"Coletar idle",()=>{game.Collect(DateTimeOffset.UtcNow);Write("Idle coletado");Persist();});
        Button(economy,"Simular Varkhan • 1 ticket",()=>{Write(game.TrainingReward("varkhan",true,true)?"Simulação concluída":"Requer vitória no treino e ticket");Persist();});
        var progression=new HFlowContainer{Visible=false};box.AddChild(progression);
        selection=new OptionButton();progression.AddChild(selection);
        Button(progression,"+1 nível",()=>{Write(game.LevelUp(Selected())?"Nível aumentado":"XP insuficiente ou limite de estrela");Persist();});
        Button(progression,"+1 estrela",()=>{Write(game.Promote(Selected())?"Estrela aumentada":"Requer nível máximo e cópia/Coringa");Persist();});
        Button(progression,"Reconstruir • 10 frag.",()=>{Write(game.Reconstruct(Selected())?"Cópia reconstruída":"Fragmentos insuficientes");Persist();});
        Button(progression,"Kael Lutador",()=>{Write(game.ChangePath("martial")?"Kael agora Lutador":"Requer Kael nível 30");Persist();});
        Button(progression,"Kael Arsenal",()=>{Write(game.ChangePath("arsenal")?"Arsenal: kit base provisório":"Requer Kael nível 30");Persist();});
        Button(progression,"Usar no slot 5",()=>{var id=Selected();if(!game.Save.Roster.Any(x=>x.Id==id)||game.Save.Team.Take(4).Contains(id)){Write("Selecione um personagem recrutado fora dos quatro primeiros slots.");return;}game.Save.Team[4]=id;Persist();});
        Button(nav,"Treinamento",ShowTrainingMenu);
        Button(nav,"AKUROTH • desafio",()=>Start("akuroth"));
        Button(nav,"Invocação",()=>{economy.Visible=!economy.Visible;row.Visible=false;progression.Visible=false;units.Visible=false;});
        Button(nav,"Fichas",ShowCharacterCards);
        Button(nav,"Equipe",()=>{progression.Visible=!progression.Visible;units.Visible=progression.Visible;economy.Visible=false;row.Visible=false;});
        field=new Battlefield{SizeFlagsVertical=SizeFlags.ExpandFill};box.AddChild(field);field.UnitSelected+=_=>Refresh();
        field.FormationChanged+=()=>{if(battle==null)return;if(mode is "demo" or "campaign" or "training"){game.Save.FormationCells=battle.Allies.Select(u=>u.Layer*3+u.Column).ToList();Persist();}Refresh();};
        partyHud=new PartyHud{Field=field};box.AddChild(partyHud);
        var playback=new HFlowContainer();box.AddChild(playback);
        Button(playback,"▶  Próxima ação",Step);Button(playback,"▶ / Ⅱ  Automático",()=>{auto=!auto;Refresh();});
        Button(playback,"Registro",()=>log.Visible=!log.Visible);
        skillsPanel=new Control{MouseFilter=MouseFilterEnum.Ignore};field.AddChild(skillsPanel);
        skillsPanel.SetAnchorsAndOffsetsPreset(LayoutPreset.CenterBottom);
        skillsPanel.OffsetLeft=-300;skillsPanel.OffsetRight=300;skillsPanel.OffsetTop=-355;skillsPanel.OffsetBottom=-8;
        var skillLayout=new VBoxContainer{MouseFilter=MouseFilterEnum.Ignore,Alignment=BoxContainer.AlignmentMode.End};skillLayout.AddThemeConstantOverride("separation",4);skillsPanel.AddChild(skillLayout);skillLayout.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
        var actingPortrait=new ActingPortrait{Field=field,SizeFlagsHorizontal=SizeFlags.ShrinkCenter};skillLayout.AddChild(actingPortrait);
        skillDescription=new Label{Text="Escolha uma habilidade",HorizontalAlignment=HorizontalAlignment.Center,AutowrapMode=TextServer.AutowrapMode.WordSmart,CustomMinimumSize=new Vector2(380,54)};
        skillDescription.AddThemeFontSizeOverride("font_size",18);skillDescription.AddThemeColorOverride("font_color",Colors.White);skillDescription.AddThemeColorOverride("font_shadow_color",Colors.Black);skillDescription.AddThemeConstantOverride("shadow_outline_size",5);descriptionFrame=new EnigmaPanel{Name="SkillDescriptionFrame",MouseFilter=MouseFilterEnum.Ignore,SizeFlagsHorizontal=SizeFlags.ShrinkCenter,CustomMinimumSize=new Vector2(420,0)};skillLayout.AddChild(descriptionFrame);descriptionFrame.AddChild(skillDescription);
        skillBar=new SkillWheel{MouseFilter=MouseFilterEnum.Ignore};skillLayout.AddChild(skillBar);
        skillActions=new HBoxContainer{Alignment=BoxContainer.AlignmentMode.Center,MouseFilter=MouseFilterEnum.Ignore};skillLayout.AddChild(skillActions);

        status=new Label();box.AddChild(status);
        notice=new Label{Text="",TextOverrunBehavior=TextServer.OverrunBehavior.TrimEllipsis};notice.AddThemeColorOverride("font_color",new Color("d8b474"));box.AddChild(notice);
        var effects=new HFlowContainer{Visible=false};box.AddChild(effects);
        Button(effects,"Teste: stun no selecionado",()=>ApplyControl("stun"));
        Button(effects,"Teste: +1 gelo",()=>ApplyControl("ice"));
        Button(effects,"Teste: congelar",()=>ApplyControl("freeze"));
        effects.AddChild(new Label{Text="Clique em um combatente. Ferramentas apenas na Arena de efeitos."});
        units=new Label{Visible=false};units.CustomMinimumSize=new Vector2(0,42);units.AddThemeFontSizeOverride("font_size",12);box.AddChild(units);
        log=new RichTextLabel{Visible=false,CustomMinimumSize=new Vector2(0,100),ScrollFollowing=true};box.AddChild(log);
        try
        {
            var catalog=Catalog.Load(Godot.FileAccess.GetFileAsString("res://Data/catalog.json"));
            game=new Game(catalog,IsQa?Game.NewSave():Persistence.Read(savePath));
            foreach(var h in catalog.Characters.Where(c=>!c.Id.StartsWith("enemy")))selection.AddItem(h.Name);
            Refresh();Write("Clique no alvo e depois em uma habilidade do personagem da vez.\nAvançar resolve automaticamente uma ação; Auto reproduz o combate. Arena de efeitos não concede recompensas.\nBanner: Kael, Savor, Aelia, Brakk e Lyra — 20% cada. Kits dos demais ainda parciais.");
            Start("demo");
            if(OS.GetCmdlineUserArgs().Contains("--qa-akuroth"))CaptureAkurothQa();else if(OS.GetCmdlineUserArgs().Contains("--qa-v30"))CaptureV30Qa();else if(OS.GetCmdlineUserArgs().Contains("--qa-v29"))CaptureV29Qa();else if(OS.GetCmdlineUserArgs().Contains("--qa-v26"))CaptureV26Qa();else if(OS.GetCmdlineUserArgs().Contains("--qa-v25"))CaptureV25Qa();else if(OS.GetCmdlineUserArgs().Contains("--qa-v24"))CaptureV24Qa();else if(OS.GetCmdlineUserArgs().Contains("--qa-v23"))CaptureV23Qa();else if(IsQa)CaptureQa();else ShowHomeMenu();
        }
        catch(Exception ex){Write("Não foi possível carregar. Nenhum save será sobrescrito. "+ex.Message);SetProcess(false);foreach(var r in new[]{row,economy,progression})foreach(var child in r.GetChildren())if(child is BaseButton b)b.Disabled=true;}
    }
    private void Button(Container parent,string name,Action action)
    {var b=new Button{Text=name};b.Pressed+=()=>{try{action();}catch(Exception ex){Write(ex.Message);}};parent.AddChild(b);}
    private string Selected()=>game.Catalog.Characters.Where(c=>!c.Id.StartsWith("enemy")).ElementAt(selection.Selected).Id;
    private void Write(string text){log.AppendText(text+"\n");notice.Text=text.Split('\n').Last();}
    private void Persist(){if(!IsQa)Persistence.Write(savePath,game.Save);Refresh();}
    private void ApplyControl(string effect)
    {
        if(mode!="demo"||battle==null||battle.Finished){Write("Controles de teste disponíveis apenas em uma Arena de efeitos ativa.");return;}
        var target=field.Selected;if(target==null||!target.Alive){Write("Selecione um combatente vivo.");return;}
        if(effect=="stun")Write(target.ApplyStun()?target.Definition.Name+": stun por uma ação":target.Definition.Name+": chefe imune a stun (regra provisória)");
        else{target.ApplyIce(effect=="freeze"?5:1);Write(target.Definition.Name+": gelo aplicado, respeitando imunidade e resistência de chefe.");}
        Refresh();
    }
    private void Invoke(int count){var ids=game.Summon(count);Write(ids.Length==0?"Gold insuficiente":string.Join(", ",ids));Persist();}
    private int encounterStage;
    private bool encounterReplay,campaignResultPending;
    private void StartCampaignStage(int stage)=>Start("campaign",stageNumber:stage);
    private void Start(string kind,string bossId="varkhan",int? stageNumber=null)
    {
        if(kind=="campaign"){
            int requested=stageNumber??game.Save.CompletedStage+1;
            if(!game.CanEnterStage(requested)){ShowCampaignMenu();return;}
            encounterStage=requested;encounterReplay=requested<=game.Save.CompletedStage;
        }
        campaignResultPending=false;
        if(kind=="campaign"&&(!game.Save.OpeningSeen||!game.Save.ForestEncounterSeen)&&game.Save.CompletedStage==0){ShowOpening(true);return;}
        CloseMenu();pendingSkill=null;
        if(kind=="training"&&!game.Save.Training.Contains(bossId)){Write("Treino bloqueado: conclua o marco na campanha.");return;}
        field.SetForest(kind=="story"||kind=="campaign"&&encounterStage<=10);
        mode=kind;trainingId=bossId;awarded=false;auto=false;training=null;
        var cells=Formation.Normalize(game.Save.FormationCells,game.Save.Team.Count);
        var allies=game.Save.Team.Select(id=>game.Save.Roster.Single(o=>o.Id==id)).Select((o,i)=>new Unit(game.Catalog.Hero(o.Id),o.Level,false,cells[i]%3,cells[i]/3,o.Path)).ToList();
        List<Unit> enemies;
        if(kind=="demo")
        {
            allies=game.Save.Team.Select(id=>new OwnedCharacter{Id=id,Level=20}).Select((o,i)=>new Unit(game.Catalog.Hero(o.Id),o.Level,false,cells[i]%3,cells[i]/3)).ToList();
            enemies=new(){new Unit(game.Catalog.Hero("varkhan"),20,true,0,0)};
            var sentinel=game.Catalog.Hero("enemy_fenda") with {Name="Eco da Fenda",Stats=new Stats(2100,80,40,20,90)};
            for(int i=1;i<5;i++)enemies.Add(new Unit(sentinel,20,false,i%3,i/3));
            enemies[1].ApplyStun();enemies[2].ApplyIce(5);enemies[3].ApplyIce(3);
        }
        else if(kind=="varkas"){
            allies=new(){new Unit(game.Catalog.Hero("varkas"),30,false,1,0),new Unit(game.Catalog.Hero("aelia"),30,false,0,1)};
            enemies=Enumerable.Range(0,3).Select(i=>new Unit(game.Catalog.Hero("enemy_fenda") with {Stats=new Stats(20000,25,10,15,70)},20,i==0,i,0)).ToList();
        }
        else if(kind=="sevrin"){
            allies=new(){new Unit(game.Catalog.Hero("sevrin"),30,false,1,0),new Unit(game.Catalog.Hero("aelia"),30,false,0,1)};
            enemies=Enumerable.Range(0,3).Select(i=>new Unit(game.Catalog.Hero("enemy_fenda") with {Stats=new Stats(20000,25,10,15,70)},20,false,1,i)).ToList();
        }
        else if(kind=="eclipse"){
            allies=new[]{"sevrin","kael","aelia","brakk","lyra"}.Select((id,i)=>new Unit(game.Catalog.Hero(id),60,false,cells[i]%3,cells[i]/3)).ToList();
            enemies=new(){new Unit(game.Catalog.Hero("maltherion"),60,true,1,1)};
        }
        else if(kind=="akuroth")
        {
            allies=new[]{"sevrin","kael","aelia","brakk","lyra"}.Select((id,i)=>new Unit(game.Catalog.Hero(id),70,false,cells[i]%3,cells[i]/3)).ToList();
            enemies=new(){new Unit(game.Catalog.Hero("akuroth"),70,true,1,1)};
        }
        else if(kind=="cast")
        {
            allies=new[]{"savor","aelia","brakk","lyra","raizen"}.Select((id,i)=>new Unit(game.Catalog.Hero(id),20,false,cells[i]%3,cells[i]/3)).ToList();
            enemies=new(){new Unit(game.Catalog.Hero("varkhan"),20,true,0,0),new Unit(game.Catalog.Hero("solarius"),20,true,1,0)};
        }
        else if(kind=="sandbox")
        {
            allies=new(){new Unit(game.Catalog.Hero("kael"),70,false,0,0,"martial")};allies[0].OpenGate(8);
            enemies=new(){new Unit(game.Catalog.Hero("solarius"),70,true)};
        }
        else if(kind=="floral")
        {
            allies=new[]{"kael","savor","aelia","brakk","lyra"}.Select((id,i)=>new Unit(game.Catalog.Hero(id),50,false,cells[i]%3,cells[i]/3)).ToList();
            enemies=new(){new Unit(game.Catalog.Hero("nerathis"),50,true,1,1)};
        }
        else if(kind=="training")
        {
            training=game.BeginTraining(bossId);if(training==null){Write("Treino indisponível");return;}
            battle=training.Current;field.Bind(battle);PrepareEncounter();Write("Treinamento: onda 1/3. HP e estados persistem entre ondas.");Refresh();return;
        }
        else
        {
            var stage=game.Catalog.Stages.SingleOrDefault(s=>s.Number==(kind=="story"?1:encounterStage));
            if(stage==null){Write("Fim da campanha de laboratório");return;}
            if(stage.Number<=10){
                // Narrative companions are temporary: do not unlock Raizen or replace the saved roster.
                allies=new[]{"kael","savor","aelia","brakk","raizen"}.Select((id,i)=>new Unit(game.Catalog.Hero(id),kind=="story"?1:game.Save.Roster.FirstOrDefault(o=>o.Id==id)?.Level??game.Save.CampaignGuests.FirstOrDefault(o=>o.Id==id)?.Level??1,false,cells[i]%3,cells[i]/3)).ToList();
            }
            enemies=stage.Enemies.Select((e,i)=>new Unit(game.Catalog.Hero(e.CharacterId),e.Level,e.IsBoss,e.CharacterId is "nerathis" or "maltherion"?1:i%3,e.CharacterId is "nerathis" or "maltherion"?1:i/3)).ToList();
        }
        battle=new Battle(game.Catalog,allies,enemies);field.Bind(battle);PrepareEncounter();Write(preparing?"Organize a formação: arraste um aliado ou clique nele e depois na casa desejada.":"Novo encontro: "+(kind=="varkas"?"Fenrath":kind=="floral"?"Velmora":kind));Refresh();
    }
    private void PrepareEncounter(){preparing=!IsQa;field.FormationEditing=false;if(preparing)field.BeginEntrance();else battle?.LockFormation();}
    private void BeginCombat(){pendingSkill=null;if(battle==null||field.EntranceActive)return;preparing=false;field.FormationEditing=false;battle.LockFormation();field.SelectUnit(battle.Enemies.First());elapsed=0;Write("Combate iniciado. Escolha um alvo e uma habilidade.");Refresh();}
    private void Step()
    {PlayAction(null);}
    private void PlayAction(string? skill)
    {
        if(battle==null)return;
        if(preparing){auto=false;Write("Organize sua formação e clique em Iniciar combate.");Refresh();return;}
        if(field.IsAnimating&&!IsQa)return;
        if(battle.Finished)
        {
            if(training!=null&&training.NextWave())
            {battle=training.Current;field.Bind(battle);Write($"Onda {training.Wave}/3");Refresh();return;}
            if(training!=null&&training.Won&&awarded&&repeatTraining.ButtonPressed&&auto)
            {Start("training",trainingId);BeginCombat();auto=true;Refresh();return;}
            auto=false;Refresh();return;
        }
        int before=battle.Log.Count;var hpBefore=battle.AlliedTargets.Concat(battle.EnemyTargets).ToDictionary(u=>u,u=>u.Hp);
        if(skill!=null)
        {
            auto=false;
            if(!battle.TryAct(skill,field.Selected,out var error)){Write(error);Refresh();return;}
        }
        else battle.Advance();
        field.Animate(hpBefore);foreach(var line in battle.Log.Skip(before))Write(line);
        if(battle.Finished&&!awarded)
        {
            if(training!=null&&battle.Won&&training.Wave<3)
            {Write($"Onda {training.Wave} vencida. Avance para a próxima; recompensas apenas após o chefe.");Refresh();return;}
            awarded=true;
            if(training==null||!training.Won||!repeatTraining.ButtonPressed)auto=false;
            if(mode=="campaign"){game.RewardCampaignEncounter(encounterStage,battle.Won,DateTimeOffset.UtcNow,battle.Allies.Select(u=>u.Id));if(battle.Won)Write(game.LastCampaignReward);campaignResultPending=true;}
            if(mode=="training"&&training!=null)Write(training.ClaimReward()?"Três ondas vencidas: Gold, XP e fragmentos concedidos.":"Treino encerrado sem recompensa.");
            if(mode is "campaign" or "training")Persist();
        }
        Refresh();
    }
    public override void _Process(double delta){
        descriptionFrame.Visible=!string.IsNullOrWhiteSpace(skillDescription.Text);
        skillsPanel.Visible=!field.EntranceActive&&!field.ExitActive&&!(mode=="campaign"&&battle?.Finished==true);
        if(preparing&&!field.EntranceActive&&!field.FormationEditing){field.FormationEditing=true;Refresh();}
        if(campaignResultPending&&!field.IsAnimating&&menuOverlay==null){campaignResultPending=false;ShowCampaignOutcome();}
        if(!auto)return;elapsed+=delta;if(elapsed>1.75){elapsed=0;Step();}
    }
    public override void _UnhandledKeyInput(InputEvent e)
    {if(e is InputEventKey k&&k.Pressed&&k.Keycode==Key.Escape){auto=false;Refresh();}}
    private static void VerifyFrameTimeline()
    {
        void Check(bool ok){if(!ok)throw new InvalidOperationException("QA: sequência de quadros");}
        Check(Battlefield.PoseFrame(true,false,true,.3,10,0,0)==3);
        Check(Battlefield.PoseFrame(true,false,true,.65,10,0,0)==4);
        Check(Battlefield.PoseFrame(true,false,true,.85,10,0,0)==5);
        Check(Battlefield.PoseFrame(true,false,false,0,.05,0,0)==8);
        Check(Battlefield.PoseFrame(true,false,false,0,.22,0,0)==9);
        Check(Battlefield.PoseFrame(false,false,false,0,10,.3,0)==10);
        Check(Battlefield.PoseFrame(false,false,false,0,10,3,0)==11);
        Check(Battlefield.PoseFrame(true,true,false,0,10,0,.4)==0);
        GD.Print("QA_FRAME_ATTACK_HIT_DEFEAT_FREEZE_OK");
    }
    private void SaveQaImage(string path){using var capture=GetViewport().GetTexture().GetImage();capture.SavePng(path);}
    private async void CaptureQa()
    {
        Start("demo");
        VerifyFrameTimeline();
        PlayAction("rush");
        if(battle!.NextActor?.Id!="lyra")throw new InvalidOperationException("QA: esperado turno de Lyra após Kael");
        await ToSignal(GetTree(),SceneTree.SignalName.ProcessFrame);
        await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
        var args=OS.GetCmdlineUserArgs();var index=Array.IndexOf(args,"--qa");
        if(index+1<args.Length)SaveQaImage(args[index+1]);
        await ToSignal(GetTree().CreateTimer(.78),SceneTreeTimer.SignalName.Timeout);
        await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
        if(index+1<args.Length)SaveQaImage(args[index+1]+"-kael.png");
        PlayAction("prison");
        if(!field.IsAnimating)throw new InvalidOperationException("QA: animação não iniciou");
        await ToSignal(GetTree().CreateTimer(.92),SceneTreeTimer.SignalName.Timeout);
        await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
        if(index+1<args.Length)SaveQaImage(args[index+1]+"-ice.png");
        await ToSignal(GetTree().CreateTimer(.85),SceneTreeTimer.SignalName.Timeout);
        if(field.IsAnimating)throw new InvalidOperationException("QA: animação não terminou");
        GD.Print("QA_ANIMATION_TIMELINE_OK");
        for(int n=0;n<30&&!battle!.Finished;n++)Step();
        game.Save.Training.Add("varkhan");
        foreach(var unit in game.Save.Roster){unit.Level=70;unit.Stars=6;}
        Start("training","varkhan");
        for(int n=0;n<600&&training!=null&&!training.RewardClaimed;n++)Step();
        if(training==null||!training.RewardClaimed||training.Wave!=3)throw new InvalidOperationException("QA: treino completo não concedeu recompensa");
        var fragments=game.Save.Inventory.Fragments["varkhan"];Step();
        if(game.Save.Inventory.Fragments["varkhan"]!=fragments)throw new InvalidOperationException("QA: recompensa duplicada");
        repeatTraining.ButtonPressed=true;auto=true;Step();
        for(int n=0;n<600&&!training!.RewardClaimed;n++)Step();
        if(game.Save.Inventory.Fragments["varkhan"]!=fragments+1)throw new InvalidOperationException("QA: repetição não concedeu nova recompensa");
        auto=false;Step();
        if(!training!.RewardClaimed)throw new InvalidOperationException("QA: repetição não parou");
        System.Collections.Generic.IEnumerable<Node> Walk(Node node){foreach(var child in node.GetChildren()){yield return child;foreach(var descendant in Walk(child))yield return descendant;}}
        foreach(var menu in new[]{"Equipe","Invocação","Treinamento"}){
            var button=Walk(this).OfType<Button>().Single(b=>b.Text==menu);
            button.EmitSignal(BaseButton.SignalName.Pressed);
            await ToSignal(GetTree(),SceneTree.SignalName.ProcessFrame);
            await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
            if(status.GetGlobalRect().End.Y>GetViewportRect().Size.Y)throw new InvalidOperationException("QA: menu excede a tela: "+menu);
            if(menu=="Equipe"&&index+1<args.Length)SaveQaImage(args[index+1]+"-team.png");
            button.EmitSignal(BaseButton.SignalName.Pressed);
        }
        GD.Print("QA_NAVIGATION_LAYOUT_OK");
        if(index+1<args.Length){
            await CaptureEffectQa("lyra","prison",args[index+1]+"-freeze-timing.png");
            await CaptureEffectQa("aelia","heal",args[index+1]+"-heal.png");
            await CaptureEffectQa("varkhan","horizon",args[index+1]+"-horizon.png");
            await CapturePresentationQa(args[index+1]);
            await CaptureBuffQa(args[index+1]);
            await CaptureFormationQa(args[index+1]);
            await CaptureStatesQa(args[index+1]);
            ShowHomeMenu();
            await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);SaveQaImage(args[index+1]+"-home.png");
            ShowCampaignMenu();
            await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);SaveQaImage(args[index+1]+"-campaign.png");
            CloseMenu();await CaptureOpeningQa(args[index+1]);await CaptureV22Qa(args[index+1]);ShowCharacterCards();
            await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);SaveQaImage(args[index+1]+"-card.png");
        }
        GD.Print("QA_MANUAL_LYRA_OK TRAINING_3_WAVES_OK REWARD_ONCE_OK REPEAT_STOP_OK");GetTree().Quit();
    }
    private async System.Threading.Tasks.Task CaptureEffectQa(string hero,string ability,string file)
    {
        auto=false;training=null;awarded=false;mode="demo";
        var caster=new Unit(game.Catalog.Hero(hero),70,false,0,0);caster.Hp=caster.MaxHp*.45;
        var dummy=game.Catalog.Hero("enemy_fenda") with {Stats=new Stats(15000,1,1,15,1)};
        battle=new Battle(game.Catalog,new[]{caster},Enumerable.Range(0,3).Select(n=>new Unit(dummy,20,false,0,n)));
        if(ability=="prison")foreach(var enemy in battle.Enemies)enemy.ApplyIce(4);
        caster.ReadyRound[ability]=0;field.Bind(battle);
        if(ability=="horizon"){
            ChooseSkillTarget(game.Catalog.Ability(ability));
            if(pendingSkill!=ability||battle.LastActor!=null)throw new Exception("QA habilidade executou antes da escolha do alvo");
            pendingSkill=null;
        }
        var hp=battle.AlliedTargets.Concat(battle.EnemyTargets).ToDictionary(u=>u,u=>u.Hp);
        if(!battle.TryAct(ability,ability=="heal"?caster:battle.Enemies[^1],out var error))throw new InvalidOperationException("QA efeito: "+error);
        field.Animate(hp);Refresh();
        if(ability=="prison"&&(battle.Enemies[^1].Freeze==0||field.VisualFrozen(battle.Enemies[^1])))throw new Exception("QA congelamento antecipado");
        await ToSignal(GetTree().CreateTimer(.84),SceneTreeTimer.SignalName.Timeout);
        await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
        SaveQaImage(file);
        if(ability=="prison"&&!field.VisualFrozen(battle.Enemies[^1]))throw new Exception("QA gelo ausente após impacto");
        if(ability=="heal"&&caster.Hp<=hp[caster])throw new InvalidOperationException("QA cura");
        if(ability=="horizon"&&(!battle.LastImpacts.Select(i=>i.Target.Layer).SequenceEqual(new[]{0,1,2})||battle.Enemies.Any(u=>u.Hp>=hp[u])))throw new InvalidOperationException("QA linha perfurante");
        GD.Print("QA_EFFECT_"+ability.ToUpperInvariant()+"_OK");
    }
    private async System.Threading.Tasks.Task CapturePresentationQa(string file)
    {
        // Find a real seeded critical instead of synthesizing a cosmetic one.
        for(int seed=0;seed<100;seed++){
            var hero=new Unit(game.Catalog.Hero("kael"),20);
            var target=new Unit(game.Catalog.Hero("enemy_fenda") with {Stats=new Stats(50000,1,1,0,1)},20);
            battle=new Battle(game.Catalog,new[]{hero},new[]{target},seed);
            var before=battle.AlliedTargets.Concat(battle.EnemyTargets).ToDictionary(u=>u,u=>u.Hp);
            battle.TryAct("slash",target,out _);
            if(!battle.LastImpacts.Any(h=>h.Critical))continue;
            field.Bind(battle);field.Animate(before);Refresh();
            await ToSignal(GetTree().CreateTimer(.85),SceneTreeTimer.SignalName.Timeout);
            await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
            SaveQaImage(file+"-critical.png");break;
        }
        var ids=new[]{"kael","savor","aelia","brakk","lyra"};
        var allies=ids.Select((id,i)=>new Unit(game.Catalog.Hero(id) with {Archetypes=new[]{"fighter"}},70,false,i%2,i/2)).ToArray();
        var enemies=new[]{new Unit(game.Catalog.Hero("varkhan"),20,true,0,0),new Unit(game.Catalog.Hero("enemy_fenda"),20,false,1,0)};
        battle=new Battle(game.Catalog,allies,enemies);field.Bind(battle);
        var hp=allies.Concat(enemies).ToDictionary(u=>u,u=>u.Hp);
        foreach(var unit in new[]{allies[1],enemies[0],enemies[1]}){
            double amount=unit.LoseHp(unit.Hp);
            battle.LastImpacts.Add(new CombatImpact(unit,amount,"physical",false,0));
        }
        field.Animate(hp);Refresh();
        await ToSignal(GetTree().CreateTimer(2.5),SceneTreeTimer.SignalName.Timeout);
        await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
        SaveQaImage(file+"-defeat-buffs.png");
        if(!enemies[0].SurvivesDefeat||enemies[1].SurvivesDefeat||battle.Enemies.Count!=2)throw new InvalidOperationException("QA: regras de derrota");
        GD.Print("QA_CRITICAL_BUFFS_PERSISTENT_DEFEAT_OK");
    }
    private void Refresh()
    {
        summary.Text=$"Gold {game.Save.Inventory.Gold:F0}   XP {game.Save.Inventory.Xp:F0}   Fase {game.Save.CompletedStage}/70   Tickets {game.Save.Inventory.Tickets}   Treinos: {string.Join(", ",game.Save.Training)}";
        var roster=string.Join(" | ",game.Save.Roster.Select(o=>$"{o.Id} Nv{o.Level} {o.Stars}★ cópias:{o.Copies}"));
        units.Text=roster+"\nFragmentos: "+string.Join(" / ",game.Save.Inventory.Fragments.Select(p=>$"{p.Key}: {p.Value}"));
        if(battle!=null)
        {
            var u=field.Selected;
            status.Text=$"{(preparing?"FORMAÇÃO 3×3":mode is "demo" or "cast" or "story"?"SIMULAÇÃO SEM RECOMPENSAS":mode=="campaign"?"CAMPANHA":mode=="varkas"?"FENRATH • TESTE":mode=="floral"?"VELMORA • TESTE":mode.ToUpperInvariant())} • Rodada {battle.Round} • {(auto?"AUTO":"PAUSADO")} • {battle.LastAction}";
            if(training!=null)status.Text+=$" • ONDA {training.Wave}/3";
            if(u!=null)status.TooltipText=$"\nAlvo: {u.Definition.Name} • gelo {u.Ice}/5 | congelamento {u.Freeze} | stun {u.Stun} | imunidade {u.Immunity} | slow {Math.Max(u.SlowTurns,u.TechniqueSlowTurns)} | velocidade {u.Speed:F0}";
            if(battle.Finished)status.Text+=" • "+(battle.Won?"VITÓRIA":"DERROTA");
        }
        RefreshSkills();
    }
    private string DescribeSkill(Skill s)=>s.Description!=null?s.Name+"\n"+s.Description+$"\nRecarga: {s.Cooldown} rodada(s)":s.Id switch {
 "duality"=>"Dualidade Feral\nRequer 70% de vida • 1 uso\nCorpo 60% HP / Espírito 40% HP • cópia: 60% dano",
 "gale_fangs"=>"Presas do Vendaval\n5 golpes aleatórios de 55% Força\nSangramento: 60% • Recarga 4",
 "marked"=>"Marcado para Morrer\n180% Força +10% por Sangramento\nNão consome cargas • Recarga 4",
 "lacerate"=>"Dilacerar • Nível 30\n250% Força • Ferida Exposta: 2 turnos\n+25 pontos de chance de Sangramento • Recarga 5",
 _=>DescribeBaseSkill(s)};
    private string DescribeBaseSkill(Skill s)=>s.Name+"\n"+(s.Operator=="buff"?"Concede bônus aos aliados.":s.Operator=="heal"?$"Restaura {s.Multiplier:P0} de Essência.":s.Operator=="horizon"?"Perfura 3 camadas; acumula o dano anterior.":$"{s.Multiplier:P0} de {(s.Scale=="essence"?"Essência":"Força")} • {s.Hits} golpe(s).")+$"\nRecarga: {s.Cooldown} rodada(s)"+(s.Ice>0?$" • Gelo +{s.Ice}":"")+(s.Stun>0?" • Atordoa":"");
    private void ChooseSkillTarget(Skill skill)
    {
        if(field.IsAnimating){Write("Aguarde o movimento terminar.");return;}
        auto=false;
        skillDescription.Text=DescribeSkill(skill);
        
        pendingSkill=skill.Id;field.SetTargetIntent(skill);Write("Clique no alvo na arena e confirme abaixo dos ícones. Lança do Horizonte percorre a coluna desde a frente.");Refresh();
    }
    private void RefreshSkills()
    {
        foreach(var container in new Container[]{skillBar,skillActions})foreach(var child in container.GetChildren()){container.RemoveChild(child);child.QueueFree();}
        if(preparing&&field.EntranceActive){skillDescription.Text="O ENCONTRO • Prepare sua equipe";return;}
        if(preparing){skillDescription.Text="FORMAÇÃO 3×3 • Arraste os aliados para os círculos.";Button(skillActions,"Iniciar combate",BeginCombat);return;}
        var actor=battle?.NextActor;
        if(actor==null){skillDescription.Text=training!=null&&training.Wave<3&&battle!.Won?"Onda vencida: clique em Avançar.":"Encontro encerrado.";return;}
        field.SetTargetIntent(pendingSkill==null?null:game.Catalog.Ability(pendingSkill));
        if(!battle!.Allies.Contains(actor)){skillDescription.Text="";return;}
        skillDescription.Text="";
        foreach(var ability in battle.SkillsFor(actor)){
            string reason=battle.UnavailableReason(actor,ability);
            var button=new SkillIconButton{Ability=ability,Disabled=reason!="",TooltipText=DescribeSkill(ability)+(reason!=""?"\n"+reason:"")};
            button.AddThemeStyleboxOverride("normal",new StyleBoxEmpty());button.AddThemeStyleboxOverride("disabled",new StyleBoxEmpty());
            button.MouseEntered+=()=>{if(pendingSkill==null)skillDescription.Text=DescribeSkill(ability)+(reason!=""?"\n"+reason:"");};
            button.MouseExited+=()=>{if(pendingSkill==null)skillDescription.Text="";};button.Pressed+=()=>ChooseSkillTarget(ability);skillBar.AddChild(button);
        }
        var passive=new SkillIconButton{Passive=true,TooltipText=PassiveDescription(actor)};
        passive.AddThemeStyleboxOverride("normal",new StyleBoxEmpty());
        passive.Pressed+=()=>{pendingSkill=null;RefreshSkills();skillDescription.Text=PassiveDescription(actor);};skillBar.AddChild(passive);
        if(pendingSkill!=null){
            var chosen=battle.SkillsFor(actor).FirstOrDefault(a=>a.Id==pendingSkill);
            if(chosen==null){pendingSkill=null;return;}
            skillDescription.Text=DescribeSkill(chosen);
            bool automaticTarget=chosen.Operator is "buff" or "break" or "aoe" or "split" or "random" or "scarlet_aoe" or "eclipse_aoe" or "eclipse_guard";
            var targets=chosen.Operator=="heal"?battle.AlliedTargets:battle.EnemyTargets;
            var target=field.Selected;
            bool valid=automaticTarget||target!=null&&target.Alive&&targets.Contains(target);
            var confirm=new Button{Text=automaticTarget?"Confirmar habilidade":valid?"Usar em "+target!.Definition.Name:"Clique no alvo na arena",Disabled=!valid};
            confirm.Pressed+=()=>{var id=pendingSkill;pendingSkill=null;PlayAction(id);};skillActions.AddChild(confirm);
            Button(skillActions,"Cancelar",()=>{pendingSkill=null;Refresh();});
        }
    }
    private static string PassiveDescription(Unit u)=>u.Id switch{
        "sevrin"=>"PASSIVA • Rastro Escarlate\nAtaques aplicam Sangramento. Bônus de velocidade e crítico contra alvos sangrando. Na frente, reduz dano de atacantes com mais de70% de vida. Abater alvo sangrando cura15% e reduz recargas em1. Valores V1.",
        "varkas"=>"PASSIVA • Feridas Abertas • ATIVA\nAtaques diretos: 40% de aplicar Sangramento. Cada carga: 15% da Força de origem por 3 turnos. Máximo 10; funciona em chefes.",
        "kael" when u.Path=="martial"=>"PASSIVA • Limites Quebrados • ATIVA\nAbre estágios a cada 2 rodadas e ao cruzar 75%, 50% e 25% de vida. VIII exige nível 70.",
        "kael"=>"PASSIVA • Predador Ágil • EM DESENVOLVIMENTO\nPrevista: dano cresce conforme a vida perdida pelo alvo. Ainda não concede bônus.",
        "savor"=>"PASSIVA • Ladies First • EM DESENVOLVIMENTO\nPrevista: +10% de Força por aliada feminina, até 40%. Ainda não concede bônus.",
        "aelia"=>"PASSIVA • Fios da Vida • EM DESENVOLVIMENTO\nPrevista: cura automática quando um aliado cai abaixo de 30% de vida. Ainda não ativa.",
        "brakk"=>"PASSIVA • Chassi Reforçado • EM DESENVOLVIMENTO\nPrevista: redução de dano físico e cura ao bloquear. Ainda não concede bônus.",
        "lyra"=>"EFEITO AUTOMÁTICO • Gelo • ATIVO\nCada carga reduz 3% de velocidade. Com 5, congela por 1 ação; chefes recebem lentidão. Passiva própria pendente.",
        _=>"PASSIVA • EM DESENVOLVIMENTO\nEste personagem ainda usa um kit parcial. Nenhum bônus de passiva individual está ativo."
    };
}
