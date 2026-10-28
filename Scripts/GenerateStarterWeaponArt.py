# -*- coding: utf-8 -*-
"""LOT02.9: original 11-piece Blender production batch, no Unreal mutation."""
import bpy, math, csv, sys
from pathlib import Path
from mathutils import Vector
from math import pi, sin, cos

WEAPONS = [
 ("ARM038","ShortSpear","spear"),("ARM048","RecurveBow","bow"),
 ("ARM054","ThrowingKnife","knife"),("ARM057","Javelin","javelin"),
 ("ARM058","Dart","dart"),("ARM061","ArcaneWand","arcane"),
 ("ARM062","FireWand","fire"),("ARM067","ElementalStaff","staff"),
 ("ARM069","HolyScepter","holy"),("ARM070","ArcaneOrb","orb"),
 ("ARM081","HolyAegis","shield")]
# Albedo RGB, roughness, metallic: physically-based, artist-review required.
COLORS = [((.16,.17,.19),.39,.83),((.50,.57,.62),.24,.91),
 ((.30,.16,.07),.75,.02),((.14,.065,.04),.83,.01),
 ((.50,.34,.13),.35,.80),((.82,.64,.27),.27,.86),
 ((.13,.27,.83),.19,.15),((.98,.17,.06),.23,.08),
 ((.81,.88,.96),.20,.25),((.68,.60,.43),.72,.03),
 ((.11,.16,.24),.85,.02),((.055,.06,.075),.53,.70),
 ((.20,.45,.28),.36,.10),((.53,.43,.27),.60,.69),
 ((.40,.09,.12),.86,.02),((.73,.73,.78),.27,.77)]
IDX={"iron":0,"steel":1,"wood":2,"leather":3,"brass":4,"gold":5,
 "arcane":6,"ember":7,"holy":8,"bone":9,"cloth":10,"dark":11,
 "jade":12,"bronze":13,"red":14,"silver":15}

def tag(o,c): o["atlas_cell"]=IDX[c];return o
def rod(n,z,h,r,c):
 bpy.ops.mesh.primitive_cylinder_add(vertices=14,radius=r,depth=h,location=(0,0,z))
 o=bpy.context.object;o.name=n;return tag(o,c)
def tip(n,z,h,r,c):
 bpy.ops.mesh.primitive_cone_add(vertices=8,radius1=r,radius2=0,depth=h,location=(0,0,z))
 o=bpy.context.object;o.name=n;return tag(o,c)
def gem(n,z,r,c,x=0,y=0):
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=r,location=(x,y,z))
 o=bpy.context.object;o.name=n;return tag(o,c)
def ring(n,z,r,c,x=0,y=0,tilt=0):
 bpy.ops.mesh.primitive_torus_add(major_segments=24,minor_segments=8,
     major_radius=r,minor_radius=.013,location=(x,y,z),rotation=(tilt,0,0))
 o=bpy.context.object;o.name=n;return tag(o,c)
def block(n,z,dim,c):
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,0,z))
 o=bpy.context.object;o.name=n;o.dimensions=dim
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 return tag(o,c)
def wire(n,points,width,c):
 cu=bpy.data.curves.new(n,"CURVE");cu.dimensions="3D";cu.bevel_depth=width
 cu.bevel_resolution=3
 sp=cu.splines.new("POLY");sp.points.add(len(points)-1)
 for pt,xyz in zip(sp.points,points):pt.co=(*xyz,1)
 o=bpy.data.objects.new(n,cu);bpy.context.collection.objects.link(o)
 bpy.ops.object.select_all(action="DESELECT");o.select_set(True)
 bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target="MESH")
 return tag(bpy.context.object,c)

def handle(low,high,r=.045,c="wood"):
 rod("Hand-carved shaft",(low+high)/2,high-low,r,c)
 for z in (low+.11*(high-low),high-.10*(high-low)):
  ring("Forged collar",z,r*1.19,"brass")

