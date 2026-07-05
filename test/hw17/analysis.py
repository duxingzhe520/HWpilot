import pandas as pd
pd.set_option('display.unicode.east_asian_width', True)

def open_excel(name):
    try:
        table = pd.read_excel(name, sheet_name=0, index_col=0)
        # print(table)
        return table
    except Exception as e:
        print(f"error! {e}")
        return None
    
def date_string(day):
    if not isinstance(day, int):
        print(f"{day} is not a number!")
    if day <= 0 or day >= 32:
        print(f"{day} out of range!")
    if day < 10:
        return '2019-03-0' + str(day)
    return '2019-03-' +  str(day)

def date_num(day):
    if not isinstance(day, int):
        print(f"{day} is not a number!")
    if day <= 0 or day >= 32:
        print(f"{day} out of range!")
    num_dict = {
        0:'Sunday',
        1:'Monday',
        2:'Tuesday',
        3:'Wednesday',
        4:'Thursday',
        5:'Friday',
        6:'Saturday'
        }
    return num_dict[(day + 5 - 1) % 7]

def cal_money_per_day(table):
    dict = {}
    for i in range(1, 32):
        s = table.loc[table['日期'] == date_string(i), '交易额'].sum()
        dict[i] = s
    return dict

def print_answer(dict):
    if dict:
        min1, min2, min3 = 0x3f3f3f3f, 0x3f3f3f3f, 0x3f3f3f3f
        i1, i2, i3 = 0, 0, 0
        for i in range(1, 32):
            if dict[i] < min1:
                min1, min2, min3 = dict[i], min1, min2
                i1, i2, i3 = i, i1, i2
            elif dict[i] < min2:
                min2, min3 = dict[i], min2
                i2, i3 = i, i2
            elif dict[i] < min3:
                min3 = dict[i]
                i3 = i

        print(date_string(i1), dict[i1], date_num(i1))
        print(date_string(i2), dict[i2], date_num(i2))
        print(date_string(i3), dict[i3], date_num(i3))

if __name__ == "__main__":
    print_answer(cal_money_per_day(open_excel('finance.xlsx')))
