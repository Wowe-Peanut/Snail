
### TODO

#### Refactoring A5
- [X] Remove deferred rendering
- [X] Remove freecam and replace with camera that rotates around center (see a3)
- [X] Remove all but one object (make it a single object in the center screen)

#### Mass Spring Symplectic
- [ ] Construct 3d box with triangles WITH INNER SUPPORTS. Ideally it should only send the outermost triangles to the GPU and leave the inner ones for internal forces only.
- [ ] Setup the main simulation loop WITH SYMPLECTIC EULER FIRST

#### Mass Spring Implicit
- [ ] Inertia Energy calculations
- [ ] Mass Spring Energy calculations
- [ ] Setup time integrator

---


### Other things to look into:
- Just remake the shape class so you understand it better lmao (you're getting stunlocked), it should use the eigen map directly to the buffer and SHOULD NOT contain any physics items.
- To keep the rendering & physics separate, keep it out of Shape.h and put it in Object.h, then probably create a World/Scene class which should handle broad and narrow phase shit
- Also stripping the Object.h class of unnecessary things would also be a nice for readability
- CHECK IF YOU NEED TO BIND THE PROGRAM EVERY TIME IN RENDER IN YOU'RE ONLY USING ONE SHADER, MAYBE JUST ONCE IN INIT AND ONCE AT END (if there is an end)

- From the CS450 Lab 6 guide: continuously editing and copying the positions of the vertecies to the shape buffers can be expensive. So look into Eigen::Map so that we we write directly into the opengl buffer when making changes to the mesh
- **Important**: the rendering side should be separable from the simulation side. I want to be able to run this on GRACE/FASTER later on and be able to save the positions each time step. But if it's too grappled with the OpenGL shit that could be difficult. However, I still want to have real-time rendering so it should be configurable to do that. Maybe abrstract the OpenGL stuff away into a 'Renderer' class in which we just pass the triangle states each iteration. Then it could also a 'replay' and 'save video' mode by passing a list of states.


- Normal Interpolation: how to smooth normals when the mesh is deformed
- Reading about alternative LA libraries if eigen doesn't have a good scipy.spsolve counterpart
- Utah Graphics Lab's new paper on "Offset Geometric Contact (OGC)", an IPC alternative that in many cases is faster, more stable, and handles penetration free collision much better.
