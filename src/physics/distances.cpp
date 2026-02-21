
#include "distances.h"
#include <Eigen/Dense> 
#include <iostream>
using std::vector, Eigen::Vector3d, Eigen::Matrix3Xd, Eigen::MatrixXd, Eigen::Matrix3d;

const double ZERO_TOL = 1e-3;


Matrix3d asSkewSymmetric(const Vector3d& v) {
	Matrix3d ssmat;
	ssmat << 0, -v[2], v[1], v[2], 0, -v[0], -v[1], v[0], 0;

	return ssmat;
}

Matrix3Xd mapGrad(Matrix3Xd& grad, int newSize, vector<int> idxmap) {
	int curSize = (int) idxmap.size();

	Matrix3Xd newGrad = Matrix3Xd::Zero(3, newSize);
	for (int idx=0; idx<curSize; idx++) {
		newGrad.col(idxmap[idx]) = grad.col(idx);
	}

	return newGrad;
}

MatrixXd mapHess(MatrixXd& hess, int newSize, vector<int> idxmap) {
	int curSize = (int) idxmap.size();

	MatrixXd newHess = MatrixXd::Zero(3*newSize, 3*newSize);
	for (int ridx=0; ridx<curSize; ridx++) {
		for (int cidx=0; cidx<curSize; cidx++) {
			newHess.block<3, 3>(3*idxmap[ridx], 3*idxmap[cidx]) = hess.block<3, 3>(3*ridx, 3*cidx);
		}
	}

	return newHess;
}

Distance mapDistance(Distance& dist, int newSize, vector<int> idxmap, int flags) {
	if (flags & D_GRAD) dist.grad = mapGrad(dist.grad, newSize, idxmap);
	if (flags & D_HESS)	dist.hess = mapHess(dist.hess, newSize, idxmap);

	return dist;
}

Distance PointPointDist(const Vector3d& x1, const Vector3d& x2, int flags) {
	Distance dist;

	Vector3d diff = x1-x2;
	Matrix3d I2 = 2*Matrix3d::Identity(3, 3);
	dist.value = diff.dot(diff);

	if (flags & D_GRAD) {
		dist.grad = Matrix3Xd(3, 2);
		dist.grad << 2*diff, -2*diff;

		if (flags & D_HESS) {
			dist.hess = MatrixXd(6, 6);
			dist.hess << I2, -I2, -I2, I2;
		} 
	}

	return dist;
}

Distance PointLineDist(const Vector3d& x, const Vector3d& l1, const Vector3d& l2, int flags) {
	Distance dist;

	Vector3d l = l2-l1;
	Vector3d a = x-l1;
	
	double ll = l.dot(l);
	double t = a.dot(l)/ll;
	Vector3d p = a-t*l;
	dist.value = p.dot(p);


	if (flags & D_GRAD) {
		dist.grad = Matrix3Xd(3, 3);
		dist.grad << 2*p, 2*(t-1)*p, -2*t*p;

		if (flags & D_HESS) {
			Matrix3d O = Matrix3d::Identity(3, 3) - (l*l.transpose())/ll;
			Vector3d dtdl2 = (a - 2*t*l)/ll;
	
			//! Holy fucking shit!
			Matrix3d H_xx = 2*O;
			Matrix3d H_xl2 = -2*t*O - 2*(p*l.transpose())/ll;
			Matrix3d H_xl1 = -(H_xx + H_xl2);
			Matrix3d H_l1l2 = 2*dtdl2*p.transpose() + (t-1)*H_xl2;
			Matrix3d H_l1x = H_xl1.transpose();
			Matrix3d H_l2x = H_xl2.transpose();
			Matrix3d H_l1l1 = -(H_l1x + H_l1l2);
			Matrix3d H_l2l1 = H_l1l2.transpose();
			Matrix3d H_l2l2 = -(H_l2x + H_l2l1);
	
			dist.hess = MatrixXd(9, 9);
			dist.hess << H_xx,  H_l1x,  H_l2x,
						  H_xl1, H_l1l1, H_l2l1,
						 H_xl2, H_l1l2, H_l2l2;
		}
	}

	return dist;
}
   
