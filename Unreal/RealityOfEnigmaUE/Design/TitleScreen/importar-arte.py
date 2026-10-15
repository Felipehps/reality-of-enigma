import unreal, pathlib
file=pathlib.Path(__file__).resolve().with_name('Limiar-title-background.png')
t=unreal.AssetImportTask()
t.filename=str(file)
t.destination_path='/Game/GameArt'
t.destination_name='TitleLimiar'
t.automated=True
t.replace_existing=True
t.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
tex=unreal.EditorAssetLibrary.load_asset('/Game/GameArt/TitleLimiar')
assert tex
tex.set_editor_property('srgb',True)
tex.set_editor_property('never_stream',True)
tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
assert unreal.EditorAssetLibrary.save_loaded_asset(tex,False)
unreal.log('TITLE_BACKGROUND_IMPORT_OK '+tex.get_path_name())
