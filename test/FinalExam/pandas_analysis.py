import pandas as pd

files = []

def open_files():
    for i in range(1, 101):
        files.append(pd.read_csv("./pandas_data_v2/input/input_" + str(i) + ".csv"))
        files[i - 1] = files[i - 1].sort_values("id").reset_index(drop=True)

def max_r(n):
    assert 0 <= n < 500
    return min(n, 500 - n - 1)

def single_file(n):
    assert 1 <= n <= 100
    df = files[n - 1]
    print("Test Case:", n)

    prices = df['price'].astype(float).to_numpy()
    prefix_sum = pd.Series(prices).cumsum().to_list()
    prefix = [0] + prefix_sum

    for i in range(500):
        r = max_r(i)
        for j in range(1, r + 1):
            inf = i - j
            sup = i + j
            avg = (prefix[sup + 1] - prefix[inf]) / (2 * j + 1)
            if round(avg, 2) == round(df.loc[i, 'price'], 2):
                print(df.loc[i, 'id'], df.loc[i, 'name'], sep="  ")
                break

if __name__ == "__main__":
    open_files()
    for i in range(1, 101):
        single_file(i)
