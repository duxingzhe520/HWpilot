def max_ele(mat, i, j, k):
    submat = [row[max(0, j - k // 2) : min(m, j + k // 2 + 1)] for row in mat[max(0, i - k // 2) : min(n, i + k // 2 + 1)]]
    # if i == 0 and j == 0:
    #     print(f"submat = {submat}")
    maxele = -0x3f3f3f3f
    for p in range(len(submat)):
        for q in range(len(submat[0])):
            maxele = max(maxele, submat[p][q])
    return maxele

if __name__ == "__main__":
    n, m, k = map(int, input().split())
    matrix = []
    for _ in range(n):
        matrix.append(list(map(int, input().split())))
    for i in range(n):
        tmp = []
        for j in range(m):
            tmp.append(str(max_ele(matrix, i, j, k)))
        print(' '.join(tmp))
