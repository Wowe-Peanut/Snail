import numpy as np
import utils

def val(x, e, l2, k):
    sum = 0.0
    for i in range(0, len(e)):
        diff = x[e[i][0]] - x[e[i][1]]
        sum += l2[i] * 0.5 * k[i] * (diff.dot(diff) / l2[i] - 1) ** 2
    return sum

def grad(x, e, l2, k):
    g = np.array([[0.0, 0.0]] * len(x))
    for i in range(0, len(e)):
        diff = x[e[i][0]] - x[e[i][1]]
        g_diff = 2 * k[i] * (diff.dot(diff) / l2[i] - 1) * diff
        g[e[i][0]] += g_diff
        g[e[i][1]] -= g_diff
    return g

def hess(x, e, l2, k):

    # 16 b/c each edge has 2, 2d vertices = 4 DOFS, which means a 4x4 Hessian (16)
    # ∂x1  ∂y1  ∂x2  ∂y2 and all 16 combinations of two of these...
    IJV = [[0] * (len(e) * 16), [0] * (len(e) * 16), np.array([0.0] * (len(e) * 16))]


    for i in range(0, len(e)):

        # Get the vector difference between the vertices that make up this edge
        diff = x[e[i][0]] - x[e[i][1]]

        # Calculates ∂^2Pe(x)/∂x1^2 (second partial derivative of x1) (the other second partials in the Hessian are just this times +- 1)
        # Note that H_diff itself is a 2x2 Hessian since we are in 2d
        H_diff = 2 * k[i] / l2[i] * (2 * np.outer(diff, diff) + (diff.dot(diff) - l2[i]) * np.identity(2))

        # When you work out the 2nd derivatives of just this edge (the Hessian), you get the following qaulities:
        # ∂x1^2 = ∂x2^2 = -∂x1x2 = -∂x2x1 (it just works out that way)
        # Note that H_diff is itself a 2x2 matrix (being in 2d and all) so this is a 16x16 matrix.
        H_local = utils.make_PSD(np.block([[H_diff, -H_diff], [-H_diff, H_diff]]))


        # The values of the local hessian add to global matrix. Since H_local is now already PSD, the global one will be as well 
        # (b/c the sum of two convex functions will be convex). THIS SAVES HAVING TO DO A MASSSSSIVE EIGENVALUE DECOMPOSITION OF THE ENTIRE P(x)
        # instead we represent P(x) = sum (e: 1->len(e)) Pe(x) and the 2nd derivative operator is linear so it just applies individually.

        # The indexing just goes a little batshit crazy because we do everything in CSR format (sparse matrices are fucking wild)
        # Rememeber we are writing 16 values for each each (four 2-for loops)
        for nI in range(0, 2):
            for nJ in range(0, 2):
                indStart = i * 16 + (nI * 2 + nJ) * 4
                for r in range(0, 2):
                    for c in range(0, 2):

                        # Or equivalently H_global[e[i][nI]*2 + r] [e[i][nj]*2 + c] = H_local[nI * 2 + r, nJ * 2 + c]
                        IJV[0][indStart + r * 2 + c] = e[i][nI] * 2 + r
                        IJV[1][indStart + r * 2 + c] = e[i][nJ] * 2 + c
                        IJV[2][indStart + r * 2 + c] = H_local[nI * 2 + r, nJ * 2 + c]
    return IJV