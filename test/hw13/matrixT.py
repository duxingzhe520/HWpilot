h, w = map(int, input().split())
matrix = [list(map(int, input().split())) for _ in range(h)]

for i in range(w):
    for j in range(h):
        print(matrix[j][i], end=" ")
    print()