import unreal,pathlib,json
root=pathlib.Path(unreal.Paths.project_dir());source=root/'Design/3D/Akuroth';folder='/Game/CharacterModels/Akuroth';tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary;mel=unreal.MaterialEditingLibrary;report={}
def task(file,name,opt):
 t=unreal.AssetImportTask();t.filename=str(file);t.destination_path=folder;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True;t.options=opt;tools.import_asset_tasks([t]);return lib.load_asset(folder+'/'+name)
def opts(kind):
 o=unreal.FbxImportUI();o.automated_import_should_detect_type=False;o.import_mesh=True;o.import_animations=False;o.import_materials=False;o.import_textures=False;o.create_physics_asset=False;o.mesh_type_to_import=kind;return o
for phase in [1,2]:
 f=source/('Fase'+str(phase));name='SK_AkurothPhase'+str(phase);o=opts(unreal.FBXImportType.FBXIT_SKELETAL_MESH);o.import_as_skeletal=True;mesh=task(f/'Idle.fbx',name,o);assert mesh,name
 mat=lib.load_asset(folder+'/M_Akuroth'+str(phase)) or tools.create_asset('M_Akuroth'+str(phase),folder,unreal.Material,unreal.MaterialFactoryNew());mat.set_editor_property('two_sided',True);mel.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
 mel.delete_all_material_expressions(mat)
 for idx,(kind,prop) in enumerate([('basecolor',unreal.MaterialProperty.MP_BASE_COLOR),('metallic',unreal.MaterialProperty.MP_METALLIC),('roughness',unreal.MaterialProperty.MP_ROUGHNESS),('normal',unreal.MaterialProperty.MP_NORMAL)]):
  file=next((f/'Textures').glob(kind+'.*'));tex=task(file,'T_AK%d_%s'%(phase,kind),None);assert tex
  tex.set_editor_property('srgb',kind=='basecolor');tex.set_editor_property('max_texture_size',4096 if kind=='basecolor' else 2048)
  if kind=='normal':tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
  elif kind!='basecolor':tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_GRAYSCALE)
  lib.save_loaded_asset(tex,False);n=mel.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-400,idx*180);n.texture=tex;n.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='basecolor' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE;mel.connect_material_property(n,'RGB' if kind in ['basecolor','normal'] else 'R',prop)
 mel.recompile_material(mat);lib.save_loaded_asset(mat);mats=mesh.get_editor_property('materials');slot=mats[0];slot.material_interface=mat;mats[0]=slot;mesh.set_editor_property('materials',mats);lib.save_loaded_asset(mesh,False)
 skeleton=mesh.get_editor_property('skeleton');anims=[]
 for kind in ['Idle','Slash','Cast','Chaos','Ultimate','Transform','Death']:
  o=opts(unreal.FBXImportType.FBXIT_ANIMATION);o.import_mesh=False;o.import_as_skeletal=True;o.import_animations=True;o.skeleton=skeleton;o.override_animation_name='AK_%d_%s'%(phase,kind)
  clip=task(f/(kind+'.fbx'),'AK_%d_%s'%(phase,kind),o);assert clip,'Missing '+kind;anims.append(clip.get_path_name())
 report['phase'+str(phase)]={'mesh':mesh.get_path_name(),'height_cm':mesh.get_bounds().box_extent.z*2,'animations':anims};assert 170<mesh.get_bounds().box_extent.z*2<195
