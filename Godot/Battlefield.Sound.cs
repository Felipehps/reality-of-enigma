using Godot;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using Vaelorn;

public partial class Battlefield
{
    private sealed record SoundProfile(string Cast,string Impact,string Step,float Pitch);
    private sealed record SampleDefinition(string File,float Gain=0,float Duration=1);
    private Dictionary<string,SampleDefinition> samples=new();
    private readonly Dictionary<string,AudioStream> sampleStreams=new();
    private readonly Dictionary<AudioStreamPlayer,(double End,float Volume)> voiceEnds=new();
    public int RecordedSoundsPlayed{get;private set;}
    private Dictionary<string,SoundProfile> soundProfiles=new();
    private readonly Dictionary<string,AudioStreamWav> soundCache=new();
    private readonly List<(double At,string Cue,float Pitch,float Gain)> soundEvents=new();
    private readonly List<AudioStreamPlayer> soundVoices=new();
    private double nextStepAt;
    private bool audioPaused;
    public int AttackSoundsPlayed{get;private set;}
    public int FootstepsPlayed{get;private set;}
    public void SilenceActionSounds(){audioPaused=true;soundEvents.Clear();voiceEnds.Clear();foreach(var player in soundVoices)player.Stop();clashAudio?.Stop();evolutionAudio?.Stop();}
    private void LoadSoundProfiles(){
        soundProfiles=JsonSerializer.Deserialize<Dictionary<string,SoundProfile>>(Godot.FileAccess.GetFileAsString("res://Data/audio.json"))!;
        samples=JsonSerializer.Deserialize<Dictionary<string,SampleDefinition>>(Godot.FileAccess.GetFileAsString("res://Data/audio-samples.json"))!;
        foreach(var item in samples){var stream=GD.Load<AudioStream>("res://Audio/"+item.Value.File);if(stream==null)throw new InvalidOperationException("Som ausente: "+item.Value.File);sampleStreams[item.Key]=stream;}
    }
    private SoundProfile SoundFor(Unit u)=>soundProfiles.GetValueOrDefault(u.Id.StartsWith("enemy")?"enemy_fenda":u.Id,new("swing","kick","leather",1));
    private void ScheduleActionSound(){
        soundEvents.Clear();if(Battle?.LastSkill is not {} skill||actor==null)return;
        var profile=soundProfiles.GetValueOrDefault("skill:"+skill.Id,SoundFor(actor));string cast=profile.Cast,impact=profile.Impact;
        if(actor.Id=="aelia"&&skill.Operator!="heal")impact="chime";
        if(skill.Operator=="heal"){cast="chime";impact="heal";}
        else if(skill.Operator=="buff"){cast="chime";impact=actor.Id=="savor"?"sizzle":"heal";}
        else if(skill.Operator=="split"){cast="growl";impact="void";}
        else if(skill.Operator=="horizon"){cast="charge";impact="beam";}
        else if(skill.Ice>0){cast="ice";impact="ice-hit";}
        soundEvents.Add((clock+.14,cast,profile.Pitch,actor.Id=="akuroth"?-11:-15));
        if(actor.Id=="akuroth"){
            var hits=Battle.LastImpacts.Where(h=>h.Attacker==actor).ToArray();
            if(hits.Length>0){
                if(skill.Id=="akuroth_annihilation"){
                    soundEvents.Add((clock+1.10,"akuroth-collapse-impact",1,-20));
                    soundEvents.Add((clock+3.0,impact,1,-10));
                }
                else if(skill.Operator=="aoe")soundEvents.Add((clock+ImpactDelay(hits[0]),impact,1,-12));
                else foreach(var h in hits.Take(8))soundEvents.Add((clock+ImpactDelay(h),impact,1,-15));
            }
            return;
        }
        if(Battle.LastImpacts.Count==0)soundEvents.Add((clock+HitMoment,impact,profile.Pitch,-13));
        else foreach(var hit in Battle.LastImpacts.Select((h,i)=>(h,i)).GroupBy(x=>skill.Operator=="horizon"?x.i:x.h.HitIndex).Select(g=>g.First()).Take(10))soundEvents.Add((clock+ImpactDelay(hit.h),impact,profile.Pitch+(hit.i%3)*.025f,-20));
    }
    private void UpdateActionSound(){
        if(audioPaused)return;
        foreach(var voice in voiceEnds.ToArray()){
            double left=voice.Value.End-clock;
            if(left<=0||!voice.Key.Playing){voice.Key.Stop();voiceEnds.Remove(voice.Key);}
            else if(left<.12)voice.Key.VolumeDb=voice.Value.Volume+(float)(20*Math.Log10(Math.Max(.001,left/.12)));
        }
        foreach(var e in soundEvents.Where(e=>clock>=e.At).ToArray()){PlayCue(e.Cue,e.Pitch,e.Gain);soundEvents.Remove(e);AttackSoundsPlayed++;}
        if(Battle==null||clock<nextStepAt)return;
        var movers=Battle.Allies.Concat(Battle.Enemies).Where(u=>u.Alive&&ShownFreeze(u)==0&&ShownStun(u)==0&&(ExitActive&&Battle.Allies.Contains(u)||EntranceActive&&clock-entranceAt<1.08||!EntranceActive&&IsRunning(u,(float)(clock-actionAt)))).Take(3).ToArray();
        if(movers.Length==0)return;
        nextStepAt=clock+.20;
        foreach(var u in movers){var p=SoundFor(u);PlayCue(p.Step,p.Pitch, -24);FootstepsPlayed++;}
    }
    private void PlayCue(string cue,float pitch,float volume){
        AudioStream stream;bool recorded=sampleStreams.TryGetValue(cue,out var sample);
        if(recorded)stream=sample!;
        else {if(!soundCache.TryGetValue(cue,out var synthesized)){synthesized=MakeCue(cue);soundCache[cue]=synthesized;}stream=synthesized;}
        var player=soundVoices.FirstOrDefault(p=>!p.Playing);
        if(player==null){if(soundVoices.Count>=12)return;player=new AudioStreamPlayer();AddChild(player);soundVoices.Add(player);}
        voiceEnds.Remove(player);
        player.Stream=stream;player.PitchScale=recorded?1:pitch;player.VolumeDb=recorded?volume+8+samples[cue].Gain:volume;player.Play();
        if(recorded){RecordedSoundsPlayed++;voiceEnds[player]=(clock+Math.Min(samples[cue].Duration,stream.GetLength()),player.VolumeDb);}
    }
    // Original PCM synthesis: layered material transients, pitched resonances and envelopes.
    public static AudioStreamWav MakeCue(string cue){
        if(cue.StartsWith("akuroth-"))return MakeAkurothCue(cue);
        const int rate=44100;double duration=cue.StartsWith("akuroth-")?cue is "akuroth-transform" or "akuroth-annihilation"?1.4:.7:cue is "heal" or "chime"?.65:cue is "leather" or "soft" or "metal" or "clawstep"?.15:.40;
        var bytes=new byte[(int)(rate*duration)*2];var random=new Random(626);double low=0,soft=0;
        for(int n=0;n<bytes.Length/2;n++){
            double t=n/(double)rate,noise=random.NextDouble()*2-1;low=.86*low+.14*noise;
            double tone(double hz)=>Math.Sin(Math.Tau*hz*t);
            double v=cue switch{
                "steel"=>(tone(1567)*.24+tone(2639)*.16+tone(4213)*.08)*Math.Exp(-t*13)+noise*.25*Math.Exp(-t*75),
                "kick"=>tone(92-50*t)*Math.Exp(-t*28)*.65+low*Math.Exp(-t*40)*1.2,
                "heavy"=>tone(57)*Math.Exp(-t*13)*.65+tone(279)*Math.Exp(-t*22)*.22+noise*Math.Exp(-t*45)*.35,
                "ice"=>(tone(3201)+tone(4637)*.6+tone(5723)*.3)*Math.Exp(-t*14)*.20+noise*Math.Exp(-t*33)*.2,
                "heal" or "chime"=>(tone(659)+tone(988)*.6+tone(1318)*.3)*Math.Exp(-t*5)*.24,
                "void"=>Math.Sin(Math.Tau*(110*t-70*t*t))*Math.Exp(-t*7)*.45+low*Math.Exp(-t*8)*.5,
                "charge" or "beam"=>Math.Sin(Math.Tau*(240*t+900*t*t))*Math.Exp(-t*6)*.36+noise*Math.Exp(-t*14)*.18,
                "fire" or "sizzle"=>low*Math.Exp(-t*9)*2+noise*Math.Exp(-t*18)*.18,
                "servo"=>tone(189)*(0.5+0.5*tone(32))*Math.Exp(-t*10)*.4+noise*Math.Exp(-t*25)*.15,
                "growl"=>tone(78)*(0.6+0.4*tone(29))*Math.Exp(-t*8)*.5+low*Math.Exp(-t*8)*1.1,
                "claw"=>noise*Math.Exp(-t*26)*.35+tone(168)*Math.Exp(-t*24)*.32,
                "metal"=>tone(180)*Math.Exp(-t*45)*.4+tone(1271)*Math.Exp(-t*60)*.2+noise*Math.Exp(-t*80)*.2,
                "leather" or "clawstep"=>tone(105)*Math.Exp(-t*48)*.45+low*Math.Exp(-t*40)*1.4,
                "soft"=>low*Math.Exp(-t*55)*1.2,
                _=>noise*Math.Sin(Math.PI*Math.Clamp(t/.3,0,1))*Math.Exp(-t*9)*.5
            };
            // Broad, smooth material sounds replace the sharp broadband transients and laser chirps.
            double air=low-soft;soft=.97*soft+.03*low;
            double swell=Math.Sin(Math.PI*Math.Clamp(t/duration,0,1));
            if(cue is "charge" or "beam" or "servo")v=(air*2.4+tone(155)*.07)*swell*Math.Exp(-t*3);
            else if(cue=="growl")v=(soft*4+tone(83)*.055+tone(121)*.025)*swell;
            else if(cue=="claw")v=(air*2.5+soft*2)*swell*Math.Exp(-t*4);
            else if(cue is "heavy" or "kick")v=(soft*2.8+tone(cue=="heavy"?68:105)*.14)*Math.Exp(-t*10);
            else if(cue=="steel")v=(air*1.8+tone(1261)*.035+tone(1937)*.025)*Math.Exp(-t*9);
            else if(cue=="ice")v=(air*.8+tone(2201)*.025+tone(2893)*.018)*swell*Math.Exp(-t*5);
            else if(cue is "swing" or "fire" or "sizzle")v=air*2.1*swell*Math.Exp(-t*3);
            if(cue.StartsWith("akuroth-")){
                double baseHz=cue switch{"akuroth-cut"=>185,"akuroth-waves"=>75,"akuroth-prison"=>330,"akuroth-meteor"=>54,"akuroth-rift"=>121,"akuroth-chaos"=>220,"akuroth-collapse"=>63,"akuroth-rain"=>147,"akuroth-rewrite"=>440,"akuroth-transform"=>98,"akuroth-annihilation"=>41,_=>85};
                double pulse=cue=="akuroth-chaos"?.6+.4*tone(13):1;
                v=(tone(baseHz)*.22+tone(baseHz*1.498)*.12+soft*3+air*.7)*swell*pulse;
                if(cue is "akuroth-prison" or "akuroth-rewrite")v+=tone(baseHz*4)*.07*Math.Exp(-t*7);
                if(cue=="akuroth-impact")v=(tone(57)*.28+soft*3)*Math.Exp(-t*8);
            }
            v*=Math.Min(1,t/.018)*Math.Min(1,(duration-t)/.05);short sample=(short)(Math.Tanh(v)*22000);bytes[n*2]=(byte)sample;bytes[n*2+1]=(byte)(sample>>8);
        }
        return new AudioStreamWav{Format=AudioStreamWav.FormatEnum.Format16Bits,MixRate=rate,Data=bytes};
    }
}
