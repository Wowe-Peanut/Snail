# Currently working on:
- Ok, I can't run from it any longer. It was fine in python but now it's time to add contact to c++... 

Ok I'm currently still brainstorming how to structure everything.

I did some thinking on trying to make this an ECS but I've decided against it. *I don't believe the bottleneck is in iterating across*
*objects and the extra layer of indirection that OOP adds. I believe it will almost always be the super crazy*
*matrix math that I'm doing for Newton's method*. It addition, I've been reading how for complex branching logic and
prefab inheritence adds needless complexity. This isn't a game engine, these objects aren't zombies pathfinding
to the nearest player. *The complexity it would add doesn't seem worth it.*

On the other hand it did get me thinking more about broad & narrow phase now that I'm adding collisions. The constraints
are expensive and I need these checks. But that requires some sort of bounding boxs, and then that got me thinking about
how I want to store and define these costraints which got me thinking about how to improve how I store and define Objects.
I think the Object's 'isPhysicsObject' member is the perfect example now that I'm trying to add obstacles. I can't just
be adding a new fucking boolean for everything. *So I'm going back to my OOP roots* (Mr. B. would be proud). I think this 
will also allow me to separate the Renderer and Physics engine even more.

**New File Structure**

/resources
  /models
  /scenes
  /shaders

/extern (external shit)
  json.hpp
  stb_image_write.h
  tiny_obj_loader.h

/src
  main.cpp
  /Physics
    PhysicsEngine.cpp
    PhysicsEngine.h
  /Rendering
    Renderer.cpp
    Renderer.h
  /Util
    /Camera
      Camera.cpp
      Camera.h
    etc. for GLSL, MatrixStack, Object, & Shape

/build
.gitignore
README.md
CMakeLists.txt


**New Hierarchy**

Mesh (previously 'Shape'):
  - posBuf, norBuf, texBuf, indBuf, drawWithElements
  - draw(), load(), init()

  VolumetricMesh
    - Edge[], Triangle[], Tetrahedron[]

  SurfaceMesh
    - Edge[], Triangle[]


Object:
  - Shape, shaderProg, numPoints, numEdges, boundingBox
  - Object(), draw()

  PhysicsObject (dynamic objects, volumetric mesh)
    - 

  Obstacle (static objects, requires implementation of a SDF) 
    MeshObstacle
    SphereObstacle
    PlaneObstacle

struct Edge
  - vidx1, vidx2, restingLength

struct Triangle
  - vidx1, vidx2, vidx3

struct Tetrahedron
  - vidx1, vidx2, vidx3, vidx4


In addition to the new structure for Objects that I want, the broad phase shit gives me an opportunity to separate objects. 
The original reason I kept all the positions in a single array was b/c I was thinking about how the Hessian will rely on multiple
objects during collision, but broad phase kind of fixes that. My initial worry was the copying of all the objects in the same
'Island' to a single vector would be slow but I'm already kind of doing that when I copy back to the buffer and what not.
I'm not quite sure this part is true, but I also think it would speed up the search_direction calculation. The sparse solver 
probably does a good job handling this but separting things would probably be better. *I know for sure that it would*
*allowing for easier multithreading* and probably easier GPU implmentation as well but IDK yet (CUDA IS SCARY)

To handle this broad/narrow phase stuff and combining the objects into different 'islands' I kind of need to do some
psuedo-ECS in the physics engine. But I'm not quite sure how it should be structured... there will be meshes+meshes and
obstacle+mesh and meshes will need to consider (point, edge, & triangle primitives). It's all going to require changing
how the energy stuff is calculated (will have to construct and take in an island). 
**Though I wonder if the creation of these islands and dynamically making these new giant vectors will be slow...**
Maybe they could prealloc one giant one for each thread... idk yet

So lets just try running through an example

