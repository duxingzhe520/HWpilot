num_dict = {}
price_dict = {}

if __name__ == "__main__":
    n, m = map(int, input().split())
    for _ in range(m):
        s = input().split()
        num_dict[s[0]] = int(s[2])
        price_dict[s[0]] = int(s[1])
    
    total = 0
    for _ in range(n):
        s = input().split()
        for dish in s:
            if num_dict[dish] > 0:
                num_dict[dish] -= 1
                total += price_dict[dish]
    print(total)
    