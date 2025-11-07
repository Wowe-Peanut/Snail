#pragma once



class PhysicsEngine {
    public:

        enum class IntegrationMethod {
			EXPLICIT,
            SYMPLECTIC,
            IMPLICIT,
		};

        PhysicsEngine(IntegrationMethod method);

        



};