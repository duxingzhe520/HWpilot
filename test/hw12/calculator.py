x, y, z = input().split()
x, y = int(x), int(y)
if z == "+":
    print(x + y)
elif z == "-":
    print(x - y)
elif z == "*":
    print(x * y)
elif z == "/":
    if y == 0:
        print("Divided by zero!")
    else:
        print(x // y)
else:
    print("Invalid operator!")
    