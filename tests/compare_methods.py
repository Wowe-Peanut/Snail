import numpy as np
from sympy import *
from collections import defaultdict


# sympy brute force
def pp(x1, x2):
    x11, x12, x13, x21, x22, x23 = symbols("x11 x12 x13 x21 x22 x23")
    varsubs = {x11: x1[0], x12: x1[1], x13: x1[2], x21: x2[0], x22: x2[1], x23: x2[2]}
    vars = [x11, x12, x13, x21, x22, x23]

    f = (x11-x21)**2 + (x12-x22)**2 + (x13-x23)**2 

    val = f.subs(varsubs)
    grad = [f.diff(v).subs(varsubs) for v in vars] 
    hess = [[f.diff(v2).diff(v1).subs(varsubs) for v2 in vars] for v1 in vars]

    return val, np.array(grad, dtype=float), np.array(hess, dtype=float)

def pl(x, l1, l2):
    x1, x2, x3, l11, l12, l13, l21, l22, l23 = symbols("x1, x2, x3, l11, l12, l13, l21, l22, l23")
    varsubs = {x1: x[0], x2: x[1], x3: x[2], l11: l1[0], l12: l1[1], l13: l1[2], l21: l2[0], l22: l2[1], l23: l2[2]}
    vars = [x1, x2, x3, l11, l12, l13, l21, l22, l23]

    f = (x1-l11)**2 + (x2-l12)**2 + (x3-l13)**2 - ((x1-l11)*(l21-l11) + (x2-l12)*(l22-l12) + (x3-l13)*(l23-l13))**2/((l21-l11)**2 + (l22-l12)**2 + (l23-l13)**2)

    val = f.subs(varsubs)
    grad = [f.diff(v).subs(varsubs) for v in vars] 
    hess = [[f.diff(v2).diff(v1).subs(varsubs) for v2 in vars] for v1 in vars]

    return val, np.array(grad, dtype=float), np.array(hess, dtype=float)


# numpy simplified
def pp2(x1, x2):
    diff = x1-x2
    i = 2*np.identity(3)

    val = diff.dot(diff)
    grad = np.concatenate((2*diff, -2*diff))
    hess = np.block([
        [i, -i],
        [-i, i]
    ])

    return val, grad, hess
    
def pl2(x, l1, l2):
    l = l2-l1
    a = x-l1
    lsq = l.dot(l)
    
    t = a.dot(l)/lsq
    p = a - t*l

    val = p.dot(p)
    grad = np.concatenate((2*p, 2*(t-1)*p, -2*t*p))

    O = np.identity(3) - np.outer(l, l)/lsq
    dtdl2 = (a.T - 2*t*l.T)/lsq

    H_xx = 2*O
    H_xl2 = -2*t*O - 2*np.outer(p, l)/lsq
    H_xl1 = -(H_xx + H_xl2)

    H_l2l2 = -2*p*dtdl2 - t*H_xl2
    H_l1x = H_xl1.T
    H_l2x = H_xl2.T
    H_l2l1 = -(H_l2l2 + H_l2x)
    H_l1l2 = H_l2l1.T
    H_l1l1 = -(H_l1l2 + H_l1x)


    # H_l1l2 = 2*p*dtdl2 + (t-1)*H_xl2
    # H_l1x = H_xl1.T
    # H_l2x = H_xl2.T
    # H_l1l1 = -(H_l1x + H_l1l2)
    # H_l2l1 = H_l1l2.T
    # H_l2l2 = -(H_l2x + H_l2l1)

    hess = np.block([
        [H_xx, H_l1x, H_l2x],
        [H_xl1, H_l1l1, H_l2l1],
        [H_xl2, H_l1l2, H_l2l2]
    ])


    return val, grad, hess

bruteforce = [
    ("PointPoint", pp, 2),
    ("PointLine", pl, 3)
]

simplified = [
    ("PointPoint", pp2, 2),
    ("PointLine", pl2, 3)
]

results = defaultdict(lambda: {"val": [], "grad": [], "hess": []})

seed = np.random.randint(100)
for version in [bruteforce, simplified]:
    np.random.seed(seed)
    for name, func, dim in version:
        points = [10*np.random.randn(3) for _ in range(dim)]

        val, grad, hess = func(*points)
        results[name]["val"].append(val)
        results[name]["grad"].append(grad)
        results[name]["hess"].append(hess)
        


np.set_printoptions(linewidth=200)

for test, outputs in results.items():
    print(test)
    for name, outputlist in outputs.items():
        if name == "val":
            match = abs(outputlist[0] - outputlist[1]) < 0.0001
        else:
            match = np.isclose(outputlist[0], outputlist[1]).all()

        print(f"\t{name}:  {'MATCH' if match else 'DIFFERENT'}")
        
        for value in outputlist:
            strval = str(value).replace("\n", "\n\t\t")
            print(f"\t\t{strval}\n")

        if not match:
            matchstr = str(np.isclose(outputlist[0], outputlist[1]).astype(int)).replace("\n", "\n\t\t")
            print(f"\t\t{matchstr}\n")

    print("\n\n\n")

        
            

            
