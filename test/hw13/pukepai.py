def apart(str):
    return str[0], str[1:]

color_dict = {"h":3, "s":2, "d":1, "c":0}
num_dict = {"2":15, "A":14, "K":13, "Q":12, "J":11}
for i in range(3, 11):
    num_dict[str(i)] = i

def compare(str):
    color, num = apart(str)
    return num_dict[num] * 10 + color_dict[color]

if __name__ == "__main__":
    try:
        while True:
            ls = input().split()
            ls.sort(key=compare, reverse=True)
            print(' '.join(ls))
    except EOFError:
        pass
