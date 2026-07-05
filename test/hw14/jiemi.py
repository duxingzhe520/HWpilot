def decoder(s, n):
    if n < 0:
        return None
    elif n == 1 or n == 2:
        return s
    return decoder(s[1:1+(n-1)//2], (n-1)//2) + s[0] + decoder(s[1+(n-1)//2:], n//2)

if __name__ == "__main__":
    s = input()
    n = len(s)
    print(decoder(s, n))
