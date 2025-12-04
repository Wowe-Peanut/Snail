
#include "sdf.h"

#include <Eigen/Dense>

using Eigen::Vector3f, Eigen::Matrix3f;
using namespace std;

float PlaneSDF::distance(Vector3f& point) {
	return planeNormal.dot(point - planePoint);
} 

Vector3f PlaneSDF::distanceGrad(Vector3f& point) {
	return planeNormal;
} 

Matrix3f PlaneSDF::distanceHess(Vector3f& point) {
	return Matrix3f::Zero();
}