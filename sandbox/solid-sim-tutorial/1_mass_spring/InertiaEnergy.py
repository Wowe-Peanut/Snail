import numpy as np

def val(x, x_tilde, m):
    sum = 0.0
    for i in range(0, len(x)):

        # Unlike the [x1, y1, x2, y2] representation they use for notation, [[x1,y1], [x2,y2]] is used and 
        # to avoid making m[] redundant (because x1 and y1 have the same mass), it just combines everything here
        # and uses self dot to get x1**2 + y1**2 which is the norm squared from the textbook

        diff = x[i] - x_tilde[i]
        sum += m[i] * diff.dot(diff)

    # Moved 0.5 here since its on everything
    return sum*0.5

def grad(x, x_tilde, m):

    # Same idea here, delE/delx1 and delE/dely1 are grouped together because we only store one mass value per point

    g = np.array([[0.0, 0.0]] * len(x))
    for i in range(0, len(x)):
        g[i] = m[i] * (x[i] - x_tilde[i])
    return g

def hess(x, x_tilde, m):

    # Because this is typically spare, we don't want to be storing a bunch of zero values. So instead we store
    # (row_index, col_index, value) triplets which represent A[row_index][col_index] = value. Scipy's spsolve
    # which is optimized for solving Ax=b for sparse A, expects this type of format.

    IJV = [[0] * (len(x) * 2), [0] * (len(x) * 2), np.array([0.0] * (len(x) * 2))]
    for i in range(0, len(x)):
        for d in range(0, 2):
            IJV[0][i * 2 + d] = i * 2 + d
            IJV[1][i * 2 + d] = i * 2 + d
            IJV[2][i * 2 + d] = m[i]
    return IJV
