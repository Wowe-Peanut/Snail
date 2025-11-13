#pragma once

#include "Object.h"
#include "Shape.h"

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Sparse>
#include <Eigen/Dense>


class PhysicsEngine {
    public:
        PhysicsEngine(std::vector<std::shared_ptr<Object>>& objectList, float deltatime, float tolerance, int maxIterations);
        void implicitStep();
        void symplecticStep();

        
    private:
        std::vector<std::shared_ptr<Object>> objects;
        float h;
        float tol;
        int maxiter;

        Eigen::Matrix3Xf getSearchDirection(Eigen::Matrix3Xf& xtilde, float h);
        
        // Helper
        float IPValue(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::Matrix3Xf IPGradient(Eigen::Matrix3Xf& xtilde, float h);
        Eigen::MatrixXf IPHessian(Eigen::Matrix3Xf& xtilde, float h);
        void makePSD(Eigen::MatrixXf& hess);

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
 * To prepare for contact forces a little further down the line, I need to be minimizing incremental potential energy 
 * OF THE ENTIRE SYSTEM (that is E(x) is now a function that takes in the positions of the points of ALL objects). I'm going to move
 * away from the eigen::map for now, since I'll be copying over if I want to do double precision physics sims and have to cast to float
 * for opengl to work. Instead I'll just have a normal Matrix3Xf that will need to be copied over (this will also be better for
 * saving animations to files and frame-interpolation). But that means I have to keep track of what indicies go to which object and copy them over
 * to the positions buffers each frame. HOWEVER, this will also make it easier to only draw the visible triangles.
 * 
 * lots to think about...
 * 
 * It might be nice for readability later on to put the energy functions in there own cpp folder but that will have to wait for a 
 * later refactoring (woohoo! technical debt!)
 * 
 * I'll also need to have the object physics engine set the initial conditions of the objects... or maybe have it setable from json.
 *  */        



};