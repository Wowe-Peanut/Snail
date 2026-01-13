
#include "distances.h"
#include <Eigen/Dense>
using Eigen::Vector3d, Eigen::Matrix3Xd, Eigen::MatrixXd, Eigen::Matrix3d;


Matrix3d asSkewSymmetric(Vector3d& v) {
	Matrix3d ssmat;
	ssmat << 0, -v[2], v[1], v[2], 0, -v[0], -v[1], v[0], 0;

	return ssmat;
}

Distance PointPointDist(Vector3d& x1, Vector3d& x2, bool valueOnly) {
	Distance dist;

	Vector3d diff = x1-x2;
	Matrix3d I2 = 2*Matrix3d::Identity(3, 3);

	dist.value = diff.dot(diff);

	if (!valueOnly) {
		dist.grad = Matrix3Xd(3, 2);
		dist.grad << 2*diff, -2*diff;
		dist.hess = MatrixXd(6, 6);
		dist.hess << I2, -I2, -I2, I2;
	}

	return dist;
}

Distance PointLineDist(Vector3d& x, Vector3d& l1, Vector3d& l2, bool valueOnly) {
	Distance dist;

	Vector3d l = l2-l1;
	Vector3d a = x-l1;
	
	double ll = l.dot(l);
	double t = a.dot(l)/ll;

	Vector3d p = a-t*l;

	dist.value = p.dot(p);
	if (!valueOnly) {
		dist.grad = Matrix3Xd(3, 3);
		dist.grad << 2*p, 2*(t-1)*p, -2*t*p;

		Matrix3d O = Matrix3d::Identity(3, 3) - (l*l.transpose())/ll;
		Vector3d dtdl2 = (a - 2*t*l)/ll;

		//! Holy fucking shit!
		Matrix3d H_xx = 2*O;
		Matrix3d H_xl2 = -2*t*O - 2*(p*l.transpose())/ll;
		Matrix3d H_xl1 = -(H_xx + H_xl2);
		Matrix3d H_l1l2 = 2*(dtdl2*p.transpose()) + (t-1)*H_xl2;
		Matrix3d H_l1x = H_xl1.transpose();
		Matrix3d H_l2x = H_xl2.transpose();
		Matrix3d H_l1l1 = -(H_l1x + H_l1l2);
		Matrix3d H_l2l1 = H_l1l2.transpose();
		Matrix3d H_l2l2 = -(H_l2x + H_l2l1);

		dist.hess = MatrixXd(12, 12);
		dist.hess << H_xx,  H_l1x,  H_l2x,
				 	 H_xl1, H_l1l1, H_l2l1,
					 H_xl2, H_l1l2, H_l2l2;
	}

	return dist;
}
   


Distance PointPlaneDist(Vector3d& x, Vector3d& p1, Vector3d& p2, Vector3d& p3, bool valueOnly) {
	Distance dist;

	Vector3d a = x-p1;
    Vector3d e1 = p2-p1;
    Vector3d e2 = p3-p1;

    Vector3d n = e1.cross(e2);
	double nn = n.dot(n);

    double s = a.dot(n)/nn;
    Vector3d p = s*n;

	dist.value = p.dot(p);
	if (!valueOnly) {
		Matrix3d dndp2 = asSkewSymmetric(e2);
		Matrix3d dndp3 = asSkewSymmetric(e1);
		
		Vector3d temp = a-2*s*n / nn;
		Vector3d dsdp2 = dndp2 * temp;
		Vector3d dsdp3 = dndp3 * temp;

		Vector3d grad_x = 2*p;
		Vector3d grad_p2 = (dsdp2*n.transpose() + s*dndp2) * 2*p;
		Vector3d grad_p3 = (dsdp3*n.transpose() + s*dndp2) * 2*p;
		Vector3d grad_p1 = -(grad_x + grad_p2 + grad_p3);

		dist.grad = Matrix3Xd(3, 4);
		dist.grad << grad_x, grad_p1, grad_p2, grad_p3;

		dist.hess = MatrixXd::Zero(12, 12);
	}

	return dist;
}

