import math

n = int(input())
c = 0x3f3f3f3f
for i in range(1, int(math.sqrt(n)) + 1):
    if n % i == 0:
        c = min(c, 2 * (i + n // i))
print(c)