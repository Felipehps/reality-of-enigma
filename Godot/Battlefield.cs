using Godot;
using Vaelorn;
using System;
using System.Linq;
using System.Collections.Generic;

public partial class Battlefield:Control
{
    public Battle? Battle{get;private set;}
    public Unit? Selected{get;private set;}
    public event Action<Unit>? UnitSelected;
    private Texture2D arena=null!,poses=null!,combatants=null!,effectAtlas=null!;
    private readonly Dictionary<Unit,Rect2> cards=new();
    private readonly List<(Unit unit,double delta,double at)> floats=new();
    private readonly List<DamageNumber> damageNumbers=new();
    private readonly Dictionary<Unit,double> defeatedAt=new();
    private readonly HashSet<Unit> frozenDefeats=new();
    private double clock,actionAt=-10;
    private Unit? actor;
    public const float HitMoment=.65f;
    public float EffectDuration=>Battle?.LastSkill?.Id is {} id&&id.StartsWith("akuroth_")?AkurothDuration(id):Battle?.LastSkill?.Id=="gale_fangs"?3.5f:1.85f;
    public bool IsAnimating=>clock-actionAt<EffectDuration||EntranceActive||AkurothTransforming;
    private readonly Color gold=new("d8b474"),blue=new("72dcff");
    private Font Font=>EnigmaStyle.Font;
    public override void _Ready()
    {
        arena=GD.Load<Texture2D>("res://Art/arena-v17.png");poses=GD.Load<Texture2D>("res://Art/kael-poses-v5.png");combatants=GD.Load<Texture2D>("res://Art/combatants-v5.png");
        effectAtlas=GD.Load<Texture2D>("res://Art/skill-effects-v6.png");
        LoadFrames();LoadSoundProfiles();LoadAkurothArt();LoadAkurothCinema();
        TextureFilter=TextureFilterEnum.Linear;
        CustomMinimumSize=new Vector2(0,440);MouseFilter=MouseFilterEnum.Stop;
    }
    public void Bind(Battle battle){SilenceActionSounds();akurothTransformations.Clear();ResetArena();audioPaused=false;TargetIntent="attack";FormationEditing=false;exitAt=-100;evolutions.Clear();evolutionPlayed.Clear();entranceAt=-10;endingAt=-10;Battle=battle;foreach(var u in battle.Allies.Concat(battle.Enemies).Where(u=>u.Id=="akuroth"&&u.Ascended&&u.Alive))akurothTransformations[u]=-100;Selected=battle.Enemies.FirstOrDefault();floats.Clear();damageNumbers.Clear();defeatedAt.Clear();frozenDefeats.Clear();poseTransitions.Clear();actor=null;actionAt=-10;ResetVisualStates();QueueRedraw();}
    public double DisplayHp(Unit unit)=>VisualHp(unit);
    public void SelectUnit(Unit unit){Selected=unit;TooltipText="";UnitSelected?.Invoke(unit);QueueRedraw();}
    public void Animate(Dictionary<Unit,double> before)
    {
        if(Battle==null)return;actor=Battle.LastActor;actionAt=clock;DelayVisualStates();ScheduleActionSound();ScheduleAkurothTransformation();ScheduleArenaImpacts();
        foreach(var pair in before){var delta=pair.Key.Hp-pair.Value;if(Math.Abs(delta)>.01)floats.Add((pair.Key,delta,clock));}
        foreach(var hit in Battle.LastImpacts)if(hit.Amount>.01)
            damageNumbers.Add(new(hit.Target,hit.Amount,hit.Kind,hit.Critical,hit.HitIndex,clock+ImpactDelay(hit)));
        foreach(var unit in before.Keys.Where(u=>!u.Alive&&!defeatedAt.ContainsKey(u))){
            var last=damageNumbers.Where(n=>n.Unit==unit&&n.Kind!="heal").Select(n=>n.At).DefaultIfEmpty(clock+HitMoment).Max();
            defeatedAt[unit]=last;
            soundEvents.Add((last+.18,unit.Freeze>0?"ice-shatter":unit.Id.StartsWith("enemy")||unit.Id=="varkas"?"beast-fall":"body-fall",1,-19));
            if(unit.Freeze>0)frozenDefeats.Add(unit);
        }
        QueueRedraw();
    }
    public override void _Process(double delta){if(!previewTimeline){double step=delta;if(hitstopRemaining>0){hitstopRemaining-=delta;step*=.08;}clock+=step*1.16;}UpdateArenaImpacts();UpdateClashAudio();UpdateActionSound();floats.RemoveAll(f=>clock-f.at>EffectDuration);damageNumbers.RemoveAll(f=>clock-f.At>1.3);QueueRedraw();}
    public override void _GuiInput(InputEvent e)
    {
        if(e is InputEventMouseMotion motion){TooltipText=BuffHoverText(motion.Position);}
        if(FormationInput(e))return;
        if(e is InputEventMouseButton m&&m.Pressed&&m.ButtonIndex==MouseButton.Left)
            foreach(var pair in cards.OrderByDescending(p=>p.Value.End.Y))if(pair.Value.HasPoint(m.Position)){Selected=pair.Key;TooltipText="";UnitSelected?.Invoke(pair.Key);QueueRedraw();break;}
    }
    private void Text(string text,Vector2 pos,Color color,int size=14)=>DrawString(Font,pos,text,HorizontalAlignment.Left,-1,size,color);
    public override void _Draw()
    {
        DrawAkurothArena();
        DrawAkurothWeather();
        DrawRect(new Rect2(Vector2.Zero,new Vector2(Size.X,45)),new Color(.02f,.04f,.08f,.72f));
        Text(AkurothInfernoMix()>.5f?"DOMÍNIO INFERNAL • AKUROTH":forestEnvironment?"FLORESTA DE VAELORN":"SANTUÁRIO PARTIDO",new Vector2(24,29),gold,17);
        Text("ATO I   /   ECOS DA FENDA",new Vector2(Size.X-260,29),new Color("c8dbe6"),13);
        for(int n=0;n<18;n++){
            float x=(n*179+(float)clock*7)%Size.X,y=60+(n*93+(float)clock*11)%(Size.Y-100);
            DrawCircle(new Vector2(x,y),1.3f,new Color(.5f,.9f,1,.2f));
        }
        cards.Clear();buffHover.Clear();
        if(Battle==null)return;
        DrawFormationGrid();
        DrawArenaDamage();
        float height=Mathf.Min(136,Size.Y*.205f),width=160;
        foreach(var pair in new[]{(Battle.AlliedTargets,true),(Battle.EnemyTargets,false)})foreach(var unit in pair.Item1){
            var pos=CellFoot(pair.Item2,unit.Column,unit.Layer)+(unit.Origin!=null?new Vector2(pair.Item2?-70:70,62):Vector2.Zero);
            float x=pos.X,foot=pos.Y+20;
            cards[unit]=new Rect2(x-width/2,foot-height,width,height);
        }
        foreach(var pair in cards.OrderBy(p=>GroundPosition(p.Key).Y).ThenBy(p=>p.Key.Layer))DrawCombatant(pair.Key,pair.Value,Battle.Allies.Contains(pair.Key.Origin??pair.Key));
        DrawIdentityMarkers();
        DrawAkurothSoulChains();
        DrawSkillIntroduction();
        DrawPowers();
        DrawAkurothHud();
        DrawAkurothCinematicOverlay();
        DrawArenaFragments();
        DrawDamageNumbers();
        DrawTurnRail();
        DrawEncounterTransition();
    }
    private void Snow(Vector2 center,float radius)
    {for(int n=0;n<3;n++){float a=n*Mathf.Pi/3;var v=new Vector2(Mathf.Cos(a),Mathf.Sin(a))*radius;DrawLine(center-v,center+v,blue,2);}}
    private void Star(Vector2 center,float radius,Color color)
    {var points=Enumerable.Range(0,10).Select(n=>{float a=n*Mathf.Pi/5-Mathf.Pi/2;return center+new Vector2(Mathf.Cos(a),Mathf.Sin(a))*(n%2==0?radius:radius*.4f);}).ToArray();DrawColoredPolygon(points,color);}
}