def build(kind):
 if kind=="spear":
  handle(-1.05,.58);rod("Socket",.62,.20,.075,"iron")
  tip("Leaf-spearhead",.86,.48,.11,"steel");tip("Rear spike",-1.08,.14,.035,"iron")
 elif kind=="bow":
  wire("Laminated recurve limb",[(0,0,-.92),(.11,0,-.71),
       (.19,0,-.37),(.27,0,0),(.19,0,.37),(.11,0,.71),(0,0,.92)],.040,"wood")
  wire("Bowstring",[(0,0,-.92),(-.13,0,0),(0,0,.92)],.005,"silver")
  block("Leather grip",0,(.16,.11,.27),"leather")
  for z in (-.90,.90):gem("Horn limb tip",z,.055,"bone")
 elif kind=="knife":
  rod("Balanced handle",-.39,.38,.062,"leather")
  gem("Weighted pommel",-.62,.07,"brass");block("Guard",-.16,(.30,.10,.075),"iron")
  tip("Double-edge tapered blade",.29,.78,.11,"steel")
 elif kind=="javelin":
  handle(-1.02,.67,.029);tip("Light leaf head",.81,.39,.065,"steel")
  for z in (-.76,-.61,-.46):ring("Binding",z,.031,"leather")
 elif kind=="dart":
  rod("Short dart shaft",-.24,.50,.039,"dark")
  tip("Envenomed hollow needle",.19,.42,.077,"steel")
  gem("Emerald tailpiece",-.48,.085,"jade")
  for z in (-.20,-.08,.05):ring("Brass pin",z,.047,"brass")
 elif kind in ("arcane","fire"):
  handle(-.73,.53,.060,"dark" if kind=="fire" else "wood")
  for z in (-.53,-.33,.21,.44):ring("Runic inlay",z,.071,"gold")
  rod("Focus socket",.60,.18,.092,"brass")
  gem("Faceted elemental charge",.83,.20,"ember" if kind=="fire" else "arcane")
  ring("Crystal retaining halo",.83,.20,"gold",tilt=pi/2)
 elif kind=="staff":
  handle(-1.08,.65,.06,"dark")
  for z in (-.85,-.43,.16,.53):ring("Brass glyph",z,.072,"gold")
  rod("Head socket",.68,.20,.11,"bronze")
  gem("Runic central crystal",.98,.22,"arcane")
  ring("Floating energy gimbal",.98,.26,"silver",tilt=pi/3)
  for i,c in enumerate(("ember","jade","holy")):
   gem("Elemental satellite",.92+.12*sin(2*pi*i/3),.083,c,
       x=.26*cos(2*pi*i/3),y=.26*sin(2*pi*i/3))
 elif kind=="holy":
  handle(-.9,.44,.059,"bone")
  for z in (-.68,-.31,.13,.40):ring("Blessed seal",z,.071,"gold")
  gem("Holy scepter stone",.71,.23,"holy")
  ring("Sacred halo",.71,.23,"gold",tilt=pi/2)
  wire("Saint's crown",[(-.27,0,.64),(0,0,1.04),(.27,0,.64)],.029,"gold")
 elif kind=="orb":
  gem("Faceted mana orb",0,.37,"arcane")
  ring("Gimbal outer",0,.47,"brass",tilt=pi/2)
  ring("Gimbal inner",0,.40,"silver",tilt=pi/4)
  ring("Equatorial rune-ring",0,.45,"gold")
  for i in range(6):
   a=2*pi*i/6;gem("Retaining rune",.03,.05,"gold",x=.46*cos(a),y=.46*sin(a))
 elif kind=="shield":
  o=rod("Dark iron convex boss",0,.10,.63,"dark");o.rotation_euler.x=pi/2
  ring("Aegis engraved rim",0,.63,"gold",tilt=pi/2)
  ring("Inner glyph circle",0,.48,"brass",tilt=pi/2)
  gem("Sacred heart",0,.25,"holy",y=-.15)
  for i in range(8):
   a=2*pi*i/8;gem("Eight sacred rivets",.43*sin(a),.034,"gold",x=.43*cos(a),y=-.10)
 else:raise ValueError(kind)

