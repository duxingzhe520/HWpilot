if __name__ == "__main__":
    n = int(input())
    stores = []
    for _ in range(n):
        stores.append(input())
    q = int(input())
    for _ in range(q):
        store = input()
        exist = False
        for all_store in stores:
            if all_store.startswith(store):
                exist = True
                break
        if exist:
            print("Yes")
        else:
            print("No")
