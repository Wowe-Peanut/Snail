# Snail - A Softbody Physics Simulator

A softbody physics simulator inspired by this [amazing article](https://phys-sim-book.github.io/preface.html) by Minchen Li et al.

<img width="250" height="250" alt="image13" src="https://github.com/user-attachments/assets/35f15538-40a7-4fad-9e99-5325a9cd7093" />
<img width="250" height="250" alt="image14" src="https://github.com/user-attachments/assets/21eab332-329b-42b0-a9cc-ff2a69311e55" />
<img width="250" height="250" alt="clipped14square" src="https://github.com/user-attachments/assets/5c581698-6e87-4877-a23e-50a4e8ce6821" />

### Features
- Physics
	- Gravity
	- 1D Spring Elasticity
 	- 3D Stable Neo-Hookean Elasticity
	- Mesh on Mesh Contact
	- Stationary Points
- Solvers
	- Implicit Euler Integration
	- Full Newton (except for neo-hookean energy)
 	- L-BFGS
- Optimizations
	- Sweep and prune broadphase
	- Sparse solvers
- GUI
	- Blinn-Phong shading
	- Controllable camera, pausing, resetting, stepping
- JSON to Scene Parser
	- See ```resources/scenes``` for examples of scene parameters

### Dependencies
- [GLM](https://github.com/g-truc/glm) under ```GLM_INCLUDE_DIR```
- [GLEW](https://glew.sourceforge.net/) under ```GLEW_DIR```
- [GLFW](https://www.glfw.org/) under ```GLFW_DIR```
- [Eigen](libeigen.gitlab.io) under ```EIGEN3_INCLUDE_DIR```
- CMake

### Build

```bash
// Navigate to snail/
mkdir build
cd build
cmake ..
make -j4
./snail PATH_TO_RESOURCES PATH_TO_SCENE_JSON
```

### Citations & Inspirations
<ul class="reference-list">
<li class="reference-item">
	Li, M., Ferguson, Z., Schneider, T., Langlois, T. R., Zorin, D., Panozzo, D., ... &amp; Jiang, C. (2020). 
	<strong>Incremental potential contact: intersection-and inversion-free, large-deformation dynamics</strong>. 
	<em>ACM Trans. Graph.</em>, 39(4), 49-1. 
	[<a href="https://ipc-sim.github.io/file/IPC-paper-fullRes.pdf" target="_blank">PDF</a>]
</li>
<li class="reference-item">
	Kim, T., &amp; Eberle, D. (2022). 
	<strong>Dynamic Deformables: Implementation and Production Practicalities</strong>. 
	<em>ACM SIGGRAPH 2022 Courses</em>. 
	[<a href="https://www.tkim.graphics/DYNAMIC_DEFORMABLES/DynamicDeformables.pdf" target="_blank">PDF</a>]
</li>
<li class="reference-item">
	Libraries Used: Eigen, GLEW, GLFW, GLM, nlohmann/json
</li>
</ul>

