import numpy as np
np.random.seed(1010)
# --- Point-Point Functions ---

def ppval(x1, x2):
    diff = x1 - x2
    return np.dot(diff, diff)

def ppgrad(x1, x2):
    diff = x1 - x2
    return np.concatenate((2 * diff, -2 * diff))

def pphess(x1, x2):
    I2 = 2 * np.eye(3)
    return np.block([[I2, -I2], [-I2, I2]])

# --- Point-Line Functions ---

def _get_pl_terms(x, l1, l2):
    """Helper to compute shared geometric terms for Point-Line functions."""
    a = x - l1
    L = l2 - l1
    sq_norm = np.dot(L, L)
    
    # Projection factor (how far x is along the line L)
    # t = 0 means x projects onto l1, t = 1 means x projects onto l2
    t = np.dot(a, L) / sq_norm
    
    # Orthogonal vector from line to point
    # p = a - (projection of a onto L)
    p = a - t * L
    
    return a, L, sq_norm, t, p

def plval(x, l1, l2):
    _, _, _, _, p = _get_pl_terms(x, l1, l2)
    return np.dot(p, p)

def plgrad(x, l1, l2):
    a, L, sq_norm, t, p = _get_pl_terms(x, l1, l2)
    
    gx = 2 * p
    gl2 = -2 * t * p
    gl1 = -(gx + gl2) # Gradients must sum to zero for translational invariance
    
    return np.concatenate((gx, gl1, gl2))

def plhess(x, l1, l2):
    a, L, sq_norm, t, p = _get_pl_terms(x, l1, l2)
    
    I = np.eye(3)
    P = np.outer(L, L) / sq_norm  # Parallel projection matrix
    O = I - P                     # Orthogonal projection matrix
    
    # Hessian blocks
    # H_xx is constant: 2 * Orthogonal Projector
    H_xx = 2 * O
    
    # Intermediate shared matrices for off-diagonals
    m1 = (2 / sq_norm) * np.outer(p, L)
    m2 = (2 / sq_norm) * np.outer(L, p)
    
    H_xl2 = -2 * t * O + m1
    H_xl1 = -(H_xx + H_xl2)
    
    H_l2l2 = 2 * (t**2) * O - (t * (m1 + m2))
    H_l1l2 = -t * H_xl1 - m2
    H_l1l1 = -(H_xl1 + H_l1l2)

    return np.block([
        [H_xx,   H_xl1,  H_xl2],
        [H_xl1,  H_l1l1, H_l1l2],
        [H_xl2,  H_l1l2, H_l2l2]
    ])

# --- Test Suite ---

if __name__ == '__main__':

    tests = [
        ("PointPoint", ppval, ppgrad, pphess, 2),
        ("PointLine", plval, plgrad, plhess, 3)
    ]

    for name, val, grad, hess, dim in tests:
        points = [np.random.randn(3) for _ in range(dim)]

        print(name)
        print(f"Points = \n{points}\n")
        print(f"Value = \n{val(*points)}\n")
        print(f"Grad = \n{grad(*points)}\n")
        print(f"Hess = \n{hess(*points)}\n")
        print("\n--------------------------------------------------\n")