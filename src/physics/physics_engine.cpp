
#include "physics_engine.h"

#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Sparse>

using json = nlohmann::json;
using namespace std;
using Eigen::Vector3d, Eigen::Matrix3Xd, Eigen::VectorXd, Eigen::MatrixXd, Eigen::Matrix3d, Eigen::SparseMatrix, Eigen::Triplet;

PhysicsEngine::PhysicsEngine(vector<shared_ptr<Object>>& objectList, json parameters) {
    // Read parameters
    h                   = parameters["delta_time"];
    tol                 = parameters["tolerance"];
    springStiffness     = parameters["spring_stiffness"];
    pointMass           = parameters["point_mass"];
	contactStiffness	= parameters["contact_stiffness"];
	contactDistance 	= parameters["contact_distance"];
    gravity             = Vector3d(0, parameters["gravity"], 0);

    // A running total used to calculate object indices (3*numpoints)
    numPoints = 0;
    for (auto obj: objectList) {
		objects.push_back(obj);
		offsets.push_back(numPoints);
		numPoints += obj->mesh->numPoints;
    }

	
	// Initialize the ensemble state of all objects
	positions = Matrix3Xd::Zero(3, numPoints);
	velocities = Matrix3Xd::Zero(3, numPoints); 

	for (size_t objIdx=0; objIdx<objects.size(); objIdx++) {
		auto obj = objects[objIdx];
		int offset = offsets[objIdx];

		// Copy positions
		Eigen::Map<Eigen::Matrix3Xf> floatMatrix(obj->mesh->triPosBuf.data(), 3, obj->mesh->numPoints);
		positions.middleCols(offset, obj->mesh->numPoints) = floatMatrix.cast<double>();
		
		// Copy edges
		for (Edge& edge: obj->mesh->edges) {
			edges.push_back({offset+edge.v1, offset+edge.v2, edge.l2});
		}
		
		// Copy surface primitives
		for (Triangle& tri: obj->mesh->triangles) {
			surfaceTriangles.push_back({offset+tri.v1, offset+tri.v2, offset+tri.v3});
		}
		
		// Copy fixed points
		isFixedPoint.insert(isFixedPoint.end(), obj->mesh->isFixedPoint.begin(), obj->mesh->isFixedPoint.end());

		// Copy velocity
		Vector3d initialVelocity(obj->mesh->initialVelocity.x, obj->mesh->initialVelocity.y, obj->mesh->initialVelocity.z);
		for (int vidx=0; vidx < obj->mesh->numPoints; vidx++) {
			if (!obj->mesh->isFixedPoint[vidx]) {
				velocities.col(offset+vidx) = initialVelocity;
			}
		}

	}

	initialPositions = positions;
	initialVelocities = velocities;
	
}

// Step functions
void PhysicsEngine::implicitStep() {
	if (objects.empty()) return;	

	// Make copy of original positions & calculate explicit predicted positions
	Matrix3Xd originalPositions = positions;
	Matrix3Xd previousPositions = positions;
	Matrix3Xd predictedPositions = positions + h*velocities;

	// Calculate initial Incremental Potential value and search direction 
	Matrix3Xd searchDirection = getSearchDirection(predictedPositions);
	double IP = IPValue(predictedPositions);

	// Projected Newton Loop
	while (searchDirection.colwise().lpNorm<1>().maxCoeff() / h > tol)  {

		// Line search to guarantees a step size that reduces the systems energy
		double alpha = CCD(searchDirection);
		positions = previousPositions + alpha*searchDirection;

		double newIP = IPValue(predictedPositions);

		while (newIP > IP) {
			alpha /= 2;
			positions = previousPositions + alpha*searchDirection;
			newIP = IPValue(predictedPositions);

			if (alpha > 0.00001) break;
		}
		
		// Update IP & calculate next search direction
		IP = newIP;
		searchDirection = getSearchDirection(predictedPositions);
		previousPositions = positions;
	}

	// Update velocities with final positions
	velocities = (positions - originalPositions) / h;

	// Send new positions to each object and recompute all normals
    updateObjects();
}



