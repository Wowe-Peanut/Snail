# Currently working on:
- I stared into the void of volumetric mesh generation and the void stared back... so I'm using gmash and meshlabs instead

# TODO

- Refactoring:
  - [X] Make Renderer class separate from main that can be initialized later with the given JSON or replay saved animations
  - [X] Separate rendering and physics engine. B/c of interpolation and float cast we already have to copy shit over so just aim to link
  - [ ] Get rid of compiler warnings...

- QOL: 
  - [X] Make scene & simulation parameters setable from input file
  - [ ] Add fixed points, velocity, and stretch initial condition parameters to json
  - [X] Add reset animation button
  - [X] Add single step button 
  - [X] Zoom in and out with camera
  - [ ] Add light count, positions, and colors as JSON parameter

- IPC:
  - [ ] Fixed boundary condition
  - [ ] Moving boundary condition
  - [ ] Mesh on Mesh contact
  - [ ] Inversion free
  - [ ] Friction energy

- [ ] Fix sticky DBC Hessian transformation more (I think it's causing the fixed point to move around rn)

- [ ] Investigate the bug where the sim will freeze up and never return (I think it's prematurely reach a minimum somewhere and getting a search direction of zero): see *buggy_scene.json* 

- [ ] Download MATLAB & Simulink w/ the TAMU free student license and mess around with the sims to see how accurate this piece of shit is in comparison

- [ ] Improve Cube Construction:
  - Should only draw the minimum number of external triangles and not draw any internal supports (might make it hard to tell if structure inverted but idgaf rn)
  - Normals should be mapped directly outwards 
  - Texture coordinates should be set for triangles
  - Variable spring stiffness and point mass, setable via the input json

- [ ] Sparse Optimizations
  - Hessian is nearly always sparse so storing in CVR (or similar) format and using a sparse eigen solver 
  - Eigen has a sparse matrix object which I need to investigate

- [ ] Texture Map Shader

- [ ] Frame Interpolation:
  - Physics should run at a set rate different than renderer and the renderer should interpolate the positions AND normals
  - "we can render sometime between the most current physics step and the step before that, meaning our rendering is actually slightly behind our simluation. As stated anecdotally in Fixed-Time-Step Implementation, this is both imperceptible to the user as well as common practice on all major games" - https://kirbysayshi.com/2013/09/24/interpolated-physics-rendering.html

- [ ] Normal Movement:
  - The physics sims needs to update the normals somehow. Not quite sure how to go about this right now... 
  - Maybe it can go triangle by triangle and just assign the plane norm?
  - This should be done after cube improvement so that we only need normal calculations for the outer triangles

- [ ] BPhong Shading

- [ ] Float -> Double (physics only not opengl):
  - Because of interpolation we kind of have to copy everything anyway??
  - OpenGL doesn't like doubles so it draws in floats but I want double precision in physics sims
  - This means we can't use Eigen::Map but we'll have to see how much of an affect it has on performance

- [ ] Animation saving & replaying
  - Saving just outmost positions and normals to file 

- [ ] Investigate time-dependent forces (e.g. simulating viscoelasticity where the speed of the deformation plays a role in the force that is applied back)

# Notes:
- ALWAYS CHECK CMAKELIST FOR DEBUG/RELEASE
- Checkout OGC Paper from UofU
- Applying Google C++ Stylesheet




