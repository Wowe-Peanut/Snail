
#include "sdf.h"

#include <Eigen/Dense>

using Eigen::Vector3d, Eigen::Matrix3d;
using namespace std;

PlaneSDF::PlaneSDF(Vector3d normal, Vector3d point): planeNormal(normal), planePoint(point) {}

double PlaneSDF::distance(Vector3d& point) {
	return planeNormal.dot(point - planePoint);
} 

Vector3d PlaneSDF::distanceGrad(Vector3d& point) {
	return planeNormal;
} 

Matrix3d PlaneSDF::distanceHess(Vector3d& point) {
	return Matrix3d::Zero();
}

double PlaneSDF::ccd(Eigen::Vector3d& point, Eigen::Vector3d& partialSearchDir) {

	double searchProj = planeNormal.dot(partialSearchDir);

	if (searchProj < 0) {
		return 0.9 * planeNormal.dot(point - planePoint) / -searchProj;
	} else {
		return 1;
	}
}