Distance PointPlaneDist(const Vector3d& x, const Vector3d& p1, const Vector3d& p2, const Vector3d& p3, int flags) {
	Distance dist;

	Vector3d a = x-p1;
    Vector3d e1 = p2-p1;
    Vector3d e2 = p3-p1;

    Vector3d n = e1.cross(e2);
	double nn = n.dot(n);

    double s = a.dot(n)/nn;
    Vector3d p = s*n;

	dist.value = p.dot(p);
	if (flags & D_GRAD) {
		Matrix3d dndp2 = asSkewSymmetric(e2);
		Matrix3d dndp3 = -asSkewSymmetric(e1);
		
		Vector3d temp = a-2*p;
		Vector3d dsdp2 = (dndp2 * temp) / nn;
		Vector3d dsdp3 = (dndp3 * temp) / nn;

		Vector3d grad_x = 2*p;
		Vector3d grad_p2 = (dsdp2*n.transpose() + s*dndp2) * (2*p);
		Vector3d grad_p3 = (dsdp3*n.transpose() + s*dndp3) * (2*p);
		Vector3d grad_p1 = -(grad_x + grad_p2 + grad_p3);

		dist.grad = Matrix3Xd(3, 4);
		dist.grad << grad_x, grad_p1, grad_p2, grad_p3;

		if (flags & D_HESS) {
			dist.hess = MatrixXd::Zero(12, 12);
		}
	}

	return dist;
}

Distance LineLineDist(const Vector3d& l11, const Vector3d& l12, const Vector3d& l21, const Vector3d& l22, int flags) {
	Distance dist;

	Vector3d a = l12-l11;
    Vector3d b = l22-l21;
    Vector3d c = l11-l21;

	Vector3d n = a.cross(b);
	double nn = n.dot(n);

	double s = c.dot(n)/nn;
	Vector3d p = s*n;

	dist.value = p.dot(p);
	if (flags & D_GRAD) {
		Matrix3d askew = -asSkewSymmetric(a);
		Matrix3d bskew = -asSkewSymmetric(b);

		Vector3d temp = c-2*p;
		Vector3d dsda1 = (n + bskew*temp)/nn;
		Vector3d dsda2 = -bskew*temp/nn;
		Vector3d dsdb2 = askew*temp/nn;

		Vector3d grad_a1 = (dsda1*n.transpose() + s*bskew) * 2*p;
		Vector3d grad_a2 = (dsda2*n.transpose() - s*bskew) * 2*p;
		Vector3d grad_b2 = (dsdb2*n.transpose() + s*askew) * 2*p;
		Vector3d grad_b1 = -(grad_a1 + grad_a2 + grad_b2);

		dist.grad = Matrix3Xd(3, 4);
		dist.grad << grad_a1, grad_a2, grad_b1, grad_b2;

		if (flags & D_HESS) {
			dist.hess = MatrixXd::Zero(12, 12);
		}
	}
	
	return dist;
}

Distance PointEdgeDist(const Vector3d& x, const Vector3d& l1, const Vector3d& l2, int flags) {

	// Helper values
    Vector3d l = l2 - l1;
    Vector3d a = x - l1;
    double ll = l.dot(l);
    
    // Projection point onto line spanned by edge
    double t = (abs(ll) > ZERO_TOL) ? a.dot(l) / ll : 0.0;

	// PointLine
    if (t > 0 && t < 1) {
        return PointLineDist(x, l1, l2, flags);
    } 

	// PointPoint (a and l1)
	else {
		Vector3d linePoint = (t <= 0) ? l1 : l2;
		Distance dist = PointPointDist(x, linePoint, flags);
		return mapDistance(dist, 3, {0, (t <= 0) ? 1 : 2}, flags);
	}
}

