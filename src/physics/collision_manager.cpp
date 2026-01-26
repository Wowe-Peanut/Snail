
#include "collision_manager.h"
#include "physics_engine.h"
#include "distances.h"

using namespace std;
using Eigen::Matrix3Xd, Eigen::Vector3d;

const double MINIMUM_SEPARATION = 0.1;

void PointTriangle::update(SimState& state, bool valueOnly) {
	Matrix3Xd& pos = state.positions;
	
	dist = PointTriangleDist(pos.col(p), pos.col(t1), pos.col(t2), pos.col(t3), valueOnly);

	// Currently contact area only considers area of triangle, it doesn't take into account mesh density around point
	Vector3d u = pos.col(t2) - pos.col(t1);
	Vector3d v = pos.col(t3) - pos.col(t1);
	contactArea = 0.5 * u.cross(v).norm();
}

double PointTriangle::CCD(SimState& state, Matrix3Xd& searchDirection) {
	Matrix3Xd& pos = state.positions;

	Vector3d pvec = pos.col(p);
	Vector3d t1vec = pos.col(t1);
	Vector3d t2vec = pos.col(t2);
	Vector3d t3vec = pos.col(t3);

	Vector3d dp = searchDirection.col(p);
	Vector3d dt1 = searchDirection.col(t1);
	Vector3d dt2 = searchDirection.col(t2);
	Vector3d dt3 = searchDirection.col(t3);

	double maxDisplacementMag = dp.norm() + max(dt1.norm(), max(dt2.norm(), dt3.norm()));
	if (maxDisplacementMag < 1e-8) return 1; // Pretty much not moving at all, fine w/ any step size

	double curDist = sqrt(PointTriangleDist(pvec, t1vec, t2vec, t3vec, true).value);
	double minimumGap = curDist * MINIMUM_SEPARATION;

	// Keeps adding lowerbound of non-tunneling alpha values until it reaches MINIMUM_SEPARATION % of original distance (e.g. 0.1 of original)
	double curAlpha = 0;

	return (1 - MINIMUM_SEPARATION) * curDist / maxDisplacementMag;

	// while(true) {
	// 	double alphaLowerBound = (1 - MINIMUM_SEPARATION) * curDist / maxDisplacementMag;

	// 	pvec += dp * alphaLowerBound;
	// 	t1vec += dt1 * alphaLowerBound;
	// 	t2vec += dt2 * alphaLowerBound;
	// 	t3vec += dt3 * alphaLowerBound;
		
	// 	curDist = sqrt(PointTriangleDist(pvec, t1vec, t2vec, t3vec, true).value);
	// 	if (curDist < minimumGap) {
	// 		return curAlpha;
	// 	}

	// 	curAlpha += alphaLowerBound;
	// 	if (curAlpha > 1) {
	// 		return 1;
	// 	}
	// }
}

void EdgeEdge::update(SimState& state, bool valueOnly) {
	Eigen::Matrix3Xd& pos = state.positions;
	
	dist = EdgeEdgeDist(pos.col(e1), pos.col(e2), pos.col(e3), pos.col(e4), valueOnly);

	// Contact area is average lengths of the two incident edges
	Eigen::Vector3d u = pos.col(e2) - pos.col(e1);
	Eigen::Vector3d v = pos.col(e4) - pos.col(e3);
	contactArea = 0.5 * (u.norm() + v.norm());
}

double EdgeEdge::CCD(SimState& state, Matrix3Xd& searchDirection) {
	Matrix3Xd& pos = state.positions;

	Vector3d e1vec = pos.col(e1);
	Vector3d e2vec = pos.col(e2);
	Vector3d e3vec = pos.col(e3);
	Vector3d e4vec = pos.col(e4);

	Vector3d de1 = searchDirection.col(e1);
	Vector3d de2 = searchDirection.col(e2);
	Vector3d de3 = searchDirection.col(e3);
	Vector3d de4 = searchDirection.col(e4);

	double maxDisplacementMag = max(de1.norm(), de2.norm()) + max(de3.norm(), de4.norm());
	if (maxDisplacementMag < 1e-8) return 1; // Pretty much not moving at all, fine w/ any step size

	double curDist = sqrt(EdgeEdgeDist(e1vec, e2vec, e3vec, e4vec, true).value);
	double minimumGap = curDist * MINIMUM_SEPARATION;

	// Keeps adding lowerbound of non-tunneling alpha values until it reaches MINIMUM_SEPARATION % of original distance (e.g. 0.1 of original)
	double curAlpha = 0;
	

	return (1 - MINIMUM_SEPARATION) * curDist / maxDisplacementMag;
	// while(true) {
	// 	double alphaLowerBound = (1 - MINIMUM_SEPARATION) * curDist / maxDisplacementMag;

	// 	e1vec += de1 * alphaLowerBound;
	// 	e2vec += de2 * alphaLowerBound;
	// 	e3vec += de3 * alphaLowerBound;
	// 	e4vec += de4 * alphaLowerBound;
		
	// 	curDist = sqrt(EdgeEdgeDist(e1vec, e2vec, e3vec, e4vec, true).value);
	// 	if (curDist < minimumGap) {
	// 		return curAlpha;
	// 	}

	// 	curAlpha += alphaLowerBound;
	// 	if (curAlpha > 1) {
	// 		return 1;
	// 	}
	// }
}

