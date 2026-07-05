def my_print(s):
    tmp = []
    for sg_pk in s:
        if len(tmp) > 0:
            if tmp[0][1] == 'J':
                if sg_pk[1] == 'J':
                    tmp.append(sg_pk)
                else:
                    print(' '.join(tmp))
                    tmp.clear()
                    tmp.append(sg_pk)
            else:
                if sg_pk[0] == tmp[0][0]:
                    tmp.append(sg_pk)
                else:
                    print(' '.join(tmp))
                    tmp.clear()
                    tmp.append(sg_pk)
        else:
            tmp.append(sg_pk)
    if len(tmp) > 0:
        print(' '.join(tmp))
    print()

def my_sort(pks):
    # print(f"sorted({pks}) = {sorted(pks, key = compare, reverse=True)}")
    return sorted(pks, key = compare, reverse=True)

dic_num = {'B':17, 'L':16, '2':15, 'A':14, 'K':13, 'Q':12, 'J':11, 'T':10}
for i in range(3, 10):
    dic_num[str(i)] = i

dic_flower = {'S':4, 'H':3, 'C':2, 'D':1, 'J':0}

def compare(sg_pk):
    return dic_num[sg_pk[0]] * 1000 + dic_flower[sg_pk[1]]

if __name__ == "__main__":
        try:
            while True:
                pkp = input().split()
                my_print(my_sort(pkp))
        except EOFError:
            pass
