
- Newton's method was the main culprit making contact not work! 
- Now I'm working on restructing and adding general mesh-mesh contact.


- Current CCD, barrier energy value/grad/hessian does a lot of redundant calculation --> Add broad & narrow phase to 
  generate a set of collision pairs with distance value/grad/hessian already calculated. There should be different
  types that it will need to compute independently since different primitive pairs will have different barrier functions.

Note to self: for the mesh-mesh distance & barrier functions I need to consider how static objects DOFs
aren't actuallt really and can probably skip calculating some derivates maybe 

Ok doing some reading on broadphase partitioning techniques, oct-tree doesn't really seem like the way to go because it
partitions space and is hard to update for dynamic scenes. BVH for it's object partitioning and simpler updates seems
better and spatial hashing for its simplicity (however memory-hungry it may be) seem to be better options.

I would need to have bounding boxes have a buffer in each direction = contactDistance so that a bounding box overlap is
gauranteed to happen if objects are within contact distance of one another

I still don't have a great intuition for how the BVH would work for collision detection. During broadphase, would it only check 
for bounding box overlap between objects in the same direct parent? or that share a grandparent as well?

Maybe for each object, it starts at the root of the BVH (the AABB containing everything ig?) and checks for collision, then
it tries with the child nodes (sub AABB regions). It only continues checking deeper if it overlaps... this seems strange since 
we already kind of know which node the object is in. Although I suppose that doesn't really tell you all objects that are close,
just which are grouped together and multiple can be grouped together. 

OR other resources recommend often it's not the number of total objects that is the issue but the number of primitives? So each
object would have a large bounding box and there own internal BVH. However that doesn't allow for self-collision right? Maybe it
will be both, each object will need it's own BVH but a world one would help as well. 

Then each node will need to search the BVH for triangle within contact distance and each edge will need
to search the BVH for edges within contact distance. Both cases seem like overlap but I need to be careful
not to duplicate nodes by just searching for triangles near an edge...

Object movement means we have to adjust the BVH. Since move objects will be moving each frame, it might be worth 
further enlarging AABB boxes (larger than contactDistance) and only refitting if they move out of those boxes?... 

1) Refit ancestors --> can reduce quality of BVH
2) Rebuilding subtrees can be expensive
3) remove and re-insert (with tree rotations to make total SA contained in sibling nodes as even as possible)

Ok I'm going to put aside BVH for now but setup the collision code so that it's easy to add later
So thinking about broadphase, during line search we call IPValue a lot which means that if I'm going to 
utilize broadphase it needs to think about how IPValue will need testing different object locations. Maybe if I set a 
the buffer on the AABB boxes large enough maybe, but I think for fast moving objects that won't work...

Maybe instead I just need to consider those alpha steps the same as moving the object and need to update the scene each time :)
it'll have to be the same with ACCD since it slowly moves forward until we get an approximated alpha_toi. Although, it seems that 
collision pairs that are already in that contactDistance threshold if we add that contactDistnace buffer to all AABB will be
the first objects to hit during the ACCD iterative moving so it doesn't really matter. Not sure if the same logic applies to 
IPValue tho... 

IT DOES NOT! Ok so doing some more research, the contactDistance buffer seems to be a good idea but I also need
to consider every position long the search direction with alpha_max = 1, so all positions from x to x+p which 
is refered to as a swept volume. The problem is that the search direction is also determined by using barrier 
energy value/grad/hess which is part of what is supposed to be accelerated using the broadphase/BVH so the IPC paper
uses two stages:
- Static broadphase: fix using padded AABB bounding boxes and use to efficiently calculate search direction
- Swept Volume broadphase:  Using search direction to calculat swept volume AABB boxes and then use only those potential
                            collision pairs for ACCD and line-search

It may even be possible to 

Honestly it's the IPGrad and IPHessian which really need the help and could probably reuse the distance val/grad/hess values
since they are always called inbetween moves. 