# UE runtime materials for the geometry's authored slots.
materials=[]
for name,color,metal,rough,glow in [('Basalto',(.065,.08,.105),.08,.82,0),('Pedra',(.18,.18,.20),.1,.8,0),('Bronze',(.26,.115,.045),.72,.43,0),('Lava',(.65,.018,.003),.05,.5,4)]:
 mat=lib.load_asset(folder+'/M_'+name) or tools.create_asset('M_'+name,folder,unreal.Material,unreal.MaterialFactoryNew());mel.delete_all_material_expressions(mat)
 n=mel.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-300,0);n.constant=unreal.LinearColor(*color,1);mel.connect_material_property(n,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for prop,val in [(unreal.MaterialProperty.MP_METALLIC,metal),(unreal.MaterialProperty.MP_ROUGHNESS,rough)]:
  n=mel.create_material_expression(mat,unreal.MaterialExpressionConstant,-300,180);n.r=val;mel.connect_material_property(n,'',prop)
 if glow:
  n=mel.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-300,360);n.constant=unreal.LinearColor(4,.14,.008,1);mel.connect_material_property(n,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 mel.recompile_material(mat);lib.save_loaded_asset(mat);materials.append(mat)
for phase in [1,2]:
 o=opts(unreal.FBXImportType.FBXIT_STATIC_MESH);o.import_as_skeletal=False;o.static_mesh_import_data.combine_meshes=True;o.static_mesh_import_data.generate_lightmap_u_vs=False
 mesh=task(source/('Arena'+str(phase))/('Arena'+str(phase)+'.fbx'),'SM_Arena'+str(phase),o);assert mesh
 slots=mesh.get_editor_property('static_materials')
 for idx,slot in enumerate(slots):
  label=str(slot.material_slot_name);mat=next((m for m in materials if m.get_name()=='M_'+label),materials[min(idx,3)]);mesh.set_material(idx,mat)
 lib.save_loaded_asset(mesh,False);report['arena'+str(phase)]={'mesh':mesh.get_path_name(),'slots':[str(s.material_slot_name) for s in slots],'bounds_cm':list(mesh.get_bounds().box_extent.to_tuple())}
(root/'Saved/Akuroth-importacao-Unreal.json').write_text(json.dumps(report,indent=2),encoding='utf-8');unreal.log('AKU_TRIPO_IMPORT_OK '+json.dumps(report))

import unreal
lib=unreal.EditorAssetLibrary;mel=unreal.MaterialEditingLibrary;folder='/Game/CharacterModels/Akuroth'
for name,low,high in [('Basalto',(.012,.016,.023),(.038,.045,.061)),('Pedra',(.028,.031,.039),(.085,.082,.087))]:
 mat=lib.load_asset(folder+'/M_'+name);assert mat;mel.delete_all_material_expressions(mat)
 a=mel.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-600,0);a.constant=unreal.LinearColor(*low,1)
 b=mel.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-600,150);b.constant=unreal.LinearColor(*high,1)
 noise=mel.create_material_expression(mat,unreal.MaterialExpressionNoise,-600,300);noise.set_editor_property('scale',.035);noise.set_editor_property('quality',1);noise.set_editor_property('levels',2);noise.set_editor_property('output_min',0);noise.set_editor_property('output_max',1)
 mix=mel.create_material_expression(mat,unreal.MaterialExpressionLinearInterpolate,-200,0);mel.connect_material_expressions(a,'',mix,'A');mel.connect_material_expressions(b,'',mix,'B');mel.connect_material_expressions(noise,'',mix,'Alpha');mel.connect_material_property(mix,'',unreal.MaterialProperty.MP_BASE_COLOR)
 r=mel.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,200);r.r=.85;mel.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS);mel.recompile_material(mat);lib.save_loaded_asset(mat)
mat=lib.load_asset(folder+'/M_Lava');mel.delete_all_material_expressions(mat);color=mel.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-500,0);color.constant=unreal.LinearColor(2,.045,.003,1);time=mel.create_material_expression(mat,unreal.MaterialExpressionTime,-800,250);wave=mel.create_material_expression(mat,unreal.MaterialExpressionSine,-600,250);wave.set_editor_property('period',2);assert mel.connect_material_expressions(time,'',wave,'');scale=mel.create_material_expression(mat,unreal.MaterialExpressionMultiply,-400,250);scale.set_editor_property('const_b',.12);mel.connect_material_expressions(wave,'',scale,'A');level=mel.create_material_expression(mat,unreal.MaterialExpressionAdd,-200,250);level.set_editor_property('const_b',.88);mel.connect_material_expressions(scale,'',level,'A');emit=mel.create_material_expression(mat,unreal.MaterialExpressionMultiply,0,0);mel.connect_material_expressions(color,'',emit,'A');mel.connect_material_expressions(level,'',emit,'B');mel.connect_material_property(emit,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);mel.recompile_material(mat);lib.save_loaded_asset(mat);unreal.log('AKU_ARENA_SURFACE_OK')
