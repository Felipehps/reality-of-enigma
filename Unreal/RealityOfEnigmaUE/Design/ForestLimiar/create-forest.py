"""Original Limiar ruin/clearing assembled in Blender; CC0 tree and fern sources are credited separately."""
import bpy,math,random,json
from pathlib import Path
from mathutils import Vector,noise
R=Path(__file__).resolve().parent;SRC=R/'Sources';EXPORT=R/'Export';EXPORT.mkdir(exist_ok=True)
random.seed(71026)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
materials={}
def material(name,color,rough=.85,texture=None,alpha=None,emission=0):
 m=bpy.data.materials.new(name);m.use_nodes=True;n=m.node_tree.nodes;l=m.node_tree.links;bs=n.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough
 if texture:
  tex=n.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(texture),check_existing=True);l.new(tex.outputs['Color'],bs.inputs['Base Color'])
 if alpha:
  a=n.new('ShaderNodeTexImage');a.image=bpy.data.images.load(str(alpha),check_existing=True);a.image.colorspace_settings.name='Non-Color';l.new(a.outputs['Color'],bs.inputs['Alpha']);m.surface_render_method='DITHERED'
 if emission:bs.inputs['Emission Color'].default_value=(*color,1);bs.inputs['Emission Strength'].default_value=emission
 materials[name]=m;return m
tex=SRC/'textures';old=R.parent/'StoryEnvironments/TextureSources';stone=R.parents[1]/'..'
material('ForestGround',(.12,.15,.075),texture=old/'forrest_ground_01_diff_2k.jpg')
material('ForestStone',(.23,.27,.24),texture=SRC/'textures/dark_rock_diff_2k.jpg');material('ForestMoss',(.055,.12,.025));material('ForestWood',(.13,.08,.035),texture=old/'bark_brown_01_diff_2k.jpg')
material('ForestGrass',(.065,.14,.025));material('ForestWater',(.018,.09,.085),.16);material('ForestCrystal',(.025,.68,.49),.2,emission=2.5)
material('TreeTrunk',(.15,.11,.06),texture=tex/'tree_small_02_diff_2k.jpg')
material('TreeBranch',(.13,.1,.04),texture=tex/'tree_small_02_branch_diff_2k.jpg')
material('TreeLeaves',(.1,.22,.04),texture=tex/'tree_small_02_leaves_diff_2k.jpg',alpha=tex/'tree_small_02_leaves_alpha_2k.png')
material('ForestFern',(.05,.16,.03),texture=tex/'fern_02_diff_2k.jpg',alpha=tex/'fern_02_alpha_2k.png')
objects=[]
def mesh(name,verts,faces,mat,smooth=False,uv=None):
 data=bpy.data.meshes.new(name);data.from_pydata(verts,[],faces);data.update();obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);data.materials.append(materials[mat]);objects.append(obj)
 for p in data.polygons:p.use_smooth=smooth
 layer=data.uv_layers.new(name='UVMap')
 for p in data.polygons:
  for li in p.loop_indices:
   vi=data.loops[li].vertex_index;v=data.vertices[vi].co;layer.data[li].uv=uv[vi] if uv else (v.x*.35,v.y*.35) if abs(p.normal.z)>.5 else (v.x*.35+v.y*.35,v.z*.35)
 return obj

def box(name,p,size,mat='ForestStone',rot=0,bevel=.06):
 x,y,z=[v*.5 for v in size];c=math.cos(rot);s=math.sin(rot)
 vs=[(p[0]+c*a-s*b,p[1]+s*a+c*b,p[2]+d) for a,b,d in [(-x,-y,-z),(x,-y,-z),(x,y,-z),(-x,y,-z),(-x,-y,z),(x,-y,z),(x,y,z),(-x,y,z)]]
 obj=mesh(name,vs,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],mat)
 if bevel:
  mod=obj.modifiers.new('WornEdges','BEVEL');mod.width=bevel;mod.segments=2
  mod=obj.modifiers.new('CornerNormals','WEIGHTED_NORMAL')
 return obj

def curve(name,points,radius,mat='ForestWood'):
 data=bpy.data.curves.new(name,'CURVE');data.dimensions='3D';data.resolution_u=8;data.bevel_depth=radius;data.bevel_resolution=3
 sp=data.splines.new('BEZIER');sp.bezier_points.add(len(points)-1)
 for i,(pt,p) in enumerate(zip(sp.bezier_points,points)):pt.co=p;pt.handle_left_type='AUTO';pt.handle_right_type='AUTO';pt.radius=1-i/(len(points)-1)*.87
 obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);data.materials.append(materials[mat]);objects.append(obj);return obj

