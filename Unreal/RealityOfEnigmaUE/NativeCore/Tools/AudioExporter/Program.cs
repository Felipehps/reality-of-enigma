using EnigmaAudio;
using System.Text.Json.Nodes;
var data=JsonNode.Parse(File.ReadAllText(args[0]))!.AsObject();var samples=JsonNode.Parse(File.ReadAllText(args[1]))!.AsObject();Directory.CreateDirectory(args[2]);
var cues=data.SelectMany(p=>new[]{p.Value!["Cast"]!.GetValue<string>(),p.Value!["Impact"]!.GetValue<string>(),p.Value!["Step"]!.GetValue<string>()}).Append("akuroth-transform").Distinct();
foreach(var cue in cues){if(samples.ContainsKey(cue))continue;var wave=Generator.MakeCue(cue);using var writer=new BinaryWriter(File.Create(Path.Combine(args[2],"Cue_"+cue.Replace('-','_')+".wav")));int channels=wave.Stereo?2:1;writer.Write("RIFF"u8);writer.Write(36+wave.Data.Length);writer.Write("WAVEfmt "u8);writer.Write(16);writer.Write((short)1);writer.Write((short)channels);writer.Write(wave.MixRate);writer.Write(wave.MixRate*channels*2);writer.Write((short)(channels*2));writer.Write((short)16);writer.Write("data"u8);writer.Write(wave.Data.Length);writer.Write(wave.Data);}
Console.WriteLine("AUDIO_SYNTHESIS_PORT_OK");
