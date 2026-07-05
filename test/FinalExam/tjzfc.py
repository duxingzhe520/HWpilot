if __name__ == "__main__":
    ls = input()
    length = len(ls)
    maxnum = 0
    maxi = 0
    for i in range(26):
        letter = chr(i + ord('a'))
        tmp = 0
        for x in range(length):
            # print(f"x = {x}")
            # print(f"ls[x] = {ls[x]}")
            # print(f"letter = {letter}")
            if ls[x] == letter:
                tmp += 1
            # print(f"tmp = {tmp}")
        if tmp > maxnum:
            maxnum = tmp
            maxi = i
    print(chr(maxi + ord('a')), maxnum)
    

