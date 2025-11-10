#pragma once

#include "Object.h"
#include "Shape.h"

class PhysicsEngine {
    public:
        PhysicsEngine(vector<Object>& objectList, double deltatime, double tolerance, int maxIterations);
        ~PhysicsEngine();

        
    private:
        std::vector<Object> objects;
        double h;
        double tol;
        double maxiter;
        



};