// Helper
double PhysicsEngine::CCD(Matrix3Xd& searchDirection) {

	double alpha = 1;
	return alpha;

	// // For each physics objects
	// for (int oidx1=0; oidx1<objects.size(); oidx1++) {
	// 	shared_ptr<Object> obj1 = objects[oidx1];
	// 	int offset = offsets[oidx1];
		
	// 	// For each vertex (should eventually convert to just surface) calculate contact with
	// 	// using the SDF of all other objects (both physics & static)
	// 	for (int vidx=0; vidx<obj1->mesh->numPoints; vidx++) {
	// 		Vector3d p = positions.col(offset + vidx);
	// 		Vector3d partialSearchDir = searchDirection.col(offset + vidx);

	// 		//! FOR NOW ONLY STATIC MESHES
	// 		for (auto obj2: staticObjects) {		
	// 			alpha = min(alpha, obj2->mesh->sdf->ccd(p, partialSearchDir));
	// 		}
	// 	}
	// }

	// return alpha;
}

Matrix3Xd PhysicsEngine::getSearchDirection(Matrix3Xd& xtilde) {
	SparseMatrix<double> hess = IPHessian(xtilde);
	Matrix3Xd grad = IPGradient(xtilde);

	// Gradient sticky DBCs
	for (int vidx=0; vidx<numPoints; vidx++) {
		if (isFixedPoint[vidx]) {
			grad.col(vidx) = Vector3d(0, 0, 0);
		}
	}

	// Hess sticky DBCs
    for (int col=0; col<hess.outerSize(); col++) {
        for (SparseMatrix<double>::InnerIterator it(hess, col); it; ++it) {
			
			int row = it.row();
			if (isFixedPoint[(int) row / 3] || isFixedPoint[(int) col / 3]) {
				it.valueRef() = row == col ? 1 : 0;
			}
        }
    }
 
	// Sparse solver 
	Eigen::SparseLU<SparseMatrix<double>> solver;
	solver.compute(hess);

    if (solver.info() != Eigen::Success) {
        std::cerr << "Solver failed to compute decompose Hessian!\n";
        return Matrix3Xd::Zero(3, numPoints);
    }

	VectorXd p = solver.solve(-Eigen::Map<VectorXd>(grad.data(), 3*numPoints));
	return Eigen::Map<Matrix3Xd>(p.data(), 3, numPoints);
}
void PhysicsEngine::makePSD(MatrixXd& hess) {

	// Self-adjoint (A = A^T) matrix has real eigenvalues and orthogonal eigenvectors and our local hess
	// is a block of (H, -H; -H, H) which is self-adjoint so we can use the SelfAdjointEigenSolver)
	Eigen::SelfAdjointEigenSolver<MatrixXd> es(hess);
	VectorXd evals = es.eigenvalues();
	MatrixXd evecs = es.eigenvectors();

	// Zero out negative eigenvalues to make PSD
	for (int i=0; i<evals.size(); i++) {
		if (evals(i) < 0) evals(i) = 0;
	}

	// Reconstruct matrix with new eigenvalues
	hess = evecs * evals.asDiagonal() * evecs.transpose();
}
void PhysicsEngine::updateObjects() {
	for (size_t objIdx=0; objIdx<objects.size(); objIdx++) {
		auto obj = objects[objIdx];

		if (!obj->mesh->isStatic) {
			
			int offset = offsets[objIdx];
	
			Eigen::Matrix3Xf submatrix = positions.middleCols(offset, obj->mesh->numPoints).cast<float>();
			obj->mesh->triPosBuf.assign(submatrix.data(), submatrix.data() + submatrix.size());
			obj->mesh->computeSurfaceQualities();
		}
	}
}
void PhysicsEngine::reset() {
	positions = initialPositions;
	velocities = initialVelocities;

	updateObjects();
}



// Incremental Potential Energy
double PhysicsEngine::IPValue(Matrix3Xd& xtilde) {
	return InertiaValue(xtilde) + h*h*(MassSpringValue() + GravityValue());
}
Matrix3Xd PhysicsEngine::IPGradient(Matrix3Xd& xtilde) {
	return InertiaGradient(xtilde) + h*h*(MassSpringGradient() + GravityGradient());
}
SparseMatrix<double> PhysicsEngine::IPHessian(Matrix3Xd& xtilde) {
	return InertiaHessian(xtilde) + h*h*(MassSpringHessian());
}



