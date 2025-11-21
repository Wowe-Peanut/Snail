# TODO

- Refactoring:
  - [X] Make Renderer class separate from main that can be initialized later with the given JSON or replay saved animations
  - [X] Separate rendering and physics engine. B/c of interpolation and float cast we already have to copy shit over so just aim to link
  - [ ] Get rid of compiler warnings...

- QOL: 
  - [X] Make scene & simulation parameters setable from input file
  - [X] Add reset animation button
  - [X] Add single step button 
  - [X] Zoom in and out with camera
  - [ ] **Animation saving & replaying**

- Optimizations
  - [X] Sparse Hessian Solver

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

- Meshes and Materials
  - [X] Be able to load gmsh .msh files that contain volumetric components
  - [X] It should only draw external triangles and not internal supports
  - [ ] Initial conditions setable in JSON (inc. fixed points, velocity, 
  initial transformation (pre-spring init), stretch scaling (post spring init))
  - [X] Calculate vertex normals by first calculating all triangle normals and then averaging
  - [ ] Somehow add texture mapping...
  - [ ] Per-object material qualities (spring stiffness, point mass)
  - [ ] Advanced materials w/ varying spring stiffness and point masses

- Shaders:
  - [X] Bphong
  - [ ] Add light count, positions, and bphong materials as JSON parameter
  - [ ] Texture map


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
