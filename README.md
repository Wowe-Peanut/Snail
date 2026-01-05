
BVH:
- I would need to have bounding boxes have a buffer in each direction = contactDistance so that a bounding box overlap is
gauranteed to happen if objects are within contact distance of one another

Then each node will need to search the BVH for triangle within contact distance and each edge will need
to search the BVH for edges within contact distance. Both cases seem like overlap but I need to be careful
not to duplicate nodes by just searching for triangles near an edge...

Ok so doing some more research, the contactDistance buffer seems to be a good idea but I also need
to consider every position long the search direction with alpha_max = 1, so all positions from x to x+p which 
is refered to as a swept volume. The problem is that the search direction is also determined by using barrier 
energy value/grad/hess which is part of what is supposed to be accelerated using the broadphase/BVH so the IPC paper
uses two stages:
- Static broadphase: fix using padded AABB bounding boxes and use to efficiently calculate search direction
- Swept Volume broadphase:  Using search direction to calculat swept volume AABB boxes and then use only those potential
                            collision pairs for ACCD and line-search


I need the broadphase to generate a vector of collision pairs. The distance value/grad/hess
are kind of reused between the barrier energy val/grad/hess so those values for the collision 
pairs should be calculated before sending those vectors to the energy functions. *THAT IS*, until 
we begin ACCD and line search in which case those values need to be recomputed by just iterating over the
pair array and calling .compute or something that will need to be done separtely by each
function since they check for different things I think?... **(ACCD and line-search only use value, so no need to recompute grad & hess)**

It might also be better for MassSpring and Barrier energy at least to combine the energy value/grad/hess into a single loop since 
they do the same thing? That way it's only a single pass through the constraint list

The TB mentions taking a linear combination (usually 1/2 and 1/2) of triangle-node and edge-edge barrier energy discretizations but
I'm also still not super sure what the contact area between the two should be 🤷 and you also need to be careful not 
to not include edges in same triangles, nodes and an edge that includes it, etc... and to not iterate over
duplicate collision pairs

This has details on the contact area: https://phys-sim-book.github.io/lec24.1-barrier_and_dist.html
It seems for node-triangle, it seems to be 1/3 * #triangles that include node * area of contact triangle
For edge-edge, it seems to be 1/3 * (#edges involved ) * average area of triangles that include the contact edge?

Done:
  - Turn static into just a all dof = sticky DBC object (everything still in physics engine positions)
  - Keep using static to determine whether a mesh uses GL_DRAW_DYNAMIC or GL_DRAW_STATIC and use it in physics
  engine to determine which position data to send back to the mesh
  - Rename SDF files (and refactor includes), and remove SDF from objects and the parser

Todo
  - **Use Catch2 to create tests for the distance functions in a separate sandbox file (they will probably be the most error prone code I've hever had to write)**
  - Implement distance functions for all cases in a single file (standalone methods, no need for class, will be called by collision
  pair code)
  - Make file for collision pair code, CollisionPair should be a class with have fields for dval, dgrad, and dhess 
  and methods to compute each (fields b/c we are going to reuse the results and we want control over when they are calculated).

  - Add a method to physics_engine called generate_collision_pairs (right now it's just gonna brute force search, BVH and
  swept volume collision will come later) --> rn it should check calculate distance --> If < contactDist, calculate grad & hess
  (if specified since line-search and ACCD will need to cacll gen_collision_pairs again and won't need grad/hess). This list
  of pairs will be passed to the barrier energy functions 
  - Revamp the barrier energy methods, collision pairs and their distance val/grad/hess will already be computed, the barrier energy
  functions needs to compute barrier energy, cast hessians to SPD, and ensemble the local val/grad/hess.


Ok I'm adding catch2 test cases for the distant functions b/c of the hellish matrix calculus involved, but making
test cases by hand also seems tedious so I'm going to use python to a .txt file with test cases (inputs and answers)
that the catch2 .cpp will read, evaluate, and check it's answers against :)

Ok the paper mentions that although it's less efficient, dropping the second term from the barrier energy hessian
that involves the distance hessian (**at least just for the point-plane case*) is a decent enough "Gauss-newton" approximation.
Also the first term is already SDP so no extra eigenvalue-decomposition. I'm mainly choosing this since the 12x12 Hessian
calculations for the Point-Plane are incredibly annoying (I get it already...) I'll add it to the list of potential improvements. 
I believe this approximation is fine if the distances are small (which they are b/c we check that they're below contactDist each time).
I still need the distance gradient tho and add check for hess or just return zero



# TODO

- Refactoring:
  - [X] Make Renderer class separate from main that can be initialized later with the given JSON or replay saved animations
  - [X] Separate rendering and physics engine. B/c of interpolation and float cast we already have to copy shit over so just aim to link
  - [X] Read into ECSs: https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html
  - [X] Move JSON parser into own file and clean it up
  - [ ] CLEANUP NAMESPACES, INCLUDES, AND ALIASING 

- QOL: 
  - [X] Make scene & simulation parameters setable from input file
  - [X] Add reset animation button
  - [X] Add single step button 
  - [X] Zoom in and out with camera
  - [ ] Make alpha lowerbound a setable parameter
  - [ ] Change 'render_transform' and 'mesh_transform' to 'pre_init_transform' and 'post_init_scale'
  - [ ] **Animation saving & replaying**

- Optimizations
  - [X] Sparse Hessian Solver
  - [ ] Multithreading
  - [ ] GPU Optimizations
  - [ ] BVH
  - [ ] Avoid the unnecessary energy calculations for static objects

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
