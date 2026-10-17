"""Procedural material library. No paid service or downloaded executable is required."""
import unreal
from pathlib import Path
lib=unreal.EditorAssetLibrary
mel=unreal.MaterialEditingLibrary
folder='/Game/StoryEnvironments'
lib.make_directory(folder)
noise='''
struct Field {
 float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
 float noise(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);}
 float fbm(float2 p){return .57*noise(p)+.28*noise(p*2.13)+.15*noise(p*4.27);}
 float edge(float2 p){float2 g=floor(p),f=frac(p);float first=9,second=9;[unroll]for(int y=-1;y<=1;y++){[unroll]for(int x=-1;x<=1;x++){float2 o=float2(x,y),q=o+float2(hash(g+o),hash(g+o+17.1))-f;float d=dot(q,q);if(d<first){second=first;first=d;}else second=min(second,d);}}return sqrt(second)-sqrt(first);}
}; Field n;
'''
def const(mat,value):
 node=mel.create_material_expression(mat,unreal.MaterialExpressionConstant,-300,700)
 node.set_editor_property('r',value)
 return node
def custom(mat,code,output=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
 node=mel.create_material_expression(mat,unreal.MaterialExpressionCustom,0,0)
 node.set_editor_property('code',noise+code)
 node.set_editor_property('output_type',output)
 inputs=[]
 for name in ['P','UV','Tint','Clock']:
  entry=unreal.CustomInput();entry.set_editor_property('input_name',name);inputs.append(entry)
 node.set_editor_property('inputs',inputs)
 for name,source in nodes.items():mel.connect_material_expressions(source,'',node,name)
 return node
def hook(node,prop,pin=''):assert mel.connect_material_property(node,pin,prop)
recipes={
 'Wood':('''float grain=n.fbm(float2(P.z*.002,P.x*.025+P.y*.024));float ridge=pow(abs(sin(P.z*.003+(P.x+P.y)*.08+grain*5)),12);return Tint*(.42+grain*.8)*(1-ridge*.4);''',.86),
 'Ground':('''float broad=n.fbm(P.xy*.0012),grain=n.noise(P.xy*.12);float path=1-smoothstep(70,240,abs(P.y+80*sin(P.x*.0014)));float3 earth=float3(.095,.067,.035)*(0.65+grain*.5);float3 moss=Tint*(.55+broad*.95);return lerp(moss,earth,path*.75)*(1-.16*n.noise(P.xy*.035));''',.95),
 'Foliage':('''float edge=pow(abs(UV.x*2-1),2);float vein=exp(-abs(UV.x-.5)*90);float veins=pow(saturate(cos((UV.y+abs(UV.x-.5)*.6)*135)),14)*.15;float mottling=n.noise(UV*19+P.xy*.002);return Tint*(1.25+UV.y*.5+veins+vein*.45+mottling*.35)*(1-edge*.24);''',.63),
 'Petal':('''float vein=pow(saturate(cos((UV.x-.5)*70+UV.y*14)),20);float rim=smoothstep(.45,.95,abs(UV.x*2-1));float3 petal=lerp(Tint,float3(.62,.38,.12),smoothstep(.35,1,UV.y));return petal*(.7+UV.y*.25+vein*.12)*(1-rim*.12);''',.4),
 'Metal':('''float wear=n.fbm(P.xy*.025+P.z*.001);return Tint*(.65+wear*.45);''',.32),
 'Sky':('''float height=saturate((P.z+500)/3500);float cloud=n.fbm(P.xy*.00045+Clock*.0015);float wisps=pow(saturate(cloud*1.7-.55),2);float3 horizon=Tint*1.5+float3(.008,.012,.02);float3 zenith=Tint*.28;return lerp(horizon,zenith,smoothstep(0,1,height))+wisps*Tint*.26;''',1),
 'Water':('''float wave=sin(P.x*.009+Clock*.7)+sin(P.y*.013-Clock*.9);return Tint*(.75+wave*.04);''',.12),
 'Lava':('''float heat=n.fbm(P.xy*.004+Clock*.015);float fissure=1-smoothstep(.018,.09,n.edge(P.xy*.004));return float3(.008,.004,.003)+fissure*(float3(1,.12,.007)*1.7+Tint*.35)*(0.7+heat*.5);''',.62),
 'Paper':('return Tint*(.78+.22*n.noise(P.xy*.08+P.z*.01));',.9),
 'Mist':('return Tint;',1),
 'Fire':('''float f=n.fbm(float2(UV.x*4,UV.y*5-Clock*1.7));return lerp(Tint*3,float3(1,.68,.16)*4,saturate((1-UV.y)*f));''',1)
}
for name,(code,roughness) in recipes.items():
 path=folder+'/M_Story'+name
 mat=lib.load_asset(path) if lib.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_Story'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
 mel.delete_all_material_expressions(mat)
 mat.set_editor_property('two_sided',True)
 pos=mel.create_material_expression(mat,unreal.MaterialExpressionWorldPosition,-600,0)
 uv=mel.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate,-600,200)
 tint=mel.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-600,400);tint.set_editor_property('parameter_name','Color');tint.set_editor_property('default_value',unreal.LinearColor(.08,.15,.06,1))
 clock=mel.create_material_expression(mat,unreal.MaterialExpressionScalarParameter,-600,600);clock.set_editor_property('parameter_name','SceneTime');clock.set_editor_property('default_value',0)
 nodes={'P':pos,'UV':uv,'Tint':tint,'Clock':clock}
 color=custom(mat,code)
 hook(color,unreal.MaterialProperty.MP_BASE_COLOR)
 hook(const(mat,roughness),unreal.MaterialProperty.MP_ROUGHNESS)
 if name in ['Wood','Ground']:
  slug='bark_brown_01' if name=='Wood' else 'forrest_ground_01'
  source=Path(__file__).resolve().parent/'TextureSources'
  texture_uv=uv if name=='Wood' else custom(mat,'return P.xy*.004;',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
  for kind,ext,prop in [('diff','jpg',unreal.MaterialProperty.MP_BASE_COLOR),('nor_dx','png',unreal.MaterialProperty.MP_NORMAL),('rough','jpg',unreal.MaterialProperty.MP_ROUGHNESS),('ao','jpg',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)]:
   asset_name='T_Story'+name+'_'+kind
   texture_path=folder+'/'+asset_name
   tex=lib.load_asset(texture_path)
   if not tex:
    task=unreal.AssetImportTask();task.filename=str(source/(slug+'_'+kind+'_2k.'+ext));task.destination_path=folder;task.destination_name=asset_name;task.automated=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);tex=lib.load_asset(texture_path)
   assert tex,texture_path
   tex.set_editor_property('srgb',kind=='diff');tex.set_editor_property('max_texture_size',2048)
   tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP if kind=='nor_dx' else unreal.TextureCompressionSettings.TC_DEFAULT if kind=='diff' else unreal.TextureCompressionSettings.TC_GRAYSCALE)
   assert lib.save_loaded_asset(tex,False)
   sample=mel.create_material_expression(mat,unreal.MaterialExpressionTextureSample,400,500)
   sample.texture=tex;sample.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='nor_dx' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='diff' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
   mel.connect_material_expressions(texture_uv,'',sample,'Coordinates')
   if kind=='diff':
    # Snow shares the terrain geometry but keeps its pale palette over fine PBR detail.
    factor=custom(mat,'return float3(.85,.8,.72);' if name=='Wood' else 'return lerp(float3(.72,.82,.65),float3(2.7,2.9,3.1),step(.5,Tint.b));')
    multiply=mel.create_material_expression(mat,unreal.MaterialExpressionMultiply,600,500)
    mel.connect_material_expressions(sample,'RGB',multiply,'A');mel.connect_material_expressions(factor,'',multiply,'B');hook(multiply,prop)
    if name=='Ground':
     snow=mel.create_material_expression(mat,unreal.MaterialExpressionLinearInterpolate,800,500)
     mel.connect_material_expressions(multiply,'',snow,'A');mel.connect_material_expressions(tint,'',snow,'B')
     amount=custom(mat,'return step(.5,Tint.b)*.9;',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
     mel.connect_material_expressions(amount,'',snow,'Alpha');hook(snow,prop)
   else:hook(sample,prop,'RGB' if kind=='nor_dx' else 'R')

 hook(const(mat,.15 if name!='Water' else .2),unreal.MaterialProperty.MP_SPECULAR)
 if name in ['Foliage','Petal']:
  wind=custom(mat,'float strength=UV.y*UV.y;float sway=sin(P.x*.012+P.y*.008+Clock*1.4);return float3(sway*strength*5,sin(Clock*.9+P.x*.009)*strength*2,strength*sin(Clock+P.y*.01));')
  hook(wind,unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
 if name=='Metal':hook(const(mat,.78),unreal.MaterialProperty.MP_METALLIC)
 if name=='Sky':
  mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
  hook(color,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 if name=='Lava':hook(color,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 if name in ['Mist','Fire']:
  mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_ADDITIVE)
  mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
  hook(color,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  code='return pow(saturate(1-abs(UV.x*2-1)),2)*sin(saturate(UV.y)*3.14159)*.045;' if name=='Mist' else 'float f=n.fbm(float2(UV.x*5,UV.y*5-Clock*1.9));return saturate((1-abs(UV.x*2-1)-UV.y*.8+f*.5-.25)*3)*(1-UV.y)*.85;'
  hook(custom(mat,code,unreal.CustomMaterialOutputType.CMOT_FLOAT1),unreal.MaterialProperty.MP_OPACITY)
 if name=='Water':
  mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
  mat.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE)
  opacity=custom(mat,'return .78;',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
  hook(opacity,unreal.MaterialProperty.MP_OPACITY)
  normal=custom(mat,'return normalize(float3(.06*cos(P.x*.009+Clock*.7),.06*cos(P.y*.013-Clock*.9),1));')
  hook(normal,unreal.MaterialProperty.MP_NORMAL)
 mel.recompile_material(mat)
 assert lib.save_loaded_asset(mat,False),path
 unreal.log('STORY_ART_MATERIAL_READY '+path)
unreal.log('STORY_ART_MATERIALS_CREATED count=11')
