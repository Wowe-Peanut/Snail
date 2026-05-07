
#include "collision_manager.h"
#include "physics_engine.h"
#include "distances.h"
#include "iostream"

using namespace std;
using Eigen::Matrix3Xd, Eigen::Vector3d;

void PointTriangle::update(SimState& state, int flags) {
	Matrix3Xd& pos = state.positions;
	
	dist = PointTriangleDist(pos.col(p), pos.col(t1), pos.col(t2), pos.col(t3), flags);

	// Currently contact area only considers area of triangle, it doesn't take into account mesh density around point
	// Vector3d u = pos.col(t2) - pos.col(t1);
	// Vector3d v = pos.col(t3) - pos.col(t1);
	contactArea = 1; // 0.5 * u.cross(v).norm();
}

double PointTriangle::CCD(SimParameters& params, SimState& state, Matrix3Xd& searchDirection) {
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
	if (maxDisplacementMag < 1e-5) return 1; // Pretty much not moving at all, fine w/ any step size

	double curDist = sqrt(PointTriangleDist(pvec, t1vec, t2vec, t3vec, true).value);
	double minimumGap = curDist * params.accdMinimumSeparation;

	return (1 - params.accdMinimumSeparation) * curDist / maxDisplacementMag;

	// // Keeps adding lowerbound of non-tunneling alpha values until it reaches params.accdMinimumSeparation % of original distance (e.g. 0.1 of original)
	// double curAlpha = 0;
	// while(true) {
	// 	double alphaLowerBound = (1 - params.accdMinimumSeparation) * curDist / maxDisplacementMag;

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

void EdgeEdge::update(SimState& state, int flags) {
	Eigen::Matrix3Xd& pos = state.positions;
	
	dist = EdgeEdgeDist(pos.col(e1), pos.col(e2), pos.col(e3), pos.col(e4), flags);

	// Contact area is average lengths of the two incident edges
	Eigen::Vector3d u = pos.col(e2) - pos.col(e1);
	Eigen::Vector3d v = pos.col(e4) - pos.col(e3);
	contactArea = 0.5 * (u.norm() + v.norm());
}

double EdgeEdge::CCD(SimParameters& params, SimState& state, Matrix3Xd& searchDirection) {
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
	double minimumGap = curDist * params.accdMinimumSeparation;

	// Keeps adding lowerbound of non-tunneling alpha values until it reaches params.accdMinimumSeparation % of original distance (e.g. 0.1 of original)
	// double curAlpha = 0;
	

	return (1 - params.accdMinimumSeparation) * curDist / maxDisplacementMag;
	
	// while(true) {
	// 	double alphaLowerBound = (1 - params.accdMinimumSeparation) * curDist / maxDisplacementMag;

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
		alpha = min(alpha, cp->CCD(params, state, searchDirection));
	}

	return alpha;
}

void CollisionManager::trianglesToCollisionPairs(const Triangle& tri1, const Triangle& tri2) {

	vector<vector<int>> vertices = {{tri1.v1, tri1.v2, tri1.v3}, {tri2.v1, tri2.v2, tri2.v3}};
	
	// Point-Triangle (triangle from primaryTri, point from secondaryTri)
	for (int primaryTri=0; primaryTri<=1; primaryTri++) {
		int secondaryTri = primaryTri == 0 ? 1 : 0;

		vector<int>& pvs = vertices[primaryTri];
		vector<int>& svs = vertices[secondaryTri];

		// Consider all points in secondary triangle that are not a part of the primary triangle
		for (int point: svs) {
			if (find(pvs.begin(), pvs.end(), point) == pvs.end()) {

				shared_ptr<PointTriangle> candidate = make_shared<PointTriangle>(point, pvs[0], pvs[1], pvs[2]);				
				candidate->update(state, true);

				if (candidate->dist.value < params.cd2) {
					state.activeCollisionPairs.push_back(candidate);
				}
			}
		}
	}

	// Edge-Edge
	// vector<pair<int,int>> eidxs = {{0,1}, {1,2}, {2,0}};
	// for (auto& [e1i, e1j] : eidxs) {
	// 	for (auto& [e2i, e2j] : eidxs) {
	// 		int e1v1 = vertices[0][e1i];
	// 		int e1v2 = vertices[0][e1j];
	// 		int e2v1 = vertices[1][e2i];
	// 		int e2v2 = vertices[1][e2j];
			
	// 		if (e1v1 == e2v1 || e1v1 == e2v2 || e1v2 == e2v1 || e1v2 == e2v2) {
	// 			continue;
	// 		}
			
	// 		shared_ptr<EdgeEdge> candidate = make_shared<EdgeEdge>(e1v1, e1v2, e2v1, e2v2);
	// 		candidate->update(state, true);
	// 		if (candidate->dist.value <= params.cd2) state.activeCollisionPairs.push_back(candidate);
	// 	}
	// }

	// std::cout << "Total Collision Pairs = " << state.activeCollisionPairs.size() << std::endl; 
}

void CollisionManager::broadPhase() {

	const Matrix3Xd& positions = state.positions;
	const Vector3d padding(params.contactDistance, params.contactDistance, params.contactDistance);

	// Update triangle bounding boxes
	for (Triangle& tri: state.triangles) {
		tri.boundingBox = AABB(positions.col(tri.v1), positions.col(tri.v2), positions.col(tri.v3));

		// Pad with contact distance squared
		tri.boundingBox.mins -= padding;
		tri.boundingBox.maxs += padding;
	}

	// Sort by starting point along x-axis
	sort(state.triangles.begin(), state.triangles.end(), TriangleBBComparatorX());

	// Sweep and prune along x-axis
	state.activeCollisionPairs.clear();


	int numTriangles = (int) state.triangles.size();
	for (int tidx1=0; tidx1<numTriangles; tidx1++) {
		Triangle& t1 = state.triangles[tidx1];


		for (int tidx2=tidx1+1; tidx2<numTriangles; tidx2++) {
			Triangle& t2 = state.triangles[tidx2];

			// Sweep and prune early quit
			if (t2.boundingBox.mins.x() > t1.boundingBox.maxs.x()) break;

			// // Disable self collision
			// if (state.owners[t1.v1] == state.owners[t2.v1]) break;

			if (t1.boundingBox.overlaps(t2.boundingBox)) trianglesToCollisionPairs(t1, t2);
		}
	}
}

void CollisionManager::updateActivePairs(int flags) {
	for (auto& cp: state.activeCollisionPairs) {
		cp->update(state, flags);
	}
}