1) Iterate through all pairs of physics objects & check for 'islands' of bounding boxes (this should be transitive since 
if A touch B and B touch C, the Hessian of B is coupled with both and we can't separate them). This could probably be done 
fast (if we have lots of objects) using DSU (fast *disjoint set union*, thanks ACPC!) though it might not be worth it for 
small amounts of objects. 

2) Iterate through islands and create jobs
    - Singleton islands --> Can be simulated normally, *shouldn't be copied to some other class if it's by itself*
    - Mixed Islands --> Combine positions/velocities/DBCs of *non-obstacles*, constraints (both mesh-obstacle & mesh-mesh)
      - Do broad-ish phase: *only do the math for contact pairs whose boxes DIRECTlY OVERLAP*. E.g. A touch B, B touch C,
      the Hessians of A and C are coupled by B, but we don't need to consider contact pairs between A and C. 
      *These overlaps can probably be saved during island formation*
    - Obstacle Islands --> don't need processing, *should be skipped*

3) Dispath jobs to threads


# TODO

- Refactoring:
  - [X] Make Renderer class separate from main that can be initialized later with the given JSON or replay saved animations
  - [X] Separate rendering and physics engine. B/c of interpolation and float cast we already have to copy shit over so just aim to link
  - [ ] Get rid of compiler warnings...
  - [X] Read into ECSs: https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html

- QOL: 
  - [X] Make scene & simulation parameters setable from input file
  - [X] Add reset animation button
  - [X] Add single step button 
  - [X] Zoom in and out with camera
  - [ ] **Animation saving & replaying**

- Optimizations
  - [X] Sparse Hessian Solver
  - [ ] Multithreading (CUDA is too fucking scary)
  - [ ] GPU Optimizations
  - [ ] Broad/Narrow phase

- IPC:
  - [ ] Fixed boundary condition
  - [ ] Moving boundary condition
  - [ ] Mesh on Mesh contact
  - [ ] Inversion free
  - [ ] Friction energy

- Bugs
  - [X] Fix sticky DBC Hessian transformation more (I think it's causing the fixed point to move around rn)
  - [ ] Sometimes the engine will hit a minimum prematurely, resulting in a more sudden stop than is physically accurate. Lowering
  step size helps with this but that isn't always viable
  - [ ] Take another look at the Projected Newton loop, in the TB it doesn't perform a single integration once the model comes
  to rest (p below tolerance) but I think I'm doing a single iteration each time (I think it's left over from testing)

- JSON Parser
  - [ ] Initial mesh conditions: fixed points, velocity, pre, post-init squishing
  - [ ] Per object & global/default Bphong material parameters
  - [ ] Bphong lights
  - [ ] Per object spring stiffness and pointmass

- Meshes and Materials
  - [X] Be able to load gmsh .msh files that contain volumetric components
  - [X] It should only draw external triangles and not internal supports
  - [X] Calculate vertex normals by first calculating all triangle normals and then averaging
  - [ ] Somehow add texture mapping...
  - [ ] Advanced materials w/ varying spring stiffness and point masses

- Shaders:
  - [X] Bphong
  - [ ] Texture Shader
  - [ ] Add per-triangle shading (cubes look weird with smooth bphong)

- Other:
  - [ ] Slip DBCs
  - [ ] Frame Interpolation: 
    - fixed but separate renderer & engine rates (former > latter) and interpolate engine calls:
    - https://kirbysayshi.com/2013/09/24/interpolated-physics-rendering.html
  - [X] Normal Movement: 
    - Normals needs to transform as the positions are transformed by the engine
    - It'd be expensive but simply recalculating vertex norms by recalculating and reaveraging triangle norms
  - [ ] Doulbe precision engine:
    - OpenGL should stay using floats, but it'd be nice to have the option to use double precision in the physics half
    - Could probably use c++ templates/generics since Matrix3Xf is just 'typedef Matrix< float, 3, Dynamic >' 
    - Since we're already doing a copy from Engine to individual objects (for energy, interpolation, etc) it should be fine
  - [ ] Evaluate accuracy of model against MATLAB's Simulink SDK:
