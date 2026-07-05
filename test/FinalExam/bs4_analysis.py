import bs4

soup = []
star_rating = {
    'One':1,
    'Two':2,
    'Three':3,
    'Four':4,
    'Five':5
               }

def open_pages():
    for i in range(1, 21):
        name = "./data/Book_Page" + str(i) + ".html"

        soup.append(bs4.BeautifulSoup(open(name, encoding="utf-8"), "html.parser"))

def cal_prices():
    total = 0.
    for i in range(1, 21):
        for diva in soup[i - 1].find_all("div", attrs={"class":"product_price"}):
            if diva != None:
                for x in diva.find_all("p", attrs={"class":"price_color"}):
                    total += float((x.text)[1:])
                    # print(f"{i}'s price found, is {float((x.text)[1:])}. Total is {total}")
    return total

def cal_stars():
    total = 0
    for i in range(20):
        for x in soup[i].find_all("p", attrs={"class":"star-rating"}):
            total += star_rating[x['class'][1]]
    return total

def find_longest_name():
    max_len = 0
    name = None
    for i in range(20):
        for x in soup[i].find_all("div",attrs={"image_container"}):
            new_name = x.find('a').find('img')['alt']
            if len(new_name) > max_len:
                max_len = len(new_name)
                name = new_name
    return name

if __name__ == "__main__":
    open_pages()
    print(f"{cal_prices():.2f}")
    print(cal_stars())
    print(f"{find_longest_name()}")
