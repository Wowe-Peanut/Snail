import copy
from cmath import inf

import numpy as np
import numpy.linalg as LA
import scipy.sparse as sparse
from scipy.sparse.linalg import spsolve

import InertiaEnergy
import MassSpringEnergy

def step_forward(x, e, v, m, l2, k, h, tol):
    x_tilde = x + v * h     # implicit Euler predictive position
    x_n = copy.deepcopy(x)

    # Newton loop
    iter = 0
    E_last = IP_val(x, e, x_tilde, m, l2, k, h)
    p = search_dir(x, e, x_tilde, m, l2, k, h)
    while LA.norm(p, inf) / h > tol:
        print('Iteration', iter, ':')
        print('residual =', LA.norm(p, inf) / h)

        # line search
        alpha = 1
        while IP_val(x + alpha * p, e, x_tilde, m, l2, k, h) > E_last:
            alpha /= 2
        print('step size =', alpha)

        x += alpha * p
        E_last = IP_val(x, e, x_tilde, m, l2, k, h)
        p = search_dir(x, e, x_tilde, m, l2, k, h)
        iter += 1

    v = (x - x_n) / h   # implicit Euler velocity update
    return [x, v]

def IP_val(x, e, x_tilde, m, l2, k, h):
    return InertiaEnergy.val(x, x_tilde, m) + h * h * MassSpringEnergy.val(x, e, l2, k)     # implicit Euler

def IP_grad(x, e, x_tilde, m, l2, k, h):
    return InertiaEnergy.grad(x, x_tilde, m) + h * h * MassSpringEnergy.grad(x, e, l2, k)   # implicit Euler

def IP_hess(x, e, x_tilde, m, l2, k, h):
    IJV_In = InertiaEnergy.hess(x, x_tilde, m)
    IJV_MS = MassSpringEnergy.hess(x, e, l2, k)
    
    # IJV_MS[2] is the value of each non-zero number in the MassSpring hessian, we multiply by h^2 because the incremental potential function
    # we are trying to minimize has the term (delta t)^2P(x) but the MassSpringEnergy.hess only returns the Hessian of P(x) (the potential energy of the 
    # spring system. So we just add the scalar later since it has no effect on the P(x) hessian calculation
    IJV_MS[2] *= h * h    # implicit Euler
    
    # Combines the lists together so all the (row, col, val) triplets are in three big lists
    IJV = np.append(IJV_In, IJV_MS, axis=1)

    # spare.coo is "coordinate matrix" which starts with a zero matrix of the designated size and adds IVJ[2] to the location (IJV[0], IJV[1])
    # this effectively adds the two space-efficient representation of the two Hessians and then converts the entire thing to csr
    # note that csr is slightly different then what we had before, csr is (V, COL_INDEX, ROW_INDEX)

    H = sparse.coo_matrix((IJV[2], (IJV[0], IJV[1])), shape=(len(x) * 2, len(x) * 2)).tocsr()
    return H

def search_dir(x, e, x_tilde, m, l2, k, h):
    projected_hess = IP_hess(x, e, x_tilde, m, l2, k, h)

    # This reshaping is the same as flattening the pairs of xi, yi gradient values into a single vector 
    # except its still technically two (just a bunch of singletons: [[x1g], [y1g], ...]
    # (n,) is 1d, (n,1) is 2d but second dimension is singletons
    reshaped_grad = IP_grad(x, e, x_tilde, m, l2, k, h).reshape(len(x) * 2, 1)

    # The search direction is -Hess_inv * gradient which is the same as solving Hx=g but since H is nearly always spare, we
    # uses spsolve which is optimized for this and pass in the hess in CSR format (the projection is done in the MSE.hess call)
    return spsolve(projected_hess, -reshaped_grad).reshape(len(x), 2)
