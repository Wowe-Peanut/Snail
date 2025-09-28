
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
- Normal Interpolation: how to smooth normals when the mesh is deformed
- Reading about alternative LA libraries if eigen doesn't have a good scipy.spsolve counterpart
- Utah Graphics Lab's new paper on "Offset Geometric Contact (OGC)", an IPC alternative that in many cases is faster, more stable, and handles penetration free collision much better.
