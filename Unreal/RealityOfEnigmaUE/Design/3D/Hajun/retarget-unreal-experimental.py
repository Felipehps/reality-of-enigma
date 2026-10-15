import unreal,pathlib,json
project=pathlib.Path(__file__).resolve().parents[3];root=project.parent;folder='/Game/CharacterModels/Hajun'
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary
task=unreal.AssetImportTask();task.filename=str(project/'Design/3D/Hajun/Rig/Hajun-fase1-rig-corrigido.fbx');task.destination_path=folder;task.destination_name='SK_HajunPhase1';task.automated=True;task.save=True
options=unreal.FbxImportUI();options.import_mesh=True;options.import_as_skeletal=True;options.import_animations=False;options.import_materials=False;options.import_textures=False;options.create_physics_asset=False;options.automated_import_should_detect_type=False;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH
task.options=options;target=lib.load_asset(folder+'/SK_HajunPhase1')
if not target:tools.import_asset_tasks([task]);target=lib.load_asset(folder+'/SK_HajunPhase1')
assert target
materials=target.get_editor_property('materials');materials[0].material_interface=lib.load_asset(folder+'/M_HajunPhase1');target.set_editor_property('materials',materials);lib.save_loaded_asset(target)
source=lib.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple');assert source
rigs=[]
for name,mesh in [('IK_MannyForHajun',source),('IK_HajunPhase1',target)]:
 rig=lib.load_asset(folder+'/'+name) or tools.create_asset(name,folder,unreal.IKRigDefinition,unreal.IKRigDefinitionFactory());assert rig
 controller=unreal.IKRigController.get_controller(rig);assert controller.set_skeletal_mesh(mesh)
 assert controller.apply_auto_generated_retarget_definition(),name+' skeleton not recognized'
 controller.apply_auto_fbik();lib.save_loaded_asset(rig);rigs.append(rig)
retarget=lib.load_asset(folder+'/RTG_MannyToHajun') or tools.create_asset('RTG_MannyToHajun',folder,unreal.IKRetargeter,unreal.IKRetargetFactory());assert retarget
c=unreal.IKRetargeterController.get_controller(retarget)
c.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE,rigs[0]);c.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET,rigs[1]);c.remove_all_ops();c.add_default_ops();c.auto_map_chains(unreal.AutoMapChainType.EXACT,True)
c.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET,unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN);lib.save_loaded_asset(retarget)
base='/Game/Characters/Mannequins/Anims/Unarmed/'
clips=[base+'MM_Idle',base+'Attack/MM_Attack_01',base+'Attack/MM_Attack_02',base+'Attack/MM_Attack_03',base+'Attack/MM_ChargedAttack']
inputs=unreal.IKRetargetBatchOperationInputs();inputs.assets_to_retarget=[lib.find_asset_data(p) for p in clips];inputs.source_mesh=source;inputs.target_mesh=target;inputs.ik_retarget_asset=retarget;inputs.target_path=folder+'/Animations';inputs.prefix='Hajun_';inputs.include_referenced_assets=False;inputs.overwrite_existing_files=True
results=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs);assert len(results)==5
for data in results:lib.save_loaded_asset(data.get_asset())
report={'mesh':target.get_path_name(),'height_cm':target.get_bounds().box_extent.z*2,'animations':[str(d.package_name) for d in results],'method':'Unreal IK Retargeter auto-characterization, exact chains, auto aligned target pose','combat_integrated':False}
(project/'Design/3D/Hajun/retarget-geracao.json').write_text(json.dumps(report,indent=2),encoding='utf-8');unreal.log('HAJUN_RETARGET_OK '+json.dumps(report))
