
#include "distances.h"
#include <Eigen/Dense>
using Eigen::Vector3d;

double PPval(Vector3d& x1, Vector3d& x2) {
	Vector3d diff = x2-x1;
	return diff.dot(diff);
}
   




