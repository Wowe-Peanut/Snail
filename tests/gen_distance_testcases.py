import numpy as np
np.random.seed(1010)

def ppval(x1, x2):
    t_0 = (x1 - x2)
    functionValue = (np.linalg.norm(t_0) ** 2)
    return functionValue

def ppgrad(x1, x2):
    t_0 = (x1 - x2)
    return np.concatenate((2*t_0, -2*t_0))

def pphess(x1, x2):
    I = np.identity(3)
    return np.block([[2*I, -2*I], [-2*I, 2*I]])

def plval(x, l1, l2):
    a = x-l1
    b = l2-l1
    return np.dot(a, a) - np.dot(a, b)**2/np.dot(b, b)

def plgradx(x, l1, l2):
    t_0 = (x - l1)
    t_1 = (l2 - l1)
    t_2 = (np.linalg.norm(t_1) ** 2)
    t_3 = (t_0).dot(t_1)
    return ((2 * t_0) - (((2 / t_2) * t_3) * t_1))

def plgradl1(x, l1, l2):
    t_0 = (x - l1)
    t_1 = (l2 - l1)
    t_2 = np.linalg.norm(t_1)
    t_3 = (t_2 ** 2)
    t_4 = (t_0).dot(t_1)
    t_5 = (2 / t_3)
    t_6 = (t_1).dot(t_0)
    return -(((2 * t_0) + (((2 * (t_6 ** 2)) / (t_2 ** 4)) * t_1)) - (((t_5 * t_4) * t_1) + ((t_5 * t_6) * t_0)))

def plgradl2(x, l1, l2):
    t_0 = (x - l1)
    t_1 = (l2 - l1)
    t_2 = np.linalg.norm(t_1)
    t_3 = (t_2 ** 2)
    t_4 = (t_1).dot(t_0)
    return -((((2 / t_3) * t_4) * t_0) - (((2 * (t_4 ** 2)) / (t_2 ** 4)) * t_1))

def plgrad(x, l1, l2):
    return np.concatenate((plgradx(x, l1, l2), plgradl1(x, l1, l2), plgradl2(x, l1, l2)))

def plhessxx(x, l1, l2):
    t_1 = (l2 - l1)
    t_2 = (2 / (np.linalg.norm(t_1) ** 2))
    return ((2 * np.eye(3, 3)) - (t_2 * np.outer(t_1, t_1)))

def plhessxl1(x, l1, l2):
    t_0 = (x - l1)
    t_1 = (l2 - l1)
    t_2 = (np.linalg.norm(t_1) ** 2)
    t_3 = (t_0).dot(t_1)
    t_4 = (2 / t_2)
    T_5 = np.outer(t_1, t_1)
    t_6 = (t_4 * t_3)
    return -(((((2 * np.eye(3, 3)) + (((4 / (t_2 ** 2)) * t_3) * T_5)) - (t_4 * T_5)) - (t_4 * np.outer(t_1, t_0))) - (t_6 * np.eye(3, 3)))

def plshessxl2(x, l1, l2):
    t_0 = (x - l1)
    t_1 = (l2 - l1)
    t_2 = (np.linalg.norm(t_1) ** 2)
    t_3 = (2 / t_2)
    t_4 = (t_0).dot(t_1)
    t_5 = (t_3 * t_4)
    return -(((t_3 * np.outer(t_1, t_0)) - (((4 / (t_2 ** 2)) * t_4) * np.outer(t_1, t_1))) + (t_5 * np.eye(3, 3)))

def plhessl1l1(x, l1, l2):
    t_0 = (x - l1)
    t_1 = (l2 - l1)
    t_2 = np.linalg.norm(t_1)
    t_3 = (t_2 ** 2)
    t_4 = (2 / t_3)
    t_5 = (t_1).dot(t_0)
    t_6 = (t_5 ** 2)
    t_7 = (t_2 ** 4)
    t_8 = ((4 / t_7) * t_5)
    T_9 = np.outer(t_1, t_1)
    t_10 = ((t_6 * 2) / t_7)
    t_11 = (t_0).dot(t_1)
    T_12 = np.outer(t_1, t_0)
    t_13 = (t_4 * t_11)
    t_14 = (4 / (t_3 ** 2))
    T_15 = np.outer(t_0, t_1)
    t_16 = (t_4 * t_5)
    return -(((((((8 * (t_2 ** 3)) * t_6) / ((t_7 ** 2) * t_2)) * T_9) - (((2 * np.eye(3, 3)) + (t_8 * T_12)) + (t_8 * T_9))) - (t_10 * np.eye(3, 3))) - (((((((((t_14 * t_11) * T_9) - (t_4 * T_9)) - (t_4 * T_12)) - (t_13 * np.eye(3, 3))) + ((t_14 * t_5) * T_15)) - (t_4 * np.outer(t_0, t_0))) - (t_4 * T_15)) - (t_16 * np.eye(3, 3))))

def plshessl1l2(x, l1, l2):
    t_0 = (x - l1)
    t_1 = (l2 - l1)
    t_2 = np.linalg.norm(t_1)
    t_3 = (t_2 ** 2)
    t_4 = (2 / t_3)
    t_5 = (t_1).dot(t_0)
    t_6 = (t_2 ** 4)
    t_7 = (t_5 ** 2)
    t_8 = ((t_7 * 2) / t_6)
    T_9 = np.outer(t_1, t_0)
    t_10 = (t_0).dot(t_1)
    T_11 = np.outer(t_1, t_1)
    t_12 = (t_4 * t_10)
    t_13 = (4 / (t_3 ** 2))
    return -((((((4 / t_6) * t_5) * T_9) - (((8 * ((t_2 ** 3) * t_7)) / ((t_6 ** 2) * t_2)) * T_11)) + (t_8 * np.eye(3, 3))) - (((((t_4 * T_9) - ((t_13 * t_10) * T_11)) + (t_12 * np.eye(3, 3))) - ((t_13 * t_5) * np.outer(t_0, t_1))) + (t_4 * np.outer(t_0, t_0))))

def plshessl2l2(x, l1, l2):
    t_0 = (l2 - l1)
    t_1 = (x - l1)
    t_2 = (t_0).dot(t_1)
    t_3 = np.linalg.norm(t_0)
    t_4 = (t_3 ** 2)
    t_5 = (2 / t_4)
    t_6 = (t_3 ** 4)
    t_7 = (t_2 ** 2)
    t_8 = ((t_7 * 2) / t_6)
    return -(((t_5 * np.outer(t_1, t_1)) - (((4 / (t_4 ** 2)) * t_2) * np.outer(t_1, t_0))) - (((((4 / t_6) * t_2) * np.outer(t_0, t_1)) - (((8 * ((t_3 ** 3) * t_7)) / ((t_6 ** 2) * t_3)) * np.outer(t_0, t_0))) + (t_8 * np.eye(3, 3))))



def plhess(x, l1, l2):

    xx = plhessxx(x, l1, l2)
    xl1 = plhessxl1(x, l1, l2)
    xl2 = plshessxl2(x, l1, l2)
    l1l1 = plhessl1l1(x, l1, l2)
    l1l2 = plshessl1l2(x, l1, l2)
    l2l2 = plshessl2l2(x, l1, l2)

    return np.block([
        [xx, xl1, xl2],
        [xl1, l1l1, l1l2],
        [l2l2, l1l2, l2l2]
    ])

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
        
