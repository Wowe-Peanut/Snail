
#include "collision_manager.h"
#include "physics_engine.h"


void PointTriangle::update(bool valueOnly) {

}
double PointTriangle::CCD(Eigen::Matrix3Xd& searchDirection) {
	return 0.0;
}


void EdgeEdge::update(bool valueOnly) {

}
double EdgeEdge::CCD(Eigen::Matrix3Xd& searchDirection) {
	return 0.0;
}






double CollisionManager::CCD(Eigen::Matrix3Xd& searchDirection) {
	return 1;
}

void CollisionManager::broadPhase() {

}

void CollisionManager::updateActivePairs(bool valueOnly) {

}