
### TODO

#### Refactoring A5
- [ ] Remove deferred rendering
- [ ] Remove freecam and replace with camera that rotates around center (see a3)
- [ ] Remove all but one object (make it a single object in the center screen)

#### Mass Spring Symplectic
- [ ] Construct 3d box with triangles WITH INNER SUPPORTS. Ideally it should only send the outermost triangles to the GPU and leave the inner ones for internal forces only.
- [ ] Setup the main simulation loop WITH SYMPLECTIC EULER FIRST

#### Mass Spring Implicit
- [ ] Inertia Energy calculations
- [ ] Mass Spring Energy calculations
- [ ] Setup time integrator

---


### Other things to look into:
- **Important**: the rendering side should be separable from the simulation side. I want to be able to run this on GRACE/FASTER later on and be able to save the positions each time step. But if it's too grappled with the OpenGL shit that could be difficult. However, I still want to have real-time rendering so it should be configurable to do that. Maybe abrstract the OpenGL stuff away into a 'Renderer' class in which we just pass the triangle states each iteration. Then it could also a 'replay' and 'save video' mode by passing a list of states.
- Normal Interpolation: how to smooth normals when the mesh is deformed
- Reading about alternative LA libraries if eigen doesn't have a good scipy.spsolve counterpart
- Utah Graphics Lab's new paper on "Offset Geometric Contact (OGC)", an IPC alternative that in many cases is faster, more stable, and handles penetration free collision much better.
