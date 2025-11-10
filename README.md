# Currently working on:
- Planning new structure, making renderer and physics engine classes, cleaning shit up, reading jsons

# TODO

- Refactoring:
  - [] Make Renderer class separate from main that can be initialized later with the given JSON or replay saved animations
  - [] Separate rendering and physics engine. B/c of interpolation and float cast we already have to copy shit over so just aim to link
  - [] Move away from shared pointers
  - [] Matrixstack stuff still uses glm but it would be nice to switch everything over to eigen
  - [] Make storing the light information more clean

- QOL: 
  - [] Make initial conditions & simulation parameters setable from input file
  - [] Add reset animation button
  - [] Add single step button 
  - [] Zoom in and out with camera
  - [] Add light count, positions, and colors as JSON parameter

- IPC:
  - [] Fixed boundary condition
  - [] Moving boundary condition
  - [] Mesh on Mesh contact
  - [] Inversion free
  - [] Friction energy

- [] Investigate sticky DBC Hessian transformation more (whether it's actually necessary since I can't get it to work)

- [] Improve Cube Construction:
  - Should only draw the minimum number of external triangles and not draw any internal supports (might make it hard to tell if structure inverted but idgaf rn)
  - Normals should be mapped directly outwards 
  - Texture coordinates should be set for triangles
  - Variable spring stiffness and point mass

- [] Texture Map Shader

- [] Frame Interpolation:
  - Physics should run at a set rate different than renderer and the renderer should interpolate the positions AND normals

- [] Normal Movement:
  - The physics sims needs to update the normals somehow. Not quite sure how to go about this right now... 
  - Maybe it can go triangle by triangle and just assign the plane norm?
  - This should be done after cube improvement so that we only need normal calculations for the outer triangles

- [] BPhong Shading

- [] Float -> Double (physics only not opengl):
  - Because of interpolation we kind of have to copy everything anyway??
  - OpenGL doesn't like doubles so it draws in floats but I want double precision in physics sims
  - This means we can't use Eigen::Map but we'll have to see how much of an affect it has on performance

- [] Animation saving & replaying
  - Saving just outmost positions and normals to file 

- [] Sparse Optimizations
  - Hessian is nearly always sparse so storing in CVR (or similar) format and using a sparse eigen solver 
  - Eigen has a sparse matrix object which I need to investigate


# Notes:
- ALWAYS CHECK CMAKELIST FOR DEBUG/RELEASE
- Checkout OGC Paper from UofU
- Applying Google C++ Stylesheet