def h(x,y):
 r=max(abs(x)/11,abs(y)/6);b=max(0,min(1,(r-1)/1.7));b=b*b*(3-2*b)
 return -.035+b*(noise.fractal(Vector((x*.075,y*.075,2.3)),1.1,2.1,3)*1.4+.4*math.sin(x*.12+y*.17))
# Continuous sloped terrain, with a genuinely level gameplay footprint.
vs=[];uv=[];faces=[];NX=110;NY=110
for j in range(NY+1):
 for i in range(NX+1):
  x=(i/NX-.5)*100;y=(j/NY-.68)*100;vs.append((x,y,h(x,y)));uv.append((x*.4,y*.4))
for j in range(NY):
 for i in range(NX):
  k=j*(NX+1)+i;faces.append((k,k+1,k+NX+2,k+NX+1))
mesh('LivingForestFloor',vs,faces,'ForestGround',True,uv)
# Broken pilgrim path continues through the clearing toward the portal.
for j in range(15):
 y=5-j*1.65;cx=math.sin(j*.45)*1.2
 for k in range(random.choice([2,3])):
  x=cx+(k-1)*1.35+random.uniform(-.14,.14)
  box('SunkenPath',(x,y,h(x,y)-.04),(random.uniform(.9,1.4),random.uniform(.9,1.3),.14),'ForestStone',random.uniform(-.18,.18),.09)
# Main landmark: masonry portal, asymmetrical damage, ribs, and an overgrown side wall.
Y=-22;Z=h(0,Y)
for side in [-1,1]:
 for i in range(8):
  box('PortalPier',(side*3.55+random.uniform(-.07,.07),Y,Z+.36+i*.66),(1.1,1.5,.64),'ForestStone',random.uniform(-.025,.025),.09)
 box('PierFoot',(side*3.55,Y,Z+.18),(1.7,2,.45),bevel=.1)
 box('PierCapital',(side*3.55,Y,Z+5.42),(1.45,1.8,.4),bevel=.08)
 for groove in [-.25,0,.25]:curve('PierFlute',[(side*3.55+groove,Y+.78,Z+.8),(side*3.55+groove,Y+.79,Z+4.9)],.045,'ForestMoss')
for i in range(21):
 if i in [3,4]:continue
 a=i*math.pi/21+.012;b=(i+1)*math.pi/21-.012
 verts=[]
 for yy in [-.72,.72]:
  for rr,aa in [(3.48,a),(4.26,a),(4.26,b),(3.48,b)]:verts.append((rr*math.cos(aa),Y+yy,Z+5.38+rr*math.sin(aa)))
 obj=mesh('ArchVoussoir',verts,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],'ForestStone')
 mod=obj.modifiers.new('WeatheredStoneEdges','BEVEL');mod.width=.065;mod.segments=2
 mod=obj.modifiers.new('StoneNormals','WEIGHTED_NORMAL')
for i in range(12):
 x=-4.4-i*.78
 for row in range(max(1,int(4.7-i*.28))):box('BrokenAbbeyWall',(x,Y-.5,Z+row*.6+.3),(random.uniform(.7,.85),1.0,.57),rot=random.uniform(-.05,.05),bevel=.07)
for i in range(24):
 x=random.choice([-1,1])*random.uniform(4.2,11);y=Y+random.uniform(-2.5,4)
 obj=box('FallenMasonry',(x,y,h(x,y)+.2),(random.uniform(.5,1.25),random.uniform(.4,1),random.uniform(.35,.65)),rot=random.uniform(0,6.28),bevel=.1)
# Broken seal fragments make a distinct focal point inside the ruined portal.
for segment in range(5):
 points=[]
 for k in range(12):
  a=segment*math.pi*.4+.08+k*.075
  points.append((math.cos(a)*1.85,Y+.1,Z+4.85+math.sin(a)*1.85))
 curve('FracturedSeal',points,.055,'ForestCrystal')
for i in range(4):
 cx=random.uniform(-.65,.65);cy=Y+.1;zz=Z+3.1+i*.7;r=.2
 mesh('SuspendedRelic',[(cx-r,cy,zz),(cx,cy-r,zz),(cx+r,cy,zz),(cx,cy+r,zz),(cx+.1,cy,zz+.65),(cx-.08,cy,zz-.3)],[(0,1,4),(1,2,4),(2,3,4),(3,0,4),(1,0,5),(2,1,5),(3,2,5),(0,3,5)],'ForestCrystal')
