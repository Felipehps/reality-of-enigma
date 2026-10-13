using Godot;
using System;

public partial class Main
{
    private Control? menuOverlay;
    private void CloseMenu(){if(menuOverlay!=null){RemoveChild(menuOverlay);menuOverlay.QueueFree();menuOverlay=null;}}
    private VBoxContainer MenuFrame(string heading,string subtitle)
    {
        auto=false;CloseMenu();
        var root=new Control{ZIndex=40,MouseFilter=MouseFilterEnum.Stop};menuOverlay=root;AddChild(root);root.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
        var background=new TextureRect{Texture=GD.Load<Texture2D>(heading=="PROJECT VAELORN"?"res://Art/lobby-v20.png":"res://Art/arena-v17.png"),ExpandMode=TextureRect.ExpandModeEnum.IgnoreSize,StretchMode=TextureRect.StretchModeEnum.KeepAspectCovered,MouseFilter=MouseFilterEnum.Ignore};root.AddChild(background);background.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
        var shade=new ColorRect{Color=new Color(.015f,.03f,.06f,heading=="PROJECT VAELORN"?.22f:.78f),MouseFilter=MouseFilterEnum.Ignore};root.AddChild(shade);shade.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
        var margin=new MarginContainer();root.AddChild(margin);margin.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
        foreach(var e in new[]{"left","right","top","bottom"})margin.AddThemeConstantOverride("margin_"+e,64);
        var layout=new VBoxContainer();layout.AddThemeConstantOverride("separation",18);margin.AddChild(layout);
        var title=new Label{Text=heading};title.AddThemeFontSizeOverride("font_size",42);title.AddThemeColorOverride("font_color",new Color("edcc92"));layout.AddChild(title);
        layout.AddChild(new Label{Text=subtitle});return layout;
    }
    private Control NewMenuRoot(){
        auto=false;field?.CancelEntrance();field?.SilenceActionSounds();CloseMenu();var root=new Control{ZIndex=40,MouseFilter=MouseFilterEnum.Stop};menuOverlay=root;AddChild(root);root.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);return root;
    }
    private static void Place(Control node,float left,float top,float right,float bottom,float inset=0){
        node.AnchorLeft=left;node.AnchorTop=top;node.AnchorRight=right;node.AnchorBottom=bottom;
        node.OffsetLeft=inset;node.OffsetTop=inset;node.OffsetRight=-inset;node.OffsetBottom=-inset;
    }
    private static Label MenuLabel(string text,int size=18){var l=new Label{Text=text,AutowrapMode=TextServer.AutowrapMode.WordSmart};l.AddThemeFontSizeOverride("font_size",size);return l;}
    private void ShowHomeMenu()
    {
        var root=NewMenuRoot();
        var backdrop=new TextureRect{Texture=GD.Load<Texture2D>("res://Art/forest-v22.png"),ExpandMode=TextureRect.ExpandModeEnum.IgnoreSize,StretchMode=TextureRect.StretchModeEnum.KeepAspectCovered,Modulate=new Color(.28f,.32f,.40f),MouseFilter=MouseFilterEnum.Ignore};root.AddChild(backdrop);Place(backdrop,0,0,1,1);
        var banner=new MenuCarousel();root.AddChild(banner);Place(banner,0,0,1,1);
        var shade=new ColorRect{MouseFilter=MouseFilterEnum.Ignore,Material=new ShaderMaterial{Shader=new Shader{Code="shader_type canvas_item; void fragment(){ COLOR=vec4(0.045,0.015,0.085,0.68*(1.0-UV.x*UV.x)); }"}}};root.AddChild(shade);Place(shade,0,0,.65f,1);
        var left=new VBoxContainer();root.AddChild(left);Place(left,.035f,.045f,.405f,.91f);left.AddThemeConstantOverride("separation",13);
        var logo=new TextureRect{Texture=GD.Load<Texture2D>("res://Art/logo-v23.png"),CustomMinimumSize=new Vector2(0,260),ExpandMode=TextureRect.ExpandModeEnum.IgnoreSize,StretchMode=TextureRect.StretchModeEnum.KeepAspectCentered};left.AddChild(logo);
        var subtitle=MenuLabel("UM MUNDO PARTIDO. DESTINOS ENTRELAÇADOS.",13);subtitle.HorizontalAlignment=HorizontalAlignment.Center;left.AddChild(subtitle);
        left.AddChild(new Control{CustomMinimumSize=new Vector2(0,25)});
        void Action(string text,System.Action action){var b=new TentacleButton{Text=text,Alignment=HorizontalAlignment.Left,CustomMinimumSize=new Vector2(0,55)};
            b.AddThemeFontOverride("font",EnigmaIdentity.Heading);b.AddThemeFontSizeOverride("font_size",23);
            b.AddThemeColorOverride("font_color",new Color("dccdb3"));b.AddThemeColorOverride("font_hover_color",new Color("fff1c8"));b.AddThemeColorOverride("font_focus_color",new Color("fff1c8"));
            b.AddThemeColorOverride("font_shadow_color",new Color("090612"));b.AddThemeConstantOverride("shadow_outline_size",4);
            foreach(var state in new[]{"normal","hover","pressed","focus"})b.AddThemeStyleboxOverride(state,new StyleBoxEmpty{ContentMarginLeft=53,ContentMarginTop=12,ContentMarginBottom=12});
            b.MouseEntered+=()=>{if(b.IsInsideTree())b.GrabFocus();};b.Pressed+=action;left.AddChild(b);if(b.IsInsideTree()&&left.GetChildren().OfType<TentacleButton>().Count()==1)b.GrabFocus();}
        Action(game.Save.CompletedStage==0?"COMEÇAR JORNADA":"CONTINUAR JORNADA",()=>Start("campaign"));
        Action("MAPA DA CAMPANHA",ShowCampaignMenu);
        Action("HERÓIS REVELADOS",ShowCharacterCards);
        Action("AKUROTH • DESAFIO",()=>Start("akuroth"));
        if(game.Save.CompletedStage>=10)Action("SALA DE TREINAMENTO",ShowTrainingMenu);
        Action("CONFIGURAÇÕES",ShowSettings);
        Action("SAIR",()=>{Persist();GetTree().Quit();});
        var purse=new GoldPouch{Amount=game.Save.Inventory.Gold};root.AddChild(purse);Place(purse,.80f,.02f,.98f,.105f);

    }
    private int campaignRegion=-1;
    private void ShowCampaignMenu()
    {
        if(campaignRegion<0||campaignRegion>JourneyData.UnlockedRegion(game.Save.CompletedStage))campaignRegion=JourneyData.UnlockedRegion(game.Save.CompletedStage);
        var root=NewMenuRoot();var region=JourneyData.Regions[campaignRegion];
        var map=new CampaignMap{RegionIndex=campaignRegion,Completed=game.Save.CompletedStage,StageChosen=StartCampaignStage};root.AddChild(map);Place(map,0,0,1,1);
        var top=new VBoxContainer();root.AddChild(top);Place(top,.045f,.035f,.86f,.17f);
        var heading=MenuLabel(region.Name.ToUpperInvariant(),32);heading.AddThemeColorOverride("font_color",new Color("ffe1aa"));top.AddChild(heading);top.AddChild(MenuLabel(region.Summary,17));
        var bottom=new VBoxContainer();root.AddChild(bottom);Place(bottom,.055f,.82f,.945f,.99f);
        var hint=MenuLabel("Siga o caminho iluminado • passe sobre um marco para consultar a fase",15);hint.HorizontalAlignment=HorizontalAlignment.Center;bottom.AddChild(hint);
        var tabs=new HBoxContainer{Alignment=BoxContainer.AlignmentMode.Center};bottom.AddChild(tabs);
        for(int i=0;i<7;i++){
            int index=i;bool unlocked=i<=JourneyData.UnlockedRegion(game.Save.CompletedStage);
            var button=new RegionTab{RegionIndex=i,Current=i==campaignRegion,CustomMinimumSize=new Vector2(155,92),Disabled=!unlocked,TooltipText=JourneyData.Regions[i].Name+(unlocked?"":" • Derrote o chefe da região anterior.")};
            if(i==campaignRegion)button.AddThemeColorOverride("font_color",new Color("ffe1a1"));
            button.Pressed+=()=>{campaignRegion=index;ShowCampaignMenu();};tabs.AddChild(button);
        }
        var navigation=new HBoxContainer{Alignment=BoxContainer.AlignmentMode.Center};bottom.AddChild(navigation);Button(navigation,"Voltar ao início",ShowHomeMenu);if(game.Save.CompletedStage>=10)Button(navigation,"Treinamento de chefes",ShowTrainingMenu);if(game.Save.CompletedStage>=(campaignRegion+1)*10)Button(navigation,"Rever história do cenário",()=>ShowRegionEnding(campaignRegion));
    }
    private void ShowTrainingMenu()
    {
        if(game.Save.CompletedStage<10){ShowHomeMenu();return;}
        var layout=MenuFrame("SALA DE TREINAMENTO","Reencontre os chefes liberados na campanha e obtenha fragmentos.");
        foreach(var boss in game.Catalog.Bosses){
            if(!boss.Recruitable)continue;
            string id=boss.Id;bool unlocked=game.Save.Training.Contains(id);
            var button=new Button{Text=game.Catalog.Hero(boss.CharacterId).Name+(unlocked?" • Treinar":" • Bloqueado"),Disabled=!unlocked};
            button.Pressed+=()=>Start("training",id);layout.AddChild(button);
        }
        Button(layout,"Voltar ao início",ShowHomeMenu);
    }
}
