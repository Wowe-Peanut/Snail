import numpy as np
from sympy import *
from collections import defaultdict
import sys
seed = 1001
pointscale = 2

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

def pt(x, t1, t2, t3):
    x1, x2, x3, t11, t12, t13, t21, t22, t23, t31, t32, t33 = symbols("x1, x2, x3, t11, t12, t13, t21, t22, t23, t31, t32, t33")
    varsubs = {x1: x[0], x2: x[1], x3: x[2], t11: t1[0], t12: t1[1], t13: t1[2], t21: t2[0], t22: t2[1], t23: t2[2], t31: t3[0], t32: t3[1], t33: t3[2]}
    vars = [x1, x2, x3, t11, t12, t13, t21, t22, t23, t31, t32, t33]

    a = [x1-t11, x2-t12, x3-t13]
    e1 = [t21-t11, t22-t12, t23-t13]
    e2 = [t31-t11, t32-t12, t33-t13]

    n = [(e1[1]*e2[2] - e1[2]*e2[1]), -(e1[0]*e2[2] - e1[2]*e2[0]), (e1[0]*e2[1] - e1[1]*e2[0])]
    nlen = sqrt(n[0]**2 + n[1]**2 + n[2]**2)
    nhat = [n[0]/nlen, n[1]/nlen, n[2]/nlen]
    f = (a[0]*nhat[0] + a[1]*nhat[1] + a[2]*nhat[2])**2

    val = f.subs(varsubs)
    grad = [f.diff(v).subs(varsubs) for v in vars] 
    # hess = [[f.diff(v2).diff(v1).subs(varsubs) for v2 in vars] for v1 in vars]

    return val, np.array(grad, dtype=float), np.zeros((12,12), dtype=float)

def ll(a1, a2, b1, b2):
    a11, a12, a13, a21, a22, a23, b11, b12, b13, b21, b22, b23 = symbols("a11, a12, a13, a21, a22, a23, b11, b12, b13, b21, b22, b23")
    varsubs = {a11: a1[0], a12: a1[1], a13: a1[2], a21: a2[0], a22: a2[1], a23: a2[2], b11: b1[0], b12: b1[1], b13: b1[2], b21: b2[0], b22: b2[1], b23: b2[2]}
    vars = [a11, a12, a13, a21, a22, a23, b11, b12, b13, b21, b22, b23]

    a = [a21 - a11, a22 - a12, a23 - a13]
    b = [b21 - b11, b22 - b12, b23 - b13]
    n = [(a[1]*b[2] - a[2]*b[1]), -(a[0]*b[2] - a[2]*b[0]), (a[0]*b[1] - a[1]*b[0])]
    nlen = sqrt(n[0]**2 + n[1]**2 + n[2]**2)
    nhat = [n[0]/nlen, n[1]/nlen, n[2]/nlen]
    f = ((a11-b11)*nhat[0] + (a12-b12)*nhat[1] + (a13-b13)*nhat[2])**2

    val = f.subs(varsubs)
    grad = [f.diff(v).subs(varsubs) for v in vars] 
    # hess = [[f.diff(v2).diff(v1).subs(varsubs) for v2 in vars] for v1 in vars]

    return val, np.array(grad, dtype=float), np.zeros((12,12), dtype=float)



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
    dtdl2 = (a - 2*t*l)/lsq                 

    H_xx = 2*O
    H_xl2 = -2*t*O - 2*np.outer(p, l)/lsq
    H_xl1 = -(H_xx + H_xl2)
    H_l1l2 = 2*np.outer(dtdl2,p) + (t-1)*H_xl2 
    H_l1x = H_xl1.T
    H_l2x = H_xl2.T
    H_l1l1 = -(H_l1x + H_l1l2)
    H_l2l1 = H_l1l2.T
    H_l2l2 = -(H_l2x + H_l2l1)

    hess = np.block([
        [H_xx, H_l1x, H_l2x],
        [H_xl1, H_l1l1, H_l2l1],
        [H_xl2, H_l1l2, H_l2l2]
    ])


    return val, grad, hess