# Tangled roots wrap masonry and bind the scene to its ground.
for side in [-1,1]:
 for i in range(8):
  a=i*.7;start=(side*3.6,Y+.78,Z+random.uniform(1,6));end=(side*(5+random.random()*5),Y+random.uniform(3,8),h(side*7,Y+5)+.07)
  curve('CreepingRoot',[start,(start[0]+side*.6,Y+1.2,Z+.4),(end[0],end[1]-1,h(end[0],end[1])+.12),end],random.uniform(.055,.16))
for i in range(9):
 x=random.uniform(-3.3,3.3)
 curve('HangingIvy',[(x,Y+.8,Z+8.6-abs(x)*.35),(x+.25,Y+1,Z+6.4),(x-.25,Y+1.1,Z+random.uniform(3.8,5.6))],.035,'ForestMoss')
# Actual multi-faceted crystals rather than a flat spell illustration.
for i in range(11):
 x=random.choice([-1,1])*random.uniform(4,7.5);y=random.uniform(-21,-17);z=h(x,y);rad=random.uniform(.10,.26);height=random.uniform(.45,1.6)
 verts=[(x+math.cos(j*math.pi/3)*rad,y+math.sin(j*math.pi/3)*rad,z+height*.2) for j in range(6)]+[(x,y,z+height)]
 mesh('RiftCrystal',verts,[tuple(reversed(range(6)))]+[(j,(j+1)%6,6) for j in range(6)],'ForestCrystal')
# Boulder variants are smooth but cragged, with smaller moss patches on upper surfaces.
for i in range(46):
 x=random.uniform(-29,29);y=random.uniform(-39,11)
 if abs(x)<12 and y>-9:continue
 if abs(x)<4.5:continue
 scale=random.uniform(.6,2.2);z=h(x,y)
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3,radius=1,location=(x,y,z+scale*.25));obj=bpy.context.object;obj.name='WeatheredBoulder';obj.scale=(scale*1.3,scale,scale*.8)
 for v in obj.data.vertices:v.co*=1+noise.noise(v.co*3.7)*.18
 obj.data.materials.append(materials['ForestStone']);objects.append(obj)
 for p in obj.data.polygons:p.use_smooth=True
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=1,location=(x,y,z+scale*.76));cap=bpy.context.object;cap.name='MossCushion';cap.scale=(scale*.85,scale*.68,scale*.15);cap.data.materials.append(materials['ForestMoss']);objects.append(cap)
 for p in cap.data.polygons:p.use_smooth=True
# A narrow stream runs outside the hero lanes, beneath plants and stones.
vs=[];faces=[]
for j in range(70):
 y=10-j*.72;cx=14.5+math.sin(y*.13)*1.5
 for side in [-1,1]:vs.append((cx+side*1.0,y,-.22))
 if j:faces.append((j*2-2,j*2,j*2+1,j*2-1))
mesh('SideStream',vs,faces,'ForestWater',True)
# Low clustered grass uses real blade geometry; the battle footprint remains sparse.
vs=[];faces=[];uv=[]
for i in range(3500):
 x=random.uniform(-33,33);y=random.uniform(-44,15)
 if abs(x)<11.5 and -7<y<7:continue
 if abs(x-math.sin((5-y)/1.65*.45)*1.2)<2.1 and y<6:continue
 for blade in range(random.randint(4,7)):
  a=random.random()*6.28;length=random.uniform(.18,.7);w=random.uniform(.018,.045);px=x+random.uniform(-.12,.12);py=y+random.uniform(-.12,.12);pz=h(px,py)
  start=len(vs)
  for j in range(3):
   t=j*.5;cx=px+math.cos(a)*length*t*t*.42;cy=py+math.sin(a)*length*t*t*.42
   for side in [-1,1]:vs.append((cx+math.sin(a)*w*side*(1-t*.85),cy-math.cos(a)*w*side*(1-t*.85),pz+length*t));uv.append(((side+1)*.5,t))
  faces.extend([(start,start+1,start+3,start+2),(start+2,start+3,start+5,start+4)])