// Inertia Energy 
double PhysicsEngine::InertiaValue(Matrix3Xd& xtilde) {
	double sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		Vector3d diff = positions.col(vidx) - xtilde.col(vidx);
		sum += diff.dot(diff);
	}

	return pointMass * sum / 2;
}
Matrix3Xd PhysicsEngine::InertiaGradient(Matrix3Xd& xtilde) {
	return pointMass * (positions - xtilde);
}
SparseMatrix<double> PhysicsEngine::InertiaHessian(Matrix3Xd& xtilde) {

	// From eigen docs: "The cost of a single purely random insertion into a SparseMatrix is O(nnz), 
	// where nnz is the current number of non-zero coefficients."
	// So it recommends using triplets, which constructs the SparseMatrix in O(n) with n the number of triplets

	int dof = 3*numPoints;
	vector<Triplet<double>> triplets(dof);
	for (int i=0; i<dof; i++) {
		triplets[i] = Triplet<double>(i, i, pointMass);
	}

	SparseMatrix<double> hess(dof, dof);
	hess.setFromSortedTriplets(triplets.begin(), triplets.end());

	return hess;
}



// Mass Spring Energy 
double PhysicsEngine::MassSpringValue() {
	double sum = 0;
	for (Edge& edge: edges) {
		Vector3d diff = positions.col(edge.v1) - positions.col(edge.v2);
		sum += edge.l2 * pow(diff.dot(diff) / edge.l2 - 1, 2);
	}
	return sum * springStiffness / 2;
}
Matrix3Xd PhysicsEngine::MassSpringGradient() {
	Matrix3Xd grad = MatrixXd::Zero(3, numPoints);

	for (Edge& edge: edges) {
		Vector3d diff = positions.col(edge.v1) - positions.col(edge.v2);
		Vector3d edgeGrad = 2 * springStiffness * (diff.dot(diff) / edge.l2 - 1) * diff;

		grad.col(edge.v1) += edgeGrad;
		grad.col(edge.v2) -= edgeGrad;
	}

	return grad;
}
SparseMatrix<double> PhysicsEngine::MassSpringHessian() {

	int dof = 3*numPoints;
	vector<Triplet<double>> triplets;
	triplets.reserve(9*edges.size()); // 2 vertices per edge, each with 3 dofs = 3^2 = 9 second derivatives

	for (Edge& edge: edges) {
		Vector3d diff = positions.col(edge.v1) - positions.col(edge.v2);

		// Hessian for the energy of single edge, 3x3 for each DIFFERENCE in the two vertices
		Matrix3d diffHess = 2 * springStiffness / edge.l2 * (2 * diff * diff.transpose() + (diff.dot(diff) - edge.l2) * Matrix3d::Identity());

		// Essemble 6x6 hessian for the 6 DOFs on the two vertices of the edge. diffHess is symmetric, so 
		// this block matrix will also be symmetric so we can use a SelfAdjointEigenSolver to make PSD
		MatrixXd localHess(6, 6);
		localHess.block<3,3>(0,0) = diffHess;
		localHess.block<3,3>(0,3) = -diffHess;
		localHess.block<3,3>(3,0) = -diffHess;
		localHess.block<3,3>(3,3) = diffHess;
		makePSD(localHess);


		for (int blockRow=0; blockRow<=1; blockRow++) {
			for (int blockCol=0; blockCol<=1; blockCol++) {

				int startRow = (blockRow == 0 ? 3*edge.v1 : 3*edge.v2);
				int startCol = (blockCol == 0 ? 3*edge.v1 : 3*edge.v2);

				for (int row=0; row<3; row++) {
					for (int col=0; col<3 ;col++) {
						double value = localHess(3*blockRow+row, 3*blockCol+col);

						triplets.push_back(Triplet<double>(startRow+row, startCol+col, value));
					}
				}	
			}
		}
	}

	SparseMatrix<double> hess = SparseMatrix<double>(3*numPoints, 3*numPoints);
	hess.setFromTriplets(triplets.begin(), triplets.end());
	return hess;
}



// Gravity Energy 
double PhysicsEngine::GravityValue() {
	double sum = 0;
	for (int vidx=0; vidx<numPoints; vidx++) {
		sum += gravity.dot(positions.col(vidx));
	}

	return -sum * pointMass;
}
Matrix3Xd PhysicsEngine::GravityGradient() {
	Matrix3Xd grad = Matrix3Xd::Zero(3, numPoints);
	for (int vidx=0; vidx<numPoints; vidx++) {
		grad.col(vidx) = -pointMass * gravity;
	}

	return grad;
}



