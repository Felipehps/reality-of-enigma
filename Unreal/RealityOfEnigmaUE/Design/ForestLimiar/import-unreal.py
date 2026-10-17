"""Import Blender forest kit and create free PBR/foliage materials in Unreal."""
import unreal, json, shutil
from pathlib import Path
R = Path(__file__).resolve().parent
folder = '/Game/StoryEnvironments/Forest'
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()
lib.make_directory(folder)

def constant(mat, value):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionConstant)
    n.set_editor_property('r', value)
    return n

def color(mat, value):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
    n.set_editor_property('constant', unreal.LinearColor(*value, 1))
    return n

def connect(node, prop, pin=''):
    assert mel.connect_material_property(node, pin, prop)

def parameter(mat, name, value):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
    n.set_editor_property('parameter_name', name)
    n.set_editor_property('default_value', value)
    return n

def texture(name, file, kind):
    path = folder + '/T_' + name
    tex = lib.load_asset(path) if lib.does_asset_exist(path) else None
    if not tex:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', str(file))
        task.set_editor_property('destination_path', folder)
        task.set_editor_property('destination_name', 'T_' + name)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        assets.import_asset_tasks([task])
        tex = lib.load_asset(path) if lib.does_asset_exist(path) else None
    assert tex, path
    tex.set_editor_property('srgb', kind == 'diff')
    tex.set_editor_property('max_texture_size', 2048)
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP if kind == 'normal' else unreal.TextureCompressionSettings.TC_DEFAULT if kind == 'diff' else unreal.TextureCompressionSettings.TC_GRAYSCALE)
    if kind == 'normal':
        tex.set_editor_property('flip_green_channel', True)
    assert lib.save_loaded_asset(tex, False)
    return tex

def sample(mat, tex, kind, uv=None):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample)
    n.set_editor_property('texture', tex)
    n.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind == 'normal' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if kind == 'diff' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    if uv:
        mel.connect_material_expressions(uv, '', n, 'Coordinates')
    return n

def custom(mat, code, inputs, out=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionCustom)
    n.set_editor_property('code', code)
    n.set_editor_property('output_type', out)
    entries = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property('input_name', name)
        entries.append(entry)
    n.set_editor_property('inputs', entries)
    for name, node in inputs.items():
        mel.connect_material_expressions(node, '', n, name)
    return n

def mul(mat, a, b):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply)
    mel.connect_material_expressions(a, '', n, 'A')
    mel.connect_material_expressions(b, '', n, 'B')
    return n
materials = {}
for name, parent, tint in [('ForestWood', 'Wood', (0.13, 0.08, 0.035)), ('ForestWater', 'Water', (0.006, 0.037, 0.03))]:
    path = folder + '/M_' + name
    mat = (lib.load_asset(path) if lib.does_asset_exist(path) else None) or assets.create_asset('M_' + name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mat, lib.load_asset('/Game/StoryEnvironments/M_Story' + parent))
    mel.set_material_instance_vector_parameter_value(mat, 'Color', unreal.LinearColor(*tint, 1))
    lib.save_loaded_asset(mat, False)
    materials[name] = mat
