
#include "sdf.h"

#include <Eigen/Dense>

using Eigen::Vector3f, Eigen::Matrix3f;
using namespace std;

PlaneSDF::PlaneSDF(Vector3f normal, Vector3f point): planeNormal(normal), planePoint(point) {}

float PlaneSDF::distance(Vector3f& point) {
	return planeNormal.dot(point - planePoint);
} 

Vector3f PlaneSDF::distanceGrad(Vector3f& point) {
	return planeNormal;
} 

Matrix3f PlaneSDF::distanceHess(Vector3f& point) {
	return Matrix3f::Zero();
}