// // Contact Energy
// double PhysicsEngine::ContactValue() {
// 	double sum = 0;

// 	// For each physics objects
// 	for (int oidx1=0; oidx1<objects.size(); oidx1++) {
// 		shared_ptr<Object> obj1 = objects[oidx1];
// 		int offset = offsets[oidx1];
		
// 		// For each vertex (should eventually convert to just surface) calculate contact with
// 		// using the SDF of all other objects (both physics & static)
// 		for (int vidx=0; vidx<obj1->mesh->numPoints; vidx++) {
// 			Vector3d p = positions.col(offset + vidx);

// 			//! FOR NOW ONLY STATIC MESHES
// 			for (auto obj2: staticObjects) {

// 				double d = obj2->mesh->sdf->distance(p);
// 				if (d < contactDistance) {
// 					sum += obj1->mesh->vertexAreas[vidx] * contactDistance * (contactStiffness/2 * (d/contactDistance - 1) * log(d/contactDistance));
// 				}
// 			}
// 		}
// 	}

// 	return sum;
// }

// Matrix3Xd PhysicsEngine::ContactGradient() {
// 	Matrix3Xd grad = Matrix3Xd::Zero(3, numPoints);

// 	// For each physics objects
// 	for (int oidx1=0; oidx1<objects.size(); oidx1++) {
// 		shared_ptr<Object> obj1 = objects[oidx1];
// 		int offset = offsets[oidx1];
		
// 		// For each vertex (should eventually convert to just surface) calculate contact with
// 		// using the SDF of all other objects (both physics & static)
// 		for (int vidx=0; vidx<obj1->mesh->numPoints; vidx++) {
// 			Vector3d p = positions.col(offset + vidx);

// 			//! FOR NOW ONLY STATIC MESHES
// 			for (auto obj2: staticObjects) {

// 				double d = obj2->mesh->sdf->distance(p);
// 				Vector3d dgrad = obj2->mesh->sdf->distanceGrad(p);
				

// 				if (d < contactDistance) {
// 					grad.col(offset+vidx) = obj1->mesh->vertexAreas[vidx] * contactDistance * (contactStiffness/(2*contactDistance) * log(d/contactDistance) + 1/d) * dgrad;
// 				}
// 			}
// 		}
// 	}

// 	return grad;
// }

// SparseMatrix<double> PhysicsEngine::ContactHessian() {

// 	vector<Triplet<double>> triplets;

// 	// For each physics objects
// 	for (int oidx1=0; oidx1<objects.size(); oidx1++) {
// 		shared_ptr<Object> obj1 = objects[oidx1];
// 		int offset = offsets[oidx1];
		
// 		// For each vertex (should eventually convert to just surface) calculate contact with
// 		// using the SDF of all other objects (both physics & static)
// 		for (int vidx=0; vidx<obj1->mesh->numPoints; vidx++) {
// 			Vector3d p = positions.col(offset + vidx);

// 			//! FOR NOW ONLY STATIC MESHES
// 			for (auto obj2: staticObjects) {

// 				double d = obj2->mesh->sdf->distance(p);
// 				Vector3d dgrad = obj2->mesh->sdf->distanceGrad(p);
// 				Matrix3d dhess = obj2->mesh->sdf->distanceHess(p);

// 				if (d < contactDistance) {
// 					double contactWeight = obj1->mesh->vertexAreas[vidx] * contactDistance;
// 					Matrix3d term1 = contactStiffness/(2*contactDistance*d) * (dgrad * dgrad.transpose());
// 					Matrix3d term2 = (contactStiffness/(2*contactDistance)*log(d/contactDistance) + 1/d) * dhess;
// 					Matrix3d localHess =  contactWeight * (term1 + term2);

// 					for (int row=0; row<3; row++) {
// 						for (int col=0; col<3 ;col++) {
// 							triplets.push_back(Triplet<double>(3*(offset+vidx)+row, 3*(offset+vidx)+col, localHess(row, col)));
// 						}
// 					}	
// 				}
// 			}
// 		}
// 	}

// 	SparseMatrix<double> hess = SparseMatrix<double>(3*numPoints, 3*numPoints);
// 	hess.setFromTriplets(triplets.begin(), triplets.end());
// 	return hess;
// }
