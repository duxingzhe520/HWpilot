color_dic = {'y':1, 'w':0}

if __name__ == "__main__":
    n = int(input())
    board = []
    for _ in range(n):
        s = list(input())
        for i in range(len(s)):
            s[i] = color_dic[s[i]]
        board.append(s)
    
