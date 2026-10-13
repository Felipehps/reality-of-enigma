using Godot;
using Vaelorn;
using System;
using System.Collections.Generic;
using System.Text.Json;

public partial class Battlefield
{
    private sealed record FrameAtlas(string Texture,int[][] Frames);
    public static int RetreatFrame(float progress)=>progress<.10f?0:progress<.23f?1:progress<.36f?2:progress<.49f?3:progress<.64f?4:progress<.79f?5:progress<.93f?6:7;
    private readonly Dictionary<string,(Texture2D Texture,FrameAtlas Data)> frameAtlases=new();
    private void LoadFrames()
    {
        var definitions=JsonSerializer.Deserialize<Dictionary<string,FrameAtlas>>(Godot.FileAccess.GetFileAsString("res://Data/animations.json"))!;
        foreach(var pair in definitions){
            if(pair.Value.Frames.Length!=(pair.Key.EndsWith("_spiral")?16:pair.Key.EndsWith("_run")?4:pair.Key.EndsWith("_action")||pair.Key.EndsWith("_retreat")?8:12))throw new InvalidOperationException("Atlas incompleto: "+pair.Key);
            frameAtlases[pair.Key]=(GD.Load<Texture2D>(pair.Value.Texture),pair.Value);
        }
    }
    public static int ActionFrame(float age,bool melee)=>melee
        ?age<.06f?0:age<.48f?1:age<.56f?2:age<HitMoment?3:age<.74f?4:age<.84f?5:age<.97f?6:7
        :age<.15f?0:age<.30f?1:age<.48f?2:age<HitMoment?3:age<.82f?4:age<1.05f?5:age<1.3f?6:7;
    public static int PoseFrame(bool alive,bool controlled,bool active,double age,double hitAge,double fallAge,double idleTime)
    {
        if(!alive)return fallAge<.12?8:fallAge<.28?9:fallAge<.48?10:11;
        if(controlled)return 0;
        if(hitAge<.5)return hitAge<.16?8:hitAge<.34?9:7;
        if(active)return age<.22?2:age<HitMoment?3:age<.79?4:age<1.02?5:age<1.28?6:7;
        return 0; // Continuous articulated breathing replaces the two-pose toggle.
    }
    public static bool UsesCookingAtlas(Unit unit,bool active,bool alive,float hitAge,string? op)=>unit.Id=="savor"&&active&&alive&&hitAge>=.5f&&unit.Freeze==0&&unit.Stun==0&&op=="buff";
    public bool Portrait(Unit unit,out Texture2D texture,out Rect2 region)
    {
        if(unit.Id=="akuroth"){texture=akurothArt;region=new Rect2(unit.Ascended?new Vector2(1140,240):new Vector2(320,205),new Vector2(220,230));return true;}
        texture=null!;region=default;
        if(!frameAtlases.TryGetValue(unit.Id,out var atlas))return false;
        var b=atlas.Data.Frames[0];float h=b[3]*.48f,w=Math.Min(b[2],h*.86f);
        texture=atlas.Texture;region=new Rect2(b[0]+b[2]/2f-w/2,b[1],w,h);return true;
    }
    public static float UpperBodyWeight(float fraction)=>Mathf.Pow(Mathf.Clamp((.72f-fraction)/.72f,0,1),2);
    public bool FullPortrait(Unit unit,out Texture2D texture,out Rect2 region)
    {
        if(unit.Id=="akuroth"){texture=akurothArt;region=AkurothRegion(unit);return true;}
        texture=null!;region=default;
        if(!frameAtlases.TryGetValue(unit.Id,out var atlas))return false;
        var b=atlas.Data.Frames[0];texture=atlas.Texture;region=new Rect2(b[0],b[1],b[2],b[3]);return true;
    }
    private readonly Dictionary<Unit,(string Atlas,int Frame,int Previous,double Changed)> poseTransitions=new();
    private bool DrawFrame(Unit unit,Vector2 at,float height,bool friendly,bool alive,bool active,float age,float hitAge,Color tint,float lean,Vector2 stretch)
    {
        string id=unit.Id.StartsWith("enemy")?"enemy_fenda":unit.Id;
        if(id=="nerathis")friendly=!friendly;
        if(id=="maltherion"&&unit.Ascended)id="maltherion_ascended";
        if(!(unit.Id=="kael"&&unit.Path=="martial")&&EntranceActive&&clock-entranceAt>=1.5&&frameAtlases.TryGetValue(id+"_retreat",out var retreat)){
            float progress=Mathf.Clamp((float)(clock-entranceAt-1.5)/1.3f,0,1);int pose=RetreatFrame(progress);
            var b=retreat.Data.Frames[pose];float k=height/retreat.Data.Frames[7][3];
            var dimensions=new Vector2(b[2],b[3])*k;
            float bottom=pose is 2 or 3?(-height+dimensions.Y)*.5f:0;
            DrawSetTransform(at,0,new Vector2(friendly?1:-1,1));
            DrawTextureRectRegion(retreat.Texture,new Rect2(-dimensions.X/2,-dimensions.Y+bottom,dimensions.X,dimensions.Y),new Rect2(b[0],b[1],b[2],b[3]),tint);DrawSetTransform(Vector2.Zero);return true;
        }
        if(UsesCookingAtlas(unit,active,alive,hitAge,Battle?.LastSkill?.Operator))id="savor_buff";
        if(unit.Id=="kael"&&unit.Path=="martial"||!frameAtlases.TryGetValue(id,out var atlas))return false;
        int frame=PoseFrame(alive,ShownFreeze(unit)>0||ShownStun(unit)>0,active,age,hitAge,clock-defeatedAt.GetValueOrDefault(unit,clock-1),clock+unit.Layer*.17);
        if(unit.Id is "sevrin" or "maltherion" && alive && active)frame=age<.4f?4:age<HitMoment?5:age<.9f?6:7;
        bool introRun=ExitActive&&friendly||EntranceActive&&(clock-entranceAt)<1.1;
        bool running=alive&&(introRun||active)&&hitAge>=.5f&&ShownStun(unit)==0&&ShownFreeze(unit)==0&&(introRun||IsRunning(unit,age))&&frameAtlases.ContainsKey(id+"_run");
        if(running){atlas=frameAtlases[id+"_run"];frame=(int)((introRun?clock:age)*18)%4;if(!introRun&&age>1.08f)friendly=!friendly;}
        else if(active&&alive&&hitAge>=.5f&&Battle?.LastSkill?.Hits>1&&age>=HitMoment&&age<HitMoment+Battle.LastSkill.Hits*.095f)frame=4+(int)((age-HitMoment)/.095f)%2;
        bool action=alive&&active&&!running&&hitAge>=.5f&&ShownFreeze(unit)==0&&ShownStun(unit)==0&&Battle?.LastSkill?.Operator!="split"&&frameAtlases.ContainsKey(id+"_action");
        if(action){atlas=frameAtlases[id+"_action"];frame=ActionFrame(age,IsMelee(unit));}
        if(alive&&active&&unit.Id=="varkas"&&Battle?.LastSkill?.Operator=="split")frame=2;
        if(alive&&ShownFreeze(unit)>0)frame=0;
        else if(alive&&ShownStun(unit)>0)frame=9;
        string atlasKey=running?id+"_run":action?id+"_action":id;
        if(!poseTransitions.TryGetValue(unit,out var transition)||transition.Atlas!=atlasKey)transition=(atlasKey,frame,frame,clock);
        else if(transition.Frame!=frame)transition=(atlasKey,frame,transition.Frame,clock);
        poseTransitions[unit]=transition;
        var box=atlas.Data.Frames[frame];
        float scale=height*(running?.93f:1)/atlas.Data.Frames[0][3];
        var size=new Vector2(box[2],box[3])*scale;
        float anchorX=box.Length>4?box[4]*scale:size.X/2;
        // Constant scale per character, foot-anchored frames: falling drawings retain their natural height.
        DrawSetTransform(at,lean,new Vector2((friendly?1:-1)*stretch.X,stretch.Y));
        bool idle=alive&&!EntranceActive&&!ExitActive&&!active&&hitAge>=.5f&&ShownFreeze(unit)==0&&ShownStun(unit)==0;
        if(idle||(alive&&ShownStun(unit)>0&&ShownFreeze(unit)==0)){
            // Continuous strip articulation keeps the feet planted while shoulders, head and cloth move.
            const int slices=24;
            float phase=(float)clock*(ShownStun(unit)>0?3.4f:2.1f)+unit.Column*.8f+unit.Layer*1.3f;
            for(int n=0;n<slices;n++){
                float a=n/(float)slices,b=(n+1)/(float)slices;
                float ya=-size.Y+size.Y*a+Mathf.Sin(phase)*UpperBodyWeight(a)*1.2f;
                float yb=-size.Y+size.Y*b+Mathf.Sin(phase)*UpperBodyWeight(b)*1.2f;
                float sway=Mathf.Sin(phase*.5f)*UpperBodyWeight(a)*(ShownStun(unit)>0?7:1.7f);
                float widen=1+Mathf.Sin(phase)*UpperBodyWeight(a)*.008f;
                DrawTextureRectRegion(atlas.Texture,new Rect2(-size.X*widen/2+sway,ya,size.X*widen,yb-ya+.15f),new Rect2(box[0],box[1]+box[3]*a,box[2],box[3]/(float)slices),tint);
            }
        }else{
            float blend=alive&&ShownFreeze(unit)==0&&ShownStun(unit)==0?Mathf.Clamp((float)(clock-transition.Changed)/.055f,0,1):1;
            if(blend<1&&transition.Previous!=frame){
                var previous=atlas.Data.Frames[transition.Previous];var oldSize=new Vector2(previous[2],previous[3])*scale;
                DrawTextureRectRegion(atlas.Texture,new Rect2(-(previous.Length>4?previous[4]*scale:oldSize.X/2),-oldSize.Y,oldSize.X,oldSize.Y),new Rect2(previous[0],previous[1],previous[2],previous[3]),new Color(tint,1-blend));
                DrawTextureRectRegion(atlas.Texture,new Rect2(-anchorX,-size.Y,size.X,size.Y),new Rect2(box[0],box[1],box[2],box[3]),new Color(tint,blend));
            }else DrawTextureRectRegion(atlas.Texture,new Rect2(-anchorX,-size.Y,size.X,size.Y),new Rect2(box[0],box[1],box[2],box[3]),tint);
        }
        DrawSetTransform(Vector2.Zero);
        return true;
    }
}
