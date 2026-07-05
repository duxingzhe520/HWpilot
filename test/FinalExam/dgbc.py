import copy

def unwrap_single(x):
    if isinstance(x, int):
        return x
    if isinstance(x, tuple):
        if len(x) == 1:
            # print(f"tuple_len == 1 and x = {x}")
            # print(f"x[0] = {x[0]}")
            return unwrap_single(x[0])
        # print(f"x = {x}")
        # print(f"new_tuple = {(unwrap_single(y) for y in x)}")
        return tuple(unwrap_single(y) for y in x)
    if isinstance(x, list):
        if len(x) == 1:
            # print(f"list_len == 1 and x = {x}")
            return unwrap_single(x[0])
        return [unwrap_single(y) for y in x]

T = int(input())

for _ in range(T):
    obj = eval(input())
    backup = copy.deepcopy(obj)
    result = unwrap_single(obj)
    assert obj == backup, "输入对象被修改"
    print(result)
