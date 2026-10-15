import bpy,math,pathlib,json
from mathutils import Vector
out=pathlib.Path(__file__).resolve().parent;out.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
collections={}
for name in ['01_Manto','02_Armadura','03_Coroa','04_Correntes','05_Espada','06_Referencia']:
 c=bpy.data.collections.new(name);scene.collection.children.link(c);collections[name]=c

def material(name,color,metallic=0,rough=.4):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;n=m.node_tree.nodes.get('Principled BSDF');n.inputs['Base Color'].default_value=(*color,1);n.inputs['Metallic'].default_value=metallic;n.inputs['Roughness'].default_value=rough;return m
black=material('Tecido negro',(.018,.006,.012),0,.7);red=material('Forro carmesim',(.22,.005,.015),.1,.45);gold=material('Ouro envelhecido',(.45,.23,.055),.85,.3);steel=material('Armadura do vazio',(.026,.021,.029),.7,.28)
def organize(obj,name,group,mat):
 obj.name=name
 for c in list(obj.users_collection):c.objects.unlink(obj)
 collections[group].objects.link(obj)
 if mat:obj.data.materials.append(mat)
 return obj

def beam(name,a,b,r,group,mat):
 a,b=Vector(a),Vector(b);d=b-a;bpy.ops.mesh.primitive_cone_add(vertices=12,radius1=r,radius2=r*.35,depth=d.length,location=(a+b)*.5);o=bpy.context.object;o.rotation_euler=d.to_track_quat('Z','Y').to_euler();organize(o,name,group,mat)
 for f in o.data.polygons:f.use_smooth=True
 return o

for panel in range(12):
 verts=[];faces=[];angle=panel*2*math.pi/12
 for row in range(13):
  t=row/12;radius=.23+.27*t
  for side in range(5):
   a=angle+(side/4-.5)*.42;length=[.87,1.02,.95][panel%3];verts.append((math.cos(a)*radius,math.sin(a)*radius,1.4-min(t*3,1)*.35-t*length+.015*math.sin(a*20)*t))
 for row in range(12):
  for col in range(4):
   a=row*5+col;faces.append((a,a+1,a+6,a+5))
 mesh=bpy.data.meshes.new('Painel tecido');mesh.from_pydata(verts,[],faces);mesh.update();obj=bpy.data.objects.new('Manto_%02d'%panel,mesh);collections['01_Manto'].objects.link(obj);mesh.materials.append(black);mesh.materials.append(red)
 for f in mesh.polygons:f.use_smooth=True
 mod=obj.modifiers.new('Espessura tecido','SOLIDIFY');mod.thickness=.003;mod.material_offset=1
 obj.shape_key_add(name='Basis');wind=obj.shape_key_add(name='Vento')
 for i,p in enumerate(wind.data):
  t=(i//5)/12;p.co.x+=math.cos(angle)*.05*t*t;p.co.y+=math.sin(angle)*.05*t*t;p.co.z+=.025*t*t
 for frame,value in [(1,0),(25,.8),(50,0),(75,-.5),(100,0)]:wind.value=value;wind.keyframe_insert(data_path='value',frame=frame)

for side in [-1,1]:
 bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,location=(side*.29,0,1.44),scale=(.18,.14,.10));organize(bpy.context.object,'Ombreira_'+str(side),'02_Armadura',steel)
 for spike in range(3):beam('Espinho ombro',(side*(.29+spike*.03),0,1.47),(side*(.34+spike*.06),.02,1.67-spike*.03),.025,'02_Armadura',steel)
for spike in range(5):beam('Ponta coroa',((spike-2)*.045,0,1.82),((spike-2)*.065,0,2.05-(spike%2)*.07),.014,'03_Coroa',steel)
for chain in range(2):
 for link in range(36):
  a=link*2*math.pi/36;pos=(math.cos(a)*.28,math.sin(a)*.28,1.04-chain*.09-.07*abs(math.sin(a)))
  bpy.ops.mesh.primitive_torus_add(major_segments=12,minor_segments=6,location=pos,major_radius=.017,minor_radius=.0035)
  o=organize(bpy.context.object,'Elo_%d_%02d'%(chain,link),'04_Correntes',gold);o.rotation_euler=(math.pi/2 if link%2 else 0,0,a)
# Weapon is separate and must be socketed and rigged after the character is authored.
beam('Punho espada',(.62,0,.6),(.62,0,.84),.019,'05_Espada',steel)
beam('Guarda espada',(.49,0,.84),(.75,0,.84),.018,'05_Espada',gold)
verts=[(.57,-.015,.84),(.67,-.015,.84),(.68,-.012,1.15),(.635,-.007,1.52),(.62,0,1.75),(.585,-.012,1.15),(.57,.015,.84),(.67,.015,.84),(.68,.012,1.15),(.635,.007,1.52),(.585,.012,1.15)]
faces=[(0,1,2,3,4,5),(6,10,4,9,8,7),(0,6,7,1),(1,7,8,2),(2,8,9,3),(3,9,4),(4,10,5),(5,10,6,0)]
m=bpy.data.meshes.new('Lamina sombra');m.from_pydata(verts,[],faces);m.update();o=bpy.data.objects.new('Lamina do vazio',m);collections['05_Espada'].objects.link(o);m.materials.append(steel)
image_path=out/'referencia.png';im=bpy.data.images.load(str(image_path));im.pack();ref=bpy.data.objects.new('ARTE ORIGINAL — referencia, nao modelo',None);ref.empty_display_type='IMAGE';ref.data=im;ref.empty_display_size=3;ref.location=(0,1.5,1.1);ref.rotation_euler=(math.pi/2,0,0);collections['06_Referencia'].objects.link(ref)
scene.frame_end=100;scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'AKUROTH-traje-provisorio.blend'))
bpy.ops.object.select_all(action='DESELECT')
for c in list(collections.values())[:-1]:
 for o in c.objects:
  if o.type=='MESH':o.select_set(True)
bpy.ops.export_scene.fbx(filepath=str(out/'AKUROTH-traje-provisorio.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z')
manifest={'blender':bpy.app.version_string,'objects':sum(1 for o in bpy.data.objects if o.type=='MESH'),'cloth_panels':12,'reference_packed':im.packed_file is not None,'body_included':False,'rig_included':False,'integrated_game_asset':False}
(out/'validacao.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8');print('AKUROTH_BLENDER_SOURCE_OK',json.dumps(manifest))