mesh('WildGrassClusters',vs,faces,'ForestGrass',True,uv)
# Dark foreground crags and moss create a frame around the lit clearing.
for side in [-1,1]:
 for i in range(4):
  x=side*(15.8+i*1.3);y=4-i*4.2;z=h(x,y)
  bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3,radius=1,location=(x,y,z+1.0+i*.4));obj=bpy.context.object;obj.name='ForegroundCliff';obj.scale=(3.2,3.8,2.6+i*.65)
  for v in obj.data.vertices:v.co*=1+noise.noise(v.co*4.2)*.28
  obj.data.materials.append(materials['ForestStone']);objects.append(obj)
  for poly in obj.data.polygons:poly.use_smooth=True
  bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3,radius=1,location=(x-.3*side,y,z+3.1+i*.9));cap=bpy.context.object;cap.name='CliffMoss';cap.scale=(2.6,3.1,.65);cap.data.materials.append(materials['ForestMoss']);objects.append(cap)
  for poly in cap.data.polygons:poly.use_smooth=True
# Import only useful source objects, not multi-million-polygon LOD0 or geometry-node collections.
with bpy.data.libraries.load(str(SRC/'tree_small_02.blend'),link=False) as (a,b):b.objects=[n for n in a.objects if n=='tree_small_02_LOD1']
tree_parts=[]
for obj in b.objects:
 if obj:
  scene.collection.objects.link(obj);print('TREE_PART',obj.name,len(obj.data.polygons),[m.name for m in obj.data.materials],flush=True)
  for slot in obj.material_slots:
   name=slot.material.name.lower();slot.material=materials['TreeLeaves' if 'leav' in name else 'TreeBranch' if 'branch' in name else 'TreeTrunk']
  bpy.context.view_layer.objects.active=obj;obj.select_set(True)
  dec=obj.modifiers.new('GameMeshReduction','DECIMATE');dec.ratio=.085 if 'LOD1' in obj.name else .22;bpy.ops.object.modifier_apply(modifier=dec.name);tree_parts.append(obj);obj.select_set(False)
bpy.ops.object.select_all(action='DESELECT')
for obj in tree_parts:obj.select_set(True)
bpy.context.view_layer.objects.active=tree_parts[0];bpy.ops.object.join();tree=bpy.context.object;tree.name='LimiarTree';tree.location=(0,0,0)
for p in tree.data.polygons:p.use_smooth=True
with bpy.data.libraries.load(str(SRC/'fern_02.blend'),link=False) as (a,b):b.objects=['fern_02_b']
fern=b.objects[0];scene.collection.objects.link(fern);fern.name='LimiarFern';fern.location=(0,0,0);fern.data.materials.clear();fern.data.materials.append(materials['ForestFern'])
# Moss belongs to the stone surface instead of stacked cushion meshes.
for obj in list(objects):
 if obj.name.startswith(('CliffMoss','MossCushion')):objects.remove(obj);bpy.data.objects.remove(obj,do_unlink=True)
# Match the editor study to the PBR materials recreated in Unreal.
for name,slug,group in [('TreeTrunk','tree_small_02',''),('TreeBranch','tree_small_02','branch_'),('TreeLeaves','tree_small_02','leaves_'),('ForestFern','fern_02','')]:
 mat=materials[name];nodes=mat.node_tree.nodes;links=mat.node_tree.links;bs=nodes.get('Principled BSDF')
 for suffix,socket in [('rough','Roughness'),('nor_gl','Normal')]:
  im=nodes.new('ShaderNodeTexImage');im.image=bpy.data.images.load(str(tex/f'{slug}_{group}{suffix}_2k.exr'),check_existing=True);im.image.colorspace_settings.name='Non-Color'
  if suffix=='nor_gl':normal=nodes.new('ShaderNodeNormalMap');links.new(im.outputs['Color'],normal.inputs['Color']);links.new(normal.outputs['Normal'],bs.inputs[socket])
  else:links.new(im.outputs['Color'],bs.inputs[socket])
