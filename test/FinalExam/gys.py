def is_gys(n):
    while n % 2 == 0:
        n = n / 2
    while n % 3 == 0:
        n = n / 3
    if n == 1:
        return True
    return False

if __name__ == "__main__":
    n = int(input())
    if is_gys(n):
        print("yes")
    else:
        print("no")