def textures(folder,slug):
 folder.mkdir(parents=True,exist_ok=True);result={}
 for channel in ("BC","N","ORM"):
  pixels=[]
  for y in range(256):
   for x in range(256):
    col,rough,metal=COLORS[min(3,y//64)*4+min(3,x//64)]
    noise=.96+.04*sin(x*.23+y*.11)
    v=(*[min(1,c*noise) for c in col],1.) if channel=="BC" else (
       (.5,.5,1.,1.) if channel=="N" else (1.,rough,metal,1.))
    pixels.extend(v)
  img=bpy.data.images.new("T_"+slug+"_"+channel,width=256,height=256,alpha=True)
  img.pixels[:]=pixels
  if channel!="BC":img.colorspace_settings.name="Non-Color"
  img.filepath_raw=str(folder/("T_"+slug+"_01_"+channel+".png"))
  img.file_format="PNG";img.save();result[channel]=img
 return result

def shader(images,slug):
 mat=bpy.data.materials.new("M_"+slug+"_PBR");mat.use_nodes=True
 nodes=mat.node_tree.nodes;nodes.clear()
 out=nodes.new("ShaderNodeOutputMaterial")
 p=nodes.new("ShaderNodeBsdfPrincipled")
 for key in ("BC","ORM","N"):
  t=nodes.new("ShaderNodeTexImage");t.image=images[key]
  if key=="BC":mat.node_tree.links.new(t.outputs["Color"],p.inputs["Base Color"])
  if key=="ORM":
   split=nodes.new("ShaderNodeSeparateColor")
   mat.node_tree.links.new(t.outputs["Color"],split.inputs[0])
   mat.node_tree.links.new(split.outputs["Green"],p.inputs["Roughness"])
   mat.node_tree.links.new(split.outputs["Blue"],p.inputs["Metallic"])
 mat.node_tree.links.new(p.outputs["BSDF"],out.inputs["Surface"])
 return mat

def join_and_map(material,slug):
 meshlist=[x for x in bpy.context.scene.objects if x.type=="MESH"]
 for o in meshlist:
  cell=int(o.get("atlas_cell",0))
  uv=o.data.uv_layers.active or o.data.uv_layers.new(name="UVMap")
  vx=[o.matrix_world@v.co for v in o.data.vertices]
  lo=min(v.x for v in vx);hi=max(v.x for v in vx)
  b=min(v.z for v in vx);t=max(v.z for v in vx)
  for poly in o.data.polygons:
   for li in poly.loop_indices:
    v=vx[o.data.loops[li].vertex_index]
    u=.05+.90*(v.x-lo)/max(.0001,hi-lo)
    w=.05+.90*(v.z-b)/max(.0001,t-b)
    uv.data[li].uv=((cell%4+u)/4,(cell//4+w)/4)
  o.data.materials.clear();o.data.materials.append(material)
 bpy.ops.object.select_all(action="DESELECT")
 for o in meshlist:o.select_set(True)
 bpy.context.view_layer.objects.active=meshlist[0]
 bpy.ops.object.join()
 obj=bpy.context.object;obj.name="SM_Weapon_"+slug+"_01"
 return obj

def studio():
 target=Vector((0,0,0))
 bpy.ops.object.camera_add(location=(2.3,-3.3,1.4))
 camera=bpy.context.object
 camera.rotation_euler=(target-camera.location).to_track_quat("-Z","Y").to_euler()
 camera.data.type="ORTHO";camera.data.ortho_scale=3.2
 bpy.context.scene.camera=camera
 for pos,power in [((3,-4,4),800),((-3,-2,2),380),((0,3,4),900)]:
  bpy.ops.object.light_add(type="AREA",location=pos)
  light=bpy.context.object
  light.data.energy=power;light.data.size=3.1
  light.rotation_euler=(target-light.location).to_track_quat("-Z","Y").to_euler()
 sc=bpy.context.scene
 sc.render.engine="BLENDER_EEVEE_NEXT"
 sc.render.film_transparent=True
 sc.render.resolution_x=512;sc.render.resolution_y=512
 sc.render.resolution_percentage=100
 sc.render.image_settings.file_format="PNG"
 sc.render.image_settings.color_mode="RGBA"

def main():
 args=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
 root=Path(args[0]).resolve() if args else Path.cwd()/"Saved"/"ARMEMENT-LOT02"/"LOT02_9_ArtSource"
 root.mkdir(parents=True,exist_ok=True)
 rows=[]
 for ident,slug,kind in WEAPONS:
  bpy.ops.wm.read_factory_settings(use_empty=True)
  build(kind)
  folder=root/ident
  maps=textures(folder,slug)
  obj=join_and_map(shader(maps,slug),slug)
  studio()
  icon=folder/("T_Weapon_"+slug+"_01.png")
  bpy.context.scene.render.filepath=str(icon)
  bpy.ops.render.render(write_still=True)
  blend=folder/(slug+"_01.blend")
  bpy.ops.wm.save_as_mainfile(filepath=str(blend))
  fbx=folder/("SM_Weapon_"+slug+"_01.fbx")
  bpy.ops.object.select_all(action="DESELECT")
  obj.select_set(True);bpy.context.view_layer.objects.active=obj
  bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,
         object_types={"MESH"},axis_forward="-Y",axis_up="Z",path_mode="COPY")
  paths=[fbx,icon,folder/("T_"+slug+"_01_BC.png"),
          folder/("T_"+slug+"_01_N.png"),folder/("T_"+slug+"_01_ORM.png"),blend]
  if not all(f.exists() and f.stat().st_size>0 for f in paths):
   raise RuntimeError("Missing art file for "+ident)
  rows.append([ident,slug,kind,*[str(f.relative_to(root)) for f in paths],"NEEDS_VISUAL_REVIEW"])
  print("[LOT02.9] GENERATED "+ident+" "+slug,flush=True)
 with (root/"LOT02_9_ArtProduction.csv").open("w",newline="",encoding="utf-8") as f:
  writer=csv.writer(f)
  writer.writerow(["ArchetypeId","Name","Kind","FBX","IconRGBA",
    "BaseColor","Normal","ORM","BlenderSource","ReviewStatus"])
  writer.writerows(rows)
 print("[LOT02.9] COMPLETED 11 original weapon source sets",flush=True)
if __name__=="__main__":main()
