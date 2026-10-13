using Godot;
using System;
using System.Collections.Generic;
using System.Linq;

public partial class Main
{
    private async void CaptureAkurothQa(){
        try{
            var args=OS.GetCmdlineUserArgs();string prefix=args[Array.IndexOf(args,"--qa-akuroth")+1];
            IEnumerable<Node> Walk(Node n){foreach(var child in n.GetChildren()){yield return child;foreach(var descendant in Walk(child))yield return descendant;}}
            async System.Threading.Tasks.Task Capture(string suffix){await ToSignal(RenderingServer.Singleton,RenderingServer.SignalName.FramePostDraw);SaveQaImage(prefix+suffix+".png");}
            ShowHomeMenu();await Capture("-menu");
            var button=Walk(menuOverlay!).OfType<TentacleButton>().Single(b=>b.Text=="AKUROTH • DESAFIO");button.EmitSignal(BaseButton.SignalName.Pressed);
            if(mode!="akuroth"||battle?.Enemies.Single().Id!="akuroth"||battle.Allies.Any(u=>u.Level!=70))throw new Exception("Challenge menu routing failed");
            await Capture("-battle");
            int stage=game.Save.CompletedStage;decimal gold=game.Save.Inventory.Gold;
            while(!battle.Finished)Step();
            Step();await ToSignal(GetTree().CreateTimer(2),SceneTreeTimer.SignalName.Timeout);await Capture("-result");
            if(!battle.Won||!battle.Enemies[0].Ascended||stage!=game.Save.CompletedStage||gold!=game.Save.Inventory.Gold||game.Save.Roster.Any(u=>u.Level!=1))throw new Exception("Challenge changed save or failed reference fight");
            field.SilenceActionSounds();GD.Print("AKUROTH_MAIN_QA_OK: lobby button, challenge, level 70 team, playable actions, phase 2, victory, no progression rewards");GetTree().Quit();
        }catch(Exception e){GD.PrintErr(e);GetTree().Quit(1);}
    }
}
