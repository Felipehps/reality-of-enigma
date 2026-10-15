import unreal,pathlib
source=pathlib.Path(__file__).resolve().parent
folder='/Game/CharacterModels/Hajun'
tools=unreal.AssetToolsHelpers.get_asset_tools()
task=unreal.AssetImportTask();task.filename=str(source/'Hajun-fase-1-otimizado.fbx');task.destination_path=folder;task.destination_name='SM_HajunPhase1';task.automated=True;task.save=True
options=unreal.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=False;options.import_animations=False;options.import_materials=False;options.import_textures=False
options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.automated_import_should_detect_type=False
options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.generate_lightmap_u_vs=False
task.options=options;tools.import_asset_tasks([task])
mesh=unreal.EditorAssetLibrary.load_asset(folder+'/SM_HajunPhase1');assert mesh
material=tools.create_asset('M_HajunPhase1',folder,unreal.Material,unreal.MaterialFactoryNew());assert material
material.set_editor_property('two_sided',True)
maps=[('basecolor',unreal.MaterialProperty.MP_BASE_COLOR),('metallic',unreal.MaterialProperty.MP_METALLIC),('roughness',unreal.MaterialProperty.MP_ROUGHNESS),('normal',unreal.MaterialProperty.MP_NORMAL)]
for index,(kind,prop) in enumerate(maps):
 file=next((source/'Textures').glob('*_'+kind+'.*'))
 task=unreal.AssetImportTask();task.filename=str(file);task.destination_path=folder;task.destination_name='T_Hajun_'+kind;task.automated=True;task.save=True;tools.import_asset_tasks([task])
 texture=unreal.EditorAssetLibrary.load_asset(folder+'/T_Hajun_'+kind);assert texture
 texture.set_editor_property('srgb',kind=='basecolor');texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_CHARACTER)
 if kind=='normal':texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
 elif kind!='basecolor':texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_GRAYSCALE)
 unreal.EditorAssetLibrary.save_loaded_asset(texture)
 node=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSample,-400,index*200);node.texture=texture
 node.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='basecolor' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
 unreal.MaterialEditingLibrary.connect_material_property(node,'RGB' if kind in ('normal','basecolor') else 'R',prop)
unreal.MaterialEditingLibrary.recompile_material(material);unreal.EditorAssetLibrary.save_loaded_asset(material)
mesh.set_material(0,material);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
assert mesh.get_bounds().box_extent.z>85
unreal.log('HAJUN_MODEL_IMPORT_OK mesh=120000-triangles textures=4 rig=none preview=static')
