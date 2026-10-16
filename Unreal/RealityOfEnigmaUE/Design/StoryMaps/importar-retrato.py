import pathlib,unreal
project=pathlib.Path(unreal.Paths.project_dir())
source=project/'Design/CharacterReferences/Separated/akuroth_phase1.png'
task=unreal.AssetImportTask()
task.set_editor_property('filename',str(source))
task.set_editor_property('destination_path','/Game/GameArt')
task.set_editor_property('destination_name','Hero_akuroth_phase1')
task.set_editor_property('automated',True)
task.set_editor_property('replace_existing',True)
task.set_editor_property('save',True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture=unreal.load_asset('/Game/GameArt/Hero_akuroth_phase1')
assert texture
texture.set_editor_property('srgb',True)
texture.set_editor_property('never_stream',True)
texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('STORY_PORTRAIT_IMPORTED '+texture.get_path_name())
