def deepcopy(a):
    b = None
    b = list()
    for ele in a:
        if isinstance(ele, int):
            b.append(ele)
        else:
            b.append(deepcopy(ele))
    if isinstance(a, list):
        return b
    return tuple(b)

a = [1,2,[3,[4],5],(6,[7,[8],9])]
b = deepcopy(a)
print(b)
a[2][1].append(400)
a[3][1][1].append(800)
print(a)
print(b)