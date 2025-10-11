#### Notes:
- Note: cmakelist currently set to debug mode (see line #2)
- Currently working on constructing the cube & getting it to only draw the surface triangles
- Currently ALL positions are still being sent to the GPU, but I'm trying to get indBuf to only contain surface triangles atm
- Bphong replaced with distance shader until I figure out normals
- For sims, maybe cube normals could be fixed by just duplicating the triangles in a base cube (copy past and only keep external ones). Then maybe maybe start separating the physics objects and the OpenGL ones so that we can more easily separate internal from external points
- Having the eigen map directly to the opengl float buffer means that I can't use vector3d AND have the direct mapping... (this is something
I'll have to look into later if I want the physics simulations to have double precision) https://gemini.google.com/app/5623b295d50fb673
It seems you can have eigen do efficient casting and then do a block transfer (not copying one at a time). Might be slower but a good idea.

### TODO

#### Refactoring A5
- [X] Remove deferred rendering
- [X] Remove freecam and replace with camera that rotates around center (see a3)
- [X] Remove all but one object (make it a single object in the center screen)

#### Mass Spring Symplectic
- [X] Construct 3d box with triangles WITH INNER SUPPORTS. Ideally it should only send the outermost triangles to the GPU and leave the inner ones for internal forces only.
- [X] Setup the main simulation loop WITH SYMPLECTIC EULER FIRST
- [X] Convert to using Eigen<float, 3, n_points> instead of a std::vector

#### Mass Spring Implicit
- [ ] Convert to using sparse matrices for the Hessians
- [ ] Inertia Energy calculations
- [ ] Mass Spring Energy calculations
- [ ] Setup time integrator class
- [ ] Add variable point masses & variable spring stiffness

---


### Other things to look into:

- For shape maybe try having a separate buffer (one that we don't send to the GPU) for internal positions. Then the Object.h vector
of eigen maps can remain a single list but they can be mapped to either internal_position_buf or external_position_buf depending on
whether or not they are on the edge. In Shape::construct_cube, I already plan on not constructing internal triangle, just adding to the edge
list so it shouldn't be that hard... 

- **Important**: the rendering side should be separable from the simulation side. I want to be able to run this on GRACE/FASTER later on and be able to save the positions each time step. But if it's too grappled with the OpenGL shit that could be difficult. However, I still want to have real-time rendering so it should be configurable to do that. Maybe abrstract the OpenGL stuff away into a 'Renderer' class in which we just pass the triangle states each iteration. Then it could also a 'replay' and 'save video' mode by passing a list of states.
- Normal Interpolation: how to smooth normals when the mesh is deformed
- Reading about alternative LA libraries if eigen doesn't have a good scipy.spsolve counterpart
- Utah Graphics Lab's new paper on "Offset Geometric Contact (OGC)", an IPC alternative that in many cases is faster, more stable, and handles penetration free collision much better.
