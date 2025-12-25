
- Newton's method was the main culprit making contact not work! 
- Now I'm working on restructing and adding general mesh-mesh contact.


- Current CCD, barrier energy value/grad/hessian does a lot of redundant calculation --> Add broad & narrow phase to 
  generate a set of collision pairs with distance value/grad/hessian already calculated. There should be different
  types that it will need to compute independently since different primitive pairs will have different barrier functions.



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
