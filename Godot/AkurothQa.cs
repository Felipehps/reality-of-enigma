using Godot;
using Vaelorn;
using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

// Isolated presentation smoke test; never reads or writes player saves.
public partial class AkurothQa:Control
{
    private Battlefield field=null!;
    private Catalog catalog=null!;
    private string output="";
    private bool capture;
    private bool recordFrames;
    private bool resumeFrames;
    public override async void _Ready(){
        try{
            var args=OS.GetCmdlineUserArgs();output=args.Length>0?args[0]:ProjectSettings.GlobalizePath("user://akuroth-qa");
            capture=!args.Contains("--no-capture");Directory.CreateDirectory(output);
            recordFrames=args.Contains("--frames");
            resumeFrames=args.Contains("--resume");
            catalog=Catalog.Load(Godot.FileAccess.GetFileAsString("res://Data/catalog.json"));
            field=new Battlefield();AddChild(field);field.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
            await ToSignal(GetTree(),SceneTree.SignalName.ProcessFrame);
            if(!field.AkurothCinematicAssetsReady)throw new Exception("Cinematic assets failed to load");
            var battle=Encounter(false);field.Bind(battle);await Capture("01-phase1");
            var boss=battle.Enemies[0];var before=battle.Allies.Concat(battle.Enemies).ToDictionary(u=>u,u=>u.Hp);
            boss.LoseHp(boss.MaxHp*.7);field.Animate(before);
            if(recordFrames)await CaptureActionFrames("akuroth-transform",4.8f);
            else {await Pause(1.8);await Capture("02-transform");await Pause(3);}
            await Capture("03-phase2");if(!field.AkurothInfernoActive)throw new Exception("Infernal arena transition failed");
            foreach(var skill in catalog.Skills.Where(s=>s.Id.StartsWith("akuroth_"))){
                battle=Encounter(skill.MinimumPhase==2);boss=battle.Enemies[0];field.Bind(battle);
                foreach(var s in battle.SkillsFor(boss))boss.ReadyRound[s.Id]=s.Id==skill.Id?1:999;
                before=battle.Allies.Concat(battle.Enemies).ToDictionary(u=>u,u=>u.Hp);battle.Advance();field.Animate(before);
                if(battle.LastSkill?.Id!=skill.Id)throw new Exception("Wrong skill: "+skill.Id);
                if(recordFrames)await CaptureActionFrames(skill.Id);
                else {await Pause(.7);await Capture(skill.Id+"-cast");await Pause(.22);await Capture(skill.Id+"-impact");}
                if(skill.Id=="akuroth_annihilation"){
                    if(!boss.AnnihilationCharging||battle.LastImpacts.Count!=0)throw new Exception("No telegraph");
                    await Pause(.9);while(battle.NextActor!=boss)battle.Advance();
                    before=battle.Allies.Concat(battle.Enemies).ToDictionary(u=>u,u=>u.Hp);battle.Advance();field.Animate(before);
                    if(recordFrames)await CaptureActionFrames("akuroth-annihilation-discharge");
                    else {await Pause(.7);await Capture("akuroth-annihilation-discharge");}
                }
                var pcm=Battlefield.MakeCue("akuroth-"+skill.Id[8..]+"-cast");
                if(pcm.Data.Length<10000||pcm.MixRate!=44100)throw new Exception("Invalid PCM cue");
                if(pcm.SaveToWav(Path.Combine(output,skill.Id+".wav"))!=Error.Ok)throw new Exception("Cannot export sound");
                pcm.SaveToWav(Path.Combine(output,skill.Id+"-cast.wav"));
                pcm.Dispose();
                pcm=Battlefield.MakeCue("akuroth-"+skill.Id[8..]+"-impact");
                if(!pcm.Stereo)throw new Exception("Cinematic audio must be stereo");
                pcm.SaveToWav(Path.Combine(output,skill.Id+"-impact.wav"));pcm.Dispose();
                await Pause(.9);
            }
            battle=Encounter(false);field.Bind(battle);field.BeginEntrance();await Pause(.45);await Capture("04-entrance");field.CancelEntrance();
            before=battle.Allies.Concat(battle.Enemies).ToDictionary(u=>u,u=>u.Hp);battle.Enemies[0].LoseHp(battle.Enemies[0].MaxHp);field.Animate(before);await Pause(1);await Capture("05-defeat");
            foreach(var cue in new[]{"akuroth-transform","akuroth-impact"}){var pcm=Battlefield.MakeCue(cue);pcm.SaveToWav(Path.Combine(output,cue+".wav"));pcm.Dispose();}
            if(field.AttackSoundsPlayed<10)throw new Exception("Action audio scheduling did not run");
            if(field.ArenaBloodBursts==0||field.ArenaCracksCreated==0)throw new Exception("Arena reactions did not run");
            field.SilenceActionSounds();
            GD.Print($"AKUROTH_QA_OK: phases, transformation, ten skills, charge/discharge, entry, defeat, PCM, audio; {field.ArenaBloodBursts} blood bursts, {field.ArenaCracksCreated} floor impacts");GetTree().Quit();
        }catch(Exception ex){GD.PrintErr(ex);GetTree().Quit(1);}
    }
    private Battle Encounter(bool ascended){
        var allies=new[]{"sevrin","kael","aelia","brakk","lyra"}.Select((id,i)=>new Unit(catalog.Hero(id) with {Stats=catalog.Hero(id).Stats with {Speed=1}},70,column:i%3,layer:i/3)).ToArray();
        var boss=new Unit(catalog.Hero("akuroth") with {Stats=catalog.Hero("akuroth").Stats with {Speed=999}},70,true,1,1);
        if(ascended)boss.LoseHp(boss.MaxHp*.7);
        return new Battle(catalog,allies,new[]{boss});
    }
    private async Task Pause(double seconds)=>await ToSignal(GetTree().CreateTimer(seconds),SceneTreeTimer.SignalName.Timeout);
    private async Task CaptureActionFrames(string name,float? seconds=null){
        int frames=(int)Math.Ceiling((seconds??field.EffectDuration/1.16f+.25f)*24);
        if(resumeFrames&&Enumerable.Range(0,frames).All(i=>File.Exists(Path.Combine(output,name+"-frame-"+i.ToString("D3")+".png")))){field.SetPreviewTime((frames-1)/24.0*1.16);return;}
        for(int i=0;i<frames;i++){field.SetPreviewTime(i/24.0*1.16);await Capture(name+"-frame-"+i.ToString("D3"));}
    }
    private async Task Capture(string name){
        if(!capture)return;
        await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);
        using var image=GetViewport().GetTexture().GetImage();
        if(image.IsEmpty()||image.SavePng(Path.Combine(output,name+".png"))!=Error.Ok)throw new Exception("Capture failed: "+name);
    }
}