Distance EdgeEdgeDist(const Vector3d& e11, const Vector3d& e12, const Vector3d& e21, const Vector3d& e22, int flags) {

	// Helper values
	Vector3d a = e12 - e11;
	Vector3d b = e22 - e21;
	Vector3d c = e11 - e21;
	double aa = a.dot(a);
	double bb = b.dot(b);
	double ab = a.dot(b);
	double ac = a.dot(c);
	double bc = b.dot(c);

	double denom = aa*bb - ab*ab;
	double alpha, beta; 

	// CASE 1 | Parallel -----------------------------------------------------------
	if (abs(denom) < ZERO_TOL) {

		// Degrades to PointEdge (just chooses one)
		Distance dist = PointEdgeDist(e11, e21, e22, flags);
		return mapDistance(dist, 4, {0, 2, 3}, flags);

	} 



	// CASE 2 | Non-Parallel -------------------------------------------------------
	else {
		
		// Line ratios
		alpha = (ac*bb - bc*ab)/(aa*bb - ab*ab);
		beta = (alpha*aa - ac)/ab;
		bool aonline = (alpha > 0) && (alpha < 1);
		bool bonline = (beta > 0) && (beta < 1);

		// LineLine
		if (aonline && bonline) {
			return LineLineDist(e11, e12, e21, e22, flags);
		} 
		
		// PointLine
		else if (aonline != bonline) {
			Vector3d point, l1, l2;
			vector<int> idxmap(3);
			
			// Edge 1 is the line, Point is endpoint of edge 2
			if (aonline) { 
				int p_idx = (beta < 0) ? 2 : 3;
				point = (beta < 0) ? e21 : e22;
				l1 = e11; l2 = e12;
				idxmap = {p_idx, 0, 1}; 
			} 
			
			// Edge 2 is the line, Point is endpoint of edge 1
			else { 
				int p_idx = (alpha < 0) ? 0 : 1;
				point = (alpha < 0) ? e11 : e12;
				l1 = e21; l2 = e22;
				idxmap = {p_idx, 2, 3};
			}

			Distance dist = PointLineDist(point, l1, l2, flags);
			return mapDistance(dist, 4, idxmap, flags);
		} 

		// PointPoint
		else {
			int p1_idx = (alpha < 0) ? 0 : 1;
			int p2_idx = (beta < 0) ? 2 : 3;
			vector<int> idxmap = {p1_idx, p2_idx};
			
			Distance dist = PointPointDist((alpha < 0 ? e11 : e12), (beta < 0 ? e21 : e22), flags);
			return mapDistance(dist, 4, idxmap, flags);
		}
	}
}

Distance PointTriangleDist(const Vector3d& x, const Vector3d& t1, const Vector3d& t2, const Vector3d& t3, int flags) {

	// Helper values
    Vector3d e0 = t2 - t1;
    Vector3d e1 = t3 - t1;
    Vector3d a = x - t1;
    double e0e0 = e0.dot(e0);
    double e0e1 = e0.dot(e1);
    double e1e1 = e1.dot(e1);
    double ae0 = a.dot(e0);
    double ae1 = a.dot(e1);

    double denom = e0e0 * e1e1 - e0e1 * e0e1;
    
    // Degenerate triangle (collinear points) -> Triangle is a line 
    if (abs(denom) < ZERO_TOL) {
        Distance dist = PointEdgeDist(x, t1, t2, flags); 
		return mapDistance(dist, 4, {0, 1, 2}, flags);
    }

	// Barycentric coordinates in triangle plane (b1, b2, b3 correspond to t1, t2, t3 respectively)
    double b2 = (e1e1 * ae0 - e0e1 * ae1) / denom;
    double b3 = (e0e0 * ae1 - e0e1 * ae0) / denom;
    double b1 = 1.0 - b2 - b3;

	// Projection falls inside triangle
	if (b1 >= 0 && b2 >= 0 && b3 >= 0) {
        return PointPlaneDist(x, t1, t2, t3, flags);
    }

	// Outside edge t2,t3
	else if (b1 < 0) {
		Distance dist = PointEdgeDist(x, t2, t3, flags);
		return mapDistance(dist, 4, {0, 2, 3}, flags);
	}

	// Outside edge t1,t3
	else if (b2 < 0) {
		Distance dist = PointEdgeDist(x, t1, t3, flags);
		return mapDistance(dist, 4, {0, 1, 3}, flags);
	}

	// Outside edge t1,t2
	else {
		Distance dist = PointEdgeDist(x, t1, t2, flags);
		return mapDistance(dist, 4, {0, 1, 2}, flags);
	}
}