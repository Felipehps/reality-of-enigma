"""Run with Unreal's Python editor commandlet. Reuses the locally imported CC0 maps."""
import unreal
lib=unreal.EditorAssetLibrary
mel=unreal.MaterialEditingLibrary
folder='/Game/StoryEnvironments'
lib.make_directory(folder)
mat=lib.load_asset(folder+'/M_StoryStone') if lib.does_asset_exist(folder+'/M_StoryStone') else unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_StoryStone',folder,unreal.Material,unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(mat)
mat.set_editor_property('two_sided',True)
tint=mel.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-650,-200)
tint.set_editor_property('parameter_name','Color')
tint.set_editor_property('default_value',unreal.LinearColor(.15,.16,.17,1))
for i,(kind,prop) in enumerate([('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Normal',unreal.MaterialProperty.MP_NORMAL),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS),('AO',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)]):
 tex=lib.load_asset('/Game/ArtLibrary/FreeResources/T_DarkRock_'+kind)
 assert tex,kind
 node=mel.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-650,i*220)
 node.set_editor_property('texture',tex)
 node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='Normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='BaseColor' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
 if kind=='BaseColor':
  # Neutralize the dark source albedo enough to retain each region's authored tint.
  multiply=mel.create_material_expression(mat,unreal.MaterialExpressionMultiply,-350,0)
  mel.connect_material_expressions(node,'RGB',multiply,'A')
  mel.connect_material_expressions(tint,'',multiply,'B')
  gain=mel.create_material_expression(mat,unreal.MaterialExpressionMultiply,-150,0)
  gain.set_editor_property('const_b',2.5)
  mel.connect_material_expressions(multiply,'',gain,'A')
  mel.connect_material_property(gain,'',prop)
 else:
  mel.connect_material_property(node,'RGB' if kind=='Normal' else 'R',prop)
mel.recompile_material(mat)
assert lib.save_loaded_asset(mat,False)
unreal.log('STORY_STONE_PBR_CREATED')