double CollisionManager::CCD(Matrix3Xd& searchDirection) {
	double alpha = 1;
	for (auto& cp: state.activeCollisionPairs) {
		alpha = min(alpha, cp->CCD(state, searchDirection));
	}

	return alpha;
}

void CollisionManager::broadPhase() {

	double cd2 = params.contactDistance*params.contactDistance;
	state.activeCollisionPairs.clear();

	// Currently just brute forces all pairs of primitives
	for (Triangle& tri1: state.triangles) {
		for (Triangle& tri2: state.triangles) {
			if (&tri1 >= &tri2) continue; // No self collisions and avoid duplicates (tri1 & tri2 vs. tri2 and tri1)
			
			vector<int> tri1Vertices = {tri1.v1, tri1.v2, tri1.v3};
			vector<int> tri2Vertices = {tri2.v1, tri2.v2, tri2.v3};


			// Point-Triangle pairs (point cannot be a part of the triangle) -----------------------------------------

			// Point is on triangle 1
			for (int idx: tri1Vertices) {

				// If point is not in triangle 2, test for distance
				if (idx != tri2.v1 && idx != tri2.v2 && idx != tri2.v3) {
					shared_ptr<PointTriangle> candidate = make_shared<PointTriangle>(idx, tri2.v1, tri2.v2, tri2.v3);
					candidate->update(state, true);
					if (candidate->dist.value <= cd2) state.activeCollisionPairs.push_back(candidate);
				}
			}

			// Point is on triangle 2
			for (int idx: tri2Vertices) {

				// If point is not in triangle 1, test for distance
				if (idx != tri1.v1 && idx != tri1.v2 && idx != tri1.v3) {
					shared_ptr<PointTriangle> candidate = make_shared<PointTriangle>(idx, tri1.v1, tri1.v2, tri1.v3);
					candidate->update(state, true);
					if (candidate->dist.value <= cd2) state.activeCollisionPairs.push_back(candidate);
				}
			}



			// Edge-Edge pairs (edges cannot share any points) --------------------------------------------------------

			//Edge-Edge pairs (edges cannot share any points)
			// vector<pair<int,int>> edges1 = {{0,1}, {1,2}, {2,0}};
			// vector<pair<int,int>> edges2 = {{0,1}, {1,2}, {2,0}};

			// for (auto& [e1i, e1j] : edges1) {
			// 	for (auto& [e2i, e2j] : edges2) {
			// 		int e1v1 = tri1Vertices[e1i];
			// 		int e1v2 = tri1Vertices[e1j];
			// 		int e2v1 = tri2Vertices[e2i];
			// 		int e2v2 = tri2Vertices[e2j];
					
			// 		// Skip if edges share any vertex
			// 		if (e1v1 == e2v1 || e1v1 == e2v2 || e1v2 == e2v1 || e1v2 == e2v2) {
			// 			continue;
			// 		}
					
			// 		shared_ptr<EdgeEdge> candidate = make_shared<EdgeEdge>(e1v1, e1v2, e2v1, e2v2);
			// 		candidate->update(state, true);
			// 		if (candidate->dist.value <= cd2) state.activeCollisionPairs.push_back(candidate);
			// 	}
			// }
		}
	}
}

void CollisionManager::updateActivePairs(bool valueOnly) {
	for (auto& cp: state.activeCollisionPairs) {
		cp->update(state, valueOnly);
	}
}