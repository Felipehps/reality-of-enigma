using Godot;
using Vaelorn;
using System;
using System.Linq;

public partial class Battlefield
{
    public const float AkurothTransformDuration=4.6f;
    public bool AkurothTransforming=>akurothTransformations.Any(p=>p.Key.Alive&&clock>=p.Value&&clock-p.Value<AkurothTransformDuration);
    private Vector2 AkurothCameraFocus{
        get{
            var home=Size*new Vector2(.52f,.48f);
            Vector2 ClampFocus(Vector2 point){float z=AkurothCameraZoom;if(z<=1.00001f)return home;return new Vector2(Mathf.Clamp(point.X,home.X/z,Size.X-(Size.X-home.X)/z),Mathf.Clamp(point.Y,home.Y/z,Size.Y-(Size.Y-home.Y)/z));}
            foreach(var p in akurothTransformations){float t=(float)(clock-p.Value);if(p.Key.Alive&&t>=0&&t<AkurothTransformDuration){var boss=CellFoot(Battle!.Allies.Contains(p.Key),p.Key.Column,p.Key.Layer);return ClampFocus(home.Lerp(boss,Mathf.Sin(t/AkurothTransformDuration*Mathf.Pi)*.9f));}}
            return ClampFocus(home);
        }
    }
    private void DrawTransformationPose(int phase,int frame,Vector2 at,float h,bool friendly,float alpha){
        if(alpha<=0)return;var atlas=akurothActionAtlases[phase];var b=atlas.Data.Frames[frame];float scale=h/atlas.Data.ReferenceHeight;
        DrawSetTransform(at,0,new Vector2(friendly?-1:1,1));
        DrawTextureRectRegion(atlas.Texture,new Rect2(-b[4]*scale,-b[5]*scale,b[2]*scale,b[3]*scale),new Rect2(b[0],b[1],b[2],b[3]),new Color(1,1,1,alpha));DrawSetTransform(Vector2.Zero);
    }
    private void DrawAkurothTransformation(Unit u,Vector2 at,float h,bool friendly,float t){
        float lift=Mathf.Sin(Mathf.Clamp(t/3.5f,0,1)*Mathf.Pi)*22;
        var elevated=at-new Vector2(0,lift);
        float oldAlpha=1-Mathf.SmoothStep(0,1,Mathf.Clamp((t-1.5f)/.65f,0,1));
        float newAlpha=Mathf.SmoothStep(0,1,Mathf.Clamp((t-1.65f)/.7f,0,1));
        DrawTransformationPose(1,t<.6f?1:3,elevated,h,friendly,oldAlpha);
        DrawTransformationPose(2,t<2.6f?3:t<3.5f?2:7,elevated,h,friendly,newAlpha);
        var chest=elevated-new Vector2(0,h*.48f);
        float cloak=Mathf.Sin(Mathf.Clamp((t-.2f)/3.7f,0,1)*Mathf.Pi);
        DrawAkurothFx("collapse",chest,Vector2.One*(h*.65f+cloak*h*.9f),cloak*.3f,-t*.22f);
        if(t>.5f&&t<3.25f)DrawAkurothSingularity(chest,18+Mathf.Sin((t-.5f)/2.75f*Mathf.Pi)*h*.35f,Mathf.Sin((t-.5f)/2.75f*Mathf.Pi)*.85f);
        if(t>1.8f&&t<3.6f){float reveal=Mathf.Sin((t-1.8f)/1.8f*Mathf.Pi);DrawAkurothFx("chaos",chest,new Vector2(h*1.4f,h*.95f),reveal*.35f,t*.12f);}
        string label=t<1.5f?"A REALIDADE SE ROMPE":t<3.4f?"SOBERANO DO ABSOLUTO":"O FIM DAS REALIDADES";
        float w=Font.GetStringSize(label,fontSize:20).X;
        Text(label,new Vector2(Mathf.Clamp(at.X-w/2,85,Size.X-w-12),Mathf.Max(170,at.Y-h-35)),new Color(1,.83f,.89f),20);
    }
}