def pt2(x, t1, t2, t3):

    a = x-t1
    e1 = t2-t1
    e2 = t3-t1
    n = np.cross(e1, e2)
    s = a.dot(n)/n.dot(n)
    p = s*n

    
    dndt2 = np.array([
        [0,-e2[2],e2[1]],
        [e2[2],0,-e2[0]],
        [-e2[1],e2[0],0]
    ])
    dndt3 = -np.array([
        [0,-e1[2],e1[1]],
        [e1[2],0,-e1[0]],
        [-e1[1],e1[0],0]
    ])

    dsdt2 = np.matmul(dndt2, a-2*s*n)/n.dot(n)
    dsdt3 = np.matmul(dndt3, a-2*s*n)/n.dot(n)


    grad_x = 2*p
    grad_t2 = np.matmul(np.outer(dsdt2, n) + s*dndt2, 2*p)
    grad_t3 = np.matmul(np.outer(dsdt3, n) + s*dndt3, 2*p)
    grad_t1 = -(grad_x + grad_t2 + grad_t3)

    val = p.dot(p)
    grad = np.concatenate((grad_x, grad_t1, grad_t2, grad_t3))


    return val, grad, np.zeros((12,12), dtype=float)

def llratios(a1, a2, b1, b2):
    la = a2-a1
    lb = b2-b1
    c = b1-a1

    aa = la.dot(la)
    bb = lb.dot(lb)
    ab = la.dot(lb)
    ca = c.dot(la)
    cb = c.dot(lb)

    # if near parallel (ab ~ 0) then apparently it collapses to point-edge... 
    # do we need to check all four?
    if ab < 1e-8:
        alpha = 0 
        beta = 0
    else:
        alpha = ((ca*bb/ab)-cb)/((aa*bb/ab)-ab)
        beta = (alpha*aa-ca)/ab
    
    return alpha, beta

def ll2(a1, a2, b1, b2):
    a = a2 - a1
    b = b2 - b1
    n = a.cross(b)
    c = a1 - b1

    val = ()**2

    return val, np.zeros(12, dtype=float), np.zeros((12,12), dtype=float)



def test_numpy():
    bruteforce = [
        ("PointPoint", pp, 2),
        ("PointLine", pl, 3),
        ("PointPlane", pt, 4),
        ("LineLine", ll, 4)
    ]

    simplified = [
        ("PointPoint", pp2, 2),
        ("PointLine", pl2, 3),
        ("PointPlane", pt2, 4),
        ("LineLine", ll2, 4)
    ]

    results = defaultdict(lambda: {"points": [], "val": [], "grad": [], "hess": []})
    for version in [bruteforce, simplified]:
        np.random.seed(seed)
        for name, func, dim in version:
            points = [pointscale*np.random.randn(3) for _ in range(dim)]

            val, grad, hess = func(*points)
            results[name]["points"] = points
            results[name]["val"].append(val)
            results[name]["grad"].append(grad)
            results[name]["hess"].append(hess)
            

    np.set_printoptions(linewidth=200)

    for test, outputs in results.items():
        print(test)
        for name, outputlist in outputs.items():
            if name == "points":
                s = f"\tVertices:\n{np.array(outputlist)}"
                print(s.replace("\n", "\n\t\t"))
                continue

            if name == "val":
                match = abs(outputlist[0] - outputlist[1]) < 0.0001
            else:
                match = np.isclose(outputlist[0], outputlist[1]).all()

            print(f"\t{name}:  {'MATCH' if match else 'DIFFERENT'}")
            
            for value in outputlist:
                strval = str(value).replace("\n", "\n\t\t")
                print(f"\t\t{strval}\n")

            if not match and name != "val":
                matchstr = str(np.isclose(outputlist[0], outputlist[1]).astype(int)).replace("\n", "\n\t\t")
                print(f"\t\t{matchstr}\n")

        print("\n\n\n")

def print_sympy():
    functions = [
        ("PointPoint", pp, 2),
        ("PointLine", pl, 3),
        ("PointPlane", pt, 4),
        ("LineLine", ll, 4)
    ]

    np.random.seed(seed)

    print(len(functions))
    for name, func, dim in functions:
        points = [pointscale*np.random.randn(3) for _ in range(dim)]

        val, grad, hess = func(*points)

        print(name)
        print(dim)
        for point in points:
            print(*point)
        print(val)
        print(*grad)
        for row in hess:
            print(*row)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        option = sys.argv[1]

        if option == "test_numpy":
            test_numpy()
        elif option == "print_sympy":
            print_sympy()