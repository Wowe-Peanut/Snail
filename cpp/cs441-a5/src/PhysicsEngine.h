#pragma once

#include "Object.h"
#include "Shape.h"
#include "json.hpp"

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Sparse>
#include <Eigen/Dense>


class PhysicsEngine {
    public:
        PhysicsEngine(std::vector<std::shared_ptr<Object>>& objectList, nlohmann::json parameters);
        void reset();
        void implicitStep();
        void symplecticStep();

        
    private:

        // Simulation parameters
        float h;
        float tol;
        int maxiter;
        float springStiffness;
		float pointMass;
        Eigen::Vector3f gravity;

        std::vector<std::shared_ptr<Object>> physicsObjects; 
        std::vector<int> objectOffsets; // starting index of each physicsObject's point positions in 'positions' & 'velocities'

        // Combined properties of all objects
        int numPoints;
        Eigen::Matrix3Xf positions;
        Eigen::Matrix3Xf velocities;
        std::vector<bool> isFixedPoint;

        Eigen::Matrix3Xf initialPositions;
        Eigen::Matrix3Xf initialVelocities;
        
        int numEdges;
        std::vector<std::vector<unsigned int>> edgeList;
        std::vector<float> edgeRestLengthSquares;
        
        // Helper
        void makePSD(Eigen::MatrixXf& hess);
        void updateObjectPositions(); 
        Eigen::Matrix3Xf getSearchDirection(Eigen::Matrix3Xf& xtilde, float h);

        // Total energy
        float IPValue(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::Matrix3Xf IPGradient(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::MatrixXf IPHessian(Eigen::Matrix3Xf& xtilde, float h);

        // Inertia
        float InertiaValue(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::Matrix3Xf InertiaGradient(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::MatrixXf InertiaHessian(Eigen::Matrix3Xf& xtilde, float h);
        
        // Gravity
        float GravityValue(float h);
        Eigen::Matrix3Xf GravityGradient(float h);

        // Spring
        float MassSpringValue(float h);
        Eigen::Matrix3Xf MassSpringGradient(float h);
        Eigen::MatrixXf MassSpringHessian(float h);



/**
 * I also need to consider broad and narrow phase checking... with the implementation I currently have planned even objects that are not 
 * touching will be put into the same ensemble 'positions', 'velocities', etc... but that is only necessary for objects that are touching
 * (and therefore the Hessian of their combined system will have non-zero values in cross-object entries so they need to be together). However,
 * updating this for every broad and narrow phase seems super fucking expensive though it might allow for easier multithreading...
 * 
 * AHHH, but the main issue is the barrier energy calculations no? so if we use broad phase + bounding volume hierarchies to only 
 * turn on the collisions detection when objects are close... 
 * 
 * To prepare for contact forces a little further down the line, I need to be minimizing incremental potential energy 
 * OF THE ENTIRE SYSTEM (that is E(x) is now a function that takes in the positions of the points of ALL objects). I'm going to move
 * away from the eigen::map for now, since I'll be copying over if I want to do double precision physics sims and have to cast to float
 * for opengl to work. Instead I'll just have a normal Matrix3Xf that will need to be copied over (this will also be better for
 * saving animations to files and frame-interpolation). But that means I have to keep track of what indicies go to which object and copy them over
 * to the positions buffers each frame. HOWEVER, this will also make it easier to only draw the visible triangles.
 * 
 * lots to think about...
 * 
 * But I also need to consider how later on I will get it to only draw outer triangles (once I improve the cube construction). I suppose
 * 'object' should be somewhat ignorant to it's 'shape', only dealing with global transformations, holding physics properties, and sending
 * info to the shaders. So the 'shape' should be what contains the information on which triangles and draw and which are not, which makes
 * this refactoring a little easier since the construction already happens in 'shape.cpp'
 * 
 * The issue is that the current plan for 'PhysicsEngine' is to just copy over the relvant subarrays to their respective objects. Should
 * 'shape' care about having its internal positions be updated? Or should PhysicsEngine be the one to store it's updated values each step.
 * It doesn't really need to, since after construction shape is only concered with drawing stuff... so maybe when the PhysicsEngine is 
 * is initialized it goes and moves that info to itself so that the construction can still be kept in shape. But since after that point we
 * don't need 'shape' to have it, we just only send back positions that correlate with drawn triangles... not sure how to organize that tho.
 * if the indices are scattered in position, we can't do bulk copy, so perhaps in 'shape.cpp' functions that construct objects we make sure
 * to keep a separate array of "trianglePositions" and ensure that the triangles indices of the positions that PhysicsEngine reads from it
 * are AT THE START OF THE POSITION ARRAY, then we can just read the first chunk of each objects subarray in positions and copy that over. 
 * 
 * Maybe it would be easier if all the shapes were just a single buffer that seems like even more refactoring and it doesn't
 * allow me to apply different shaders to different objects nor apply separate transformations so maybe not.
 * 
 * It might be nice for readability later on to put the energy functions in there own cpp folder but that will have to wait for a 
 * later refactoring (woohoo! technical debt!)
 * 
 * I'll also need to have the object physics engine set the initial conditions of the objects... or maybe have it setable from json.
 *  */        



};