specs = {'TreeTrunk': ('tree_small_02', ''), 'TreeBranch': ('tree_small_02', 'branch_'), 'TreeLeaves': ('tree_small_02', 'leaves_'), 'ForestFern': ('fern_02', '')}
for name in ['ForestGround', 'ForestStoneSurface', 'ForestMoss', 'ForestGrass', 'ForestCrystal'] + list(specs):
    mat = (lib.load_asset(folder + '/M_' + name) if lib.does_asset_exist(folder + '/M_' + name) else None) or assets.create_asset('M_' + name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided', name in ['ForestGrass', 'TreeLeaves', 'ForestFern'])
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED if name in ['TreeLeaves', 'ForestFern'] else unreal.BlendMode.BLEND_OPAQUE)
    connect(constant(mat, 0.85), unreal.MaterialProperty.MP_ROUGHNESS)
    connect(constant(mat, 0.18), unreal.MaterialProperty.MP_SPECULAR)
    pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition)
    uv = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate)
    clock = parameter(mat, 'SceneTime', 0)
    if name in ['TreeLeaves', 'ForestFern', 'ForestGrass']:
        mat.set_editor_property('shading_model', next((getattr(unreal.MaterialShadingModel, x) for x in dir(unreal.MaterialShadingModel) if 'FOLIAGE' in x)))
    if name in specs:
        slug, group = specs[name]
        base = None
        for kind, suffix, ext, prop in [('diff', 'diff', 'jpg', unreal.MaterialProperty.MP_BASE_COLOR), ('normal', 'nor_gl', 'exr', unreal.MaterialProperty.MP_NORMAL), ('rough', 'rough', 'exr', unreal.MaterialProperty.MP_ROUGHNESS)]:
            tex = texture(name + '_' + kind, R / 'Sources/textures' / f'{slug}_{group}{suffix}_2k.{ext}', kind)
            n = sample(mat, tex, kind, uv)
            if kind == 'diff':
                gain = color(mat, (0.7, 0.78, 0.6) if name in ['TreeLeaves', 'ForestFern'] else (0.85, 0.85, 0.8))
                base = mul(mat, n, gain)
                connect(base, prop)
            else:
                connect(n, prop, 'RGB' if kind == 'normal' else 'R')
        if name in ['TreeLeaves', 'ForestFern']:
            tex = texture(name + '_alpha', R / 'Sources/textures' / f'{slug}_{group}alpha_2k.png', 'mask')
            connect(sample(mat, tex, 'mask', uv), unreal.MaterialProperty.MP_OPACITY_MASK, 'R')
            mat.set_editor_property('opacity_mask_clip_value', 0.33)
            connect(mul(mat, base, color(mat, (0.35, 0.45, 0.22))), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    elif name == 'ForestGround':
        world_uv = custom(mat, 'return P.xy*.0025;', {'P': pos}, unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        for suffix, kind, prop in [('diff', 'diff', unreal.MaterialProperty.MP_BASE_COLOR), ('nor_dx', 'normal', unreal.MaterialProperty.MP_NORMAL), ('rough', 'rough', unreal.MaterialProperty.MP_ROUGHNESS), ('ao', 'mask', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)]:
            tex = lib.load_asset('/Game/StoryEnvironments/T_StoryGround_' + suffix)
            assert tex
            n = sample(mat, tex, kind, world_uv)
            if suffix == 'diff':
                connect(mul(mat, n, color(mat, (0.48, 0.58, 0.43))), prop)
            else:
                connect(n, prop, 'RGB' if kind == 'normal' else 'R')
    elif name == 'ForestStoneSurface':
        normal_ws=mel.create_material_expression(mat,unreal.MaterialExpressionVertexNormalWS)
        moss=custom(mat,'''struct F{float h(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}float n(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(h(i),h(i+float2(1,0)),f.x),lerp(h(i+float2(0,1)),h(i+1),f.x),f.y);}};F n;float patch=n.n(P.xy*.008+P.z*.003)*.65+n.n(P.xy*.029)*.35;return smoothstep(.28,.78,N.z)*smoothstep(.34,.64,patch);''',{'P':pos,'N':normal_ws},unreal.CustomMaterialOutputType.CMOT_FLOAT1)
        stone_uv=custom(mat,'return P.xy*.0015+float2(0,P.z*.0015);',{'P':pos},unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        for suffix,kind,prop in [('BaseColor','diff',unreal.MaterialProperty.MP_BASE_COLOR),('Normal','normal',unreal.MaterialProperty.MP_NORMAL),('Roughness','rough',unreal.MaterialProperty.MP_ROUGHNESS),('AO','mask',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)]:
            tex=lib.load_asset('/Game/ArtLibrary/FreeResources/T_DarkRock_'+suffix);assert tex
            n=sample(mat,tex,kind,stone_uv)
            if suffix=='BaseColor':
                base=mul(mat,n,color(mat,(.85,.9,.8)));lerp=mel.create_material_expression(mat,unreal.MaterialExpressionLinearInterpolate)
                mel.connect_material_expressions(base,'',lerp,'A');mel.connect_material_expressions(color(mat,(.025,.065,.008)),'',lerp,'B');mel.connect_material_expressions(moss,'',lerp,'Alpha');connect(lerp,prop)
            else:connect(n,prop,'RGB' if kind=='normal' else 'R')
    elif name == 'ForestMoss':
        n = custom(mat, 'float grain=frac(sin(dot(floor(P.xy*.15+P.z*.12),float2(127.1,311.7)))*43758.54);return lerp(float3(.025,.052,.008),float3(.085,.16,.02),grain);', {'P': pos})
        connect(n, unreal.MaterialProperty.MP_BASE_COLOR)
    elif name == 'ForestGrass':
        base = custom(mat, 'float v=.7+frac(sin(dot(floor(P.xy*.025),float2(12.98,78.23)))*43758.5)*.5;return lerp(float3(.018,.055,.006),float3(.09,.17,.025),UV.y)*v;', {'P': pos, 'UV': uv})
        connect(base, unreal.MaterialProperty.MP_BASE_COLOR)
        connect(mul(mat, base, color(mat, (0.4, 0.5, 0.2))), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    else:
        connect(color(mat, (0.015, 0.22, 0.15)), unreal.MaterialProperty.MP_BASE_COLOR)
        connect(color(mat, (0.02, 1.2, 0.62)), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name in ['TreeLeaves', 'ForestFern', 'ForestGrass']:
        wind = custom(mat, 'float anchor=' + ('saturate(P.z*.001)' if name == 'TreeLeaves' else 'UV.y*UV.y') + ';float s=sin(P.x*.003+P.y*.002+Clock*.9);return float3(s*anchor*' + ('6' if name == 'TreeLeaves' else '2') + ',cos(Clock*.8+P.y*.004)*anchor,0);', {'P': pos, 'UV': uv, 'Clock': clock})
        connect(wind, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    if name in ['TreeTrunk','TreeBranch','TreeLeaves','ForestFern']:
        usage=next(getattr(unreal.MaterialUsage,x) for x in dir(unreal.MaterialUsage) if 'INSTANCED' in x and 'STATIC' in x)
        mel.set_material_usage(mat,usage)
    mel.recompile_material(mat)
    assert lib.save_loaded_asset(mat, False)
    materials['ForestStone' if name=='ForestStoneSurface' else name] = mat
    unreal.log('FOREST_MATERIAL_READY ' + name)
report = {}
for name in ['LimiarTree', 'LimiarFern', 'LimiarClearing']:
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(R / 'Export' / (name + '.fbx')))
    task.set_editor_property('destination_path', folder)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    options = unreal.FbxImportUI()
    options.set_editor_property('automated_import_should_detect_type', False)
    options.set_editor_property('import_mesh', True)
    options.set_editor_property('import_as_skeletal', False)
    options.set_editor_property('import_animations', False)
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)
    options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.static_mesh_import_data.set_editor_property('combine_meshes', True)
    options.static_mesh_import_data.set_editor_property('generate_lightmap_u_vs', False)
    options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
    options.static_mesh_import_data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    task.set_editor_property('options', options)
    assets.import_asset_tasks([task])
    mesh = lib.load_asset(folder + '/' + name)
    assert mesh, name
    slots = mesh.get_editor_property('static_materials')
    names = []
    for index,slot in enumerate(slots):
        key = str(slot.get_editor_property('imported_material_slot_name'))
        key = key.split('.')[0]
        assert key in materials, (name, key)
        mesh.set_material(index,materials[key])
        names.append(key)
    assert all(mesh.get_material(i) and mesh.get_material(i).get_path_name().startswith(folder+'/M_') for i in range(len(slots))),name
    assert lib.save_loaded_asset(mesh, False)
    bounds = mesh.get_bounds()
    report[name] = {'extent': [bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z], 'origin': [bounds.origin.x,bounds.origin.y,bounds.origin.z], 'slots': names}
    unreal.log('FOREST_MESH_READY ' + name + ' ' + str(report[name]))
layout = R.parents[1] / 'Content/GameData/forest-limiar-layout.json'
shutil.copy2(R / 'forest-layout.json', layout)
(R / 'import-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('FOREST_UNREAL_IMPORT_OK meshes=3 materials=' + str(len(materials)))