mat=materials['ForestGround'];nodes=mat.node_tree.nodes;links=mat.node_tree.links;bs=nodes.get('Principled BSDF');base=next(n for n in nodes if n.type=='TEX_IMAGE');multiply=nodes.new('ShaderNodeMixRGB');multiply.blend_type='MULTIPLY';multiply.inputs[0].default_value=1;multiply.inputs[2].default_value=(.48,.58,.43,1);links.new(base.outputs['Color'],multiply.inputs[1]);links.new(multiply.outputs[0],bs.inputs['Base Color'])
mat=materials['ForestMoss'];nodes=mat.node_tree.nodes;links=mat.node_tree.links;bs=nodes.get('Principled BSDF');grain=nodes.new('ShaderNodeTexNoise');grain.inputs['Scale'].default_value=45;bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.65;bump.inputs['Distance'].default_value=.08;links.new(grain.outputs['Fac'],bump.inputs['Height']);links.new(bump.outputs['Normal'],bs.inputs['Normal'])
# Export a lean reusable tree and fern separately; instances are shared in Unreal.
def export(name,items):
 bpy.ops.object.select_all(action='DESELECT')
 for obj in items:obj.select_set(True)
 bpy.context.view_layer.objects.active=items[0]
 bpy.ops.export_scene.fbx(filepath=str(EXPORT/(name+'.fbx')),use_selection=True,object_types={'MESH','OTHER'},apply_unit_scale=True,global_scale=1,axis_forward='-Y',axis_up='Z',bake_anim=False,use_mesh_modifiers=True,mesh_smooth_type='FACE',path_mode='STRIP',use_triangles=True)
export('LimiarTree',[tree]);export('LimiarFern',[fern]);export('LimiarClearing',objects)
# Instanced composition in the .blend matches the placement manifest used by Unreal.
placements={'trees':[],'ferns':[]}
for i in range(85):
 if i<12:x=random.choice([-1,1])*random.uniform(16,25);y=random.uniform(-5,12)
 else:x=random.uniform(-36,36);y=random.uniform(-49,-11)
 if abs(x)<5.5 and y>-28:continue
 scale=random.uniform(1.5,3.4);yaw=random.uniform(0,6.28);z=h(x,y)-.06
 obj=bpy.data.objects.new('CanopyTree',tree.data);scene.collection.objects.link(obj);obj.location=(x,y,z);obj.scale=(scale,scale,scale*random.uniform(.95,1.2));obj.rotation_euler.z=yaw
 placements['trees'].append({'p':[x*100,y*100,z*100],'s':list(obj.scale),'yaw':math.degrees(yaw)})
for i in range(520):
 x=random.uniform(-29,29);y=random.uniform(-37,12)
 if abs(x)<11.8 and -7<y<7:continue
 if abs(x)<3 and -26<y<8:continue
 scale=random.uniform(.6,1.4);yaw=random.uniform(0,6.28);z=h(x,y)-.02
 obj=bpy.data.objects.new('UnderstoryFern',fern.data);scene.collection.objects.link(obj);obj.location=(x,y,z);obj.scale=(scale,)*3;obj.rotation_euler.z=yaw
 placements['ferns'].append({'p':[x*100,y*100,z*100],'s':[scale]*3,'yaw':math.degrees(yaw)})
tree.hide_render=True;fern.hide_render=True;tree.hide_set(True);fern.hide_set(True)
# A reproducible camera/light study; this render is explicitly a Blender preview, not a game capture.
bpy.ops.object.camera_add(location=(1.2,31,9));camera=bpy.context.object;camera.rotation_euler=(Vector((0,-18,5))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.lens=35;scene.camera=camera
bpy.ops.object.light_add(type='SUN',location=(0,0,20));sun=bpy.context.object;sun.rotation_euler=(math.radians(28),math.radians(-22),math.radians(-35));sun.data.energy=2.3;sun.data.angle=.14;sun.data.color=(1,.83,.57)
bpy.ops.object.light_add(type='AREA',location=(0,4,16));fill=bpy.context.object;fill.data.energy=1800;fill.data.shape='DISK';fill.data.size=20;fill.data.color=(.45,.67,.75)
scene.world.color=(.08,.12,.16)
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.threads_mode='FIXED';scene.render.threads=4
scene.render.resolution_x=1600;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(R/'blender-preview.png')
scene.view_settings.view_transform='AgX'
placements['asset_triangles']={'tree':sum(len(p.vertices)-2 for p in tree.data.polygons),'fern':sum(len(p.vertices)-2 for p in fern.data.polygons)}
placements['battle_clearance_cm']={'min':[-1100,-600,-4],'max':[1100,600,0]}
(R/'forest-layout.json').write_text(json.dumps(placements,indent=2),encoding='utf-8')
for im in bpy.data.images:
 if im.filepath and Path(im.filepath).is_file() :im.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(R/'Floresta-do-Limiar.blend'),compress=True)
print('FOREST_BLENDER_ASSETS_OK',placements['asset_triangles'],'trees',len(placements['trees']),'ferns',len(placements['ferns']),flush=True)
# Rendering is a separate invocation so export success cannot be hidden by a long preview render.