I also need to consider while generating collision pairs is that node's shouldn't collide with the triangles 
they are a part of (it shouldn't generate any barrier energy)

Ok I really need to move energy methods to their own file and just pass position and other parameters and put broadphase stuff into
it's own file that can generate a vector of potential collision pairs that we can send to the barrier methods. The issue is that
they all use a lot of the same values that are currently fields of PhysicsEngine (h, contactDistance) and I might need to pass
the objects offsets array too... Actually that doesn't sound so bad, inertia and spring only really need pointmass, spring stiffness,
and edge lists (in addition to positions of course) and this further abstracts away the energy math from the engine.

I might also change how the sdf class works and probably add different barrier energy functions (for dist vs sqr dist methods)

I need the broadphase to generate a vector of collision pairs. The distance value/grad/hess
are kind of reused between the barrier energy val/grad/hess so those values for the collision 
pairs should be calculated before sending those vectors to the energy functions. *THAT IS*, until 
we begin ACCD and line search in which case those values need to be recomputed by just iterating over the
pair array and calling .compute or something that will need to be done separtely by each
function since they check for different things I think?... **(ACCD and line-search only use value, so no need to recompute grad & hess)**

It might also be better for MassSpring and Barrier energy at least to combine the energy value/grad/hess into a single loop since 
they do the same thing? That way it's only a single pass through the constraint list


PhysicsEngine
  simulation parameters
  object information

  updateObjects
  getSearchDir
  implicitSet

Utils
  makePSD (cause it will be used by contact (at least mesh-mesh), spring, and friction)

BroadPhase:
  

*In addition to positions & parameters*
inertia: nothing
spring: edges
gravity: none (since that's a param)
barrier: meshes (for contact area maybe?), edges, triangles, contact pairs

  




# TODO

- Refactoring:
  - [X] Make Renderer class separate from main that can be initialized later with the given JSON or replay saved animations
  - [X] Separate rendering and physics engine. B/c of interpolation and float cast we already have to copy shit over so just aim to link
  - [X] Read into ECSs: https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html

- QOL: 
  - [X] Make scene & simulation parameters setable from input file
  - [X] Add reset animation button
  - [X] Add single step button 
  - [X] Zoom in and out with camera
  - [ ] Make alpha lowerbound a setable parameter
  - [ ] **Animation saving & replaying**

- Optimizations
  - [X] Sparse Hessian Solver
  - [ ] Multithreading
  - [ ] GPU Optimizations
  - [ ] Broad/Narrow phase

- IPC:
  - [X] Fixed boundary condition
  - [ ] Moving boundary condition
  - [ ] Mesh on Mesh contact
  - [ ] Inversion free
  - [ ] Friction energy

- Bugs
  - [X] Fix sticky DBC Hessian transformation more (I think it's causing the fixed point to move around rn)
  - [X] Sometimes the engine will hit a minimum prematurely, resulting in a more sudden stop than is physically accurate. Lowering
  step size helps with this but that isn't always viable
  - [X] Take another look at the Projected Newton loop, in the TB it doesn't perform a single integration once the model comes
  to rest (p below tolerance) but I think I'm doing a single iteration each time (I think it's left over from testing)
  - [ ] If the .msh node tags are not 1-n and instead have a skip the parser breaks!

- JSON Parser
  - [X] Initial mesh conditions: fixed points, velocity, pre, post-init squishing
  - [X] Per object & global/default Bphong material parameters
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
  - [ ] Make Program a virtual class and add subclasses like BPhong program that handle the lights and sending them to the GPU
  (these subtypes should also handle setting the attributes and what not). Then, the shader of choice should be settable from
  the JSON file and all objects with the same set shader type should be grouped together (for each prog, bind, draw all, unbind).
  This should probably be handled just by the renderer (keep that shit out of main i think)

- Other:
  - [ ] Slip DBCs
  - [ ] Frame Interpolation: 
    - fixed but separate renderer & engine rates (former > latter) and interpolate engine calls:
    - https://kirbysayshi.com/2013/09/24/interpolated-physics-rendering.html
  - [X] Normal Movement: 
    - Normals needs to transform as the positions are transformed by the engine
    - It'd be expensive but simply recalculating vertex norms by recalculating and reaveraging triangle norms
  - [X] Double precision engine:
    - OpenGL should stay using floats, but it'd be nice to have the option to use double precision in the physics half
    - Could probably use c++ templates/generics since Matrix3Xf is just 'typedef Matrix< float, 3, Dynamic >' 
    - Since we're already doing a copy from Engine to individual objects (for energy, interpolation, etc) it should be fine
  - [ ] Evaluate accuracy of model against MATLAB's Simulink SDK:
