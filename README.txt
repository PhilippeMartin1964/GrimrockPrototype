FIREFX - Realistic Fire VFX
===========================
Version 1.0  |  Unreal Engine 5.0+  |  Niagara + Materials

Eighteen realistic Niagara fire systems: campfire, windy campfire, bonfire, torch, brazier, fireplace, candle,
fire burst, ground fire, wildfire, fire wall, magic fire, fire on shapes (circle / triangle / pentagram),
character fire (skeletal mesh), burning object (any static mesh) and a master system to build your own.

Every fire is built from layered emitters that read as real fire:
  - flame tongues (simulated, seamless 8x8 flipbook loops - wide and narrow sets - blended in the material)
  - flame puffs (one-shot simulated flipbook licks)
  - embers with curl noise, lit smoke, heat-haze refraction and a soft glow with a dynamic light


CONTENTS  (Content/FireFX)
--------
FX/          NS_FFX_Campfire, NS_FFX_Campfire_Windy, NS_FFX_Bonfire, NS_FFX_Torch, NS_FFX_Brazier,
             NS_FFX_Fireplace, NS_FFX_Candle, NS_FFX_Fire_Burst, NS_FFX_Ground_Fire, NS_FFX_Wildfire,
             NS_FFX_Fire_Wall, NS_FFX_Magic_Fire                         - placed fires
             NS_FFX_Shape_Fire (circle), NS_FFX_Shape_Triangle, NS_FFX_Shape_Pentagram  - fire along a shape mesh
             NS_FFX_Burning_Object                                       - fire on any static mesh (attach it to the mesh)
             NS_FFX_Character_Fire                                       - fire on a skeletal mesh (attach to it)
             NS_FFX_Fire                                                 - master system / template
             ET_FFX_Fire                                                 - effect type (Low / Medium / High scalability)
Materials/   M_FFX_Flame, M_FFX_Candle, M_FFX_Smoke, M_FFX_Ember, M_FFX_Glow, M_FFX_HeatHaze  (+ instances)
             M_FFX_Burnable  - surface material: char colour, glowing ember edges, burn-away mask (Burn 0..1)
             M_FFX_Scorch    - scorch decal (Decals/MI_FFX_Decal_Scorch, _Cold)
             Demo/           - simple materials used by the demo map
Meshes/      SM_FFX_Shape_Circle / Triangle / Pentagram / Line  - flat spawn shapes (CPU access enabled)
             SM_FFX_Demo_Crate, SM_FFX_Demo_Log                  - demo props
Textures/    T_FFX_FlameLoop_Wide / _Narrow (2048x4096), T_FFX_FlamePuff, T_FFX_Smoke, T_FFX_Candle,
             T_FFX_Ember, T_FFX_Spark, T_FFX_Glow, T_FFX_FireRamp, T_FFX_HeatNormal, T_FFX_NoisePack,
             T_FFX_Scorch_D / _M, T_FFX_DefaultARM
Maps/        L_FFX_Demo - every system placed and labelled, night lighting, movable lights only (nothing to build)


QUICK START
-----------
Drag any NS_FFX_* system into the level, or from Blueprint:
    Spawn System at Location / Spawn System Attached   System = NS_FFX_Campfire
Fires loop until the component is deactivated. NS_FFX_Fire_Burst is one-shot style (bursts every LoopTime).

Fire on a mesh:
    NS_FFX_Burning_Object samples the Static Mesh of the component it is ATTACHED to:
      - in a level: place the system on the mesh actor and attach it (drag the Niagara actor onto the mesh
        actor in the Outliner), or add a Niagara component as a child of the Static Mesh component;
      - in Blueprint: Spawn System Attached (NS_FFX_Burning_Object) to the Static Mesh component.
    Without a parent it uses the default mesh in User.StaticMesh. From Blueprint you can also pick any mesh with
    "Override System User Variable Static Mesh" (name: StaticMesh).
    NS_FFX_Shape_*: run along their own flat shape mesh; duplicate one and change User.StaticMesh in the system
    for your own runes / paths.
    The mesh needs "Allow CPU Access" enabled (Static Mesh editor > Details). See Burning_Crate in L_FFX_Demo.
Fire on a character:
    Spawn System Attached (NS_FFX_Character_Fire) to the character's Skeletal Mesh component. The Skeletal Mesh
    data interface picks up the mesh it is attached to, so the fire follows the animation.


USER PARAMETERS  (every system - override per component or with Set Niagara Variable)
---------------
FlameRate, FlameLife, FlameSizeMin / Max, FlameVelocity, FlameMat      flame tongues
PuffRate, PuffBurst, PuffSizeMin / Max, PuffVelocity, PuffMat          flame puffs
EmberRate, EmberBurst, EmberVelocity                                   embers
SmokeRate, SmokeBurst, SmokeSizeMin / Max, SmokeVelocity               smoke
GlowSize, LightColor, HazeRate                                         glow sprite, dynamic light, heat haze
CandleCount, CandleSize                                                candle flame emitter (0 = off)
SpawnBox                                                               size of the burning area (cm)
Wind                                                                   wind vector, bends flames / smoke / embers
Tint                                                                   colour multiplier (see Magic Fire)
LoopTime                                                               burst interval
StaticMesh / SkeletalMesh                                              (mesh systems only)


SCALABILITY
-----------
All systems use ET_FFX_Fire: Low = 30 %, Medium = 60 %, High / Epic / Cinematic = 100 % spawn counts, plus
distance and instance-count culling. It follows the project's Effects Quality (sg.EffectsQuality /
fx.Niagara.QualityLevel).


NOTES
-----
- Works in UE 5.0 and newer. No C++, no plugins besides Niagara. CPU simulation.
- The flame, smoke and candle flipbooks were simulated with a custom fluid solver and are original to this pack.
- Effects are not replicated; spawn them on every client.
- The props, ground textures and characters shown in the preview images are not part of this pack (CC0 props
  from Poly Haven and Epic's UE5 mannequins were used only for the screenshots). The demo map uses pack content.
