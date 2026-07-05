import requests
import re

'''
运行方法是 python search.py > results.txt
需要预先 pip install requests
虚拟环境说明详见 requirements.txt
（虽然好像基本没用到）
'''

def get_url(word):
    url = "https://cn.bing.com/dict/search?q=" + word + "&qs=n&form=Z9LH5&sp=-1&lq=0&pq=new&sc=3-6&sk=&cvid=8E1EE733C2B743738C0745D600A7F22E"
    return url

def get_html(url):
    fakeHeaders = {
        "User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
                    "AppleWebKit/537.36 (KHTML, like Gecko) "
                    "Chrome/125.0.0.0 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
        "Accept-Language": "zh-CN,zh;q=0.9,en;q=0.8",
        "Connection": "keep-alive"
    }

    try:
        r = requests.get(url, headers = fakeHeaders)
        r.encoding = r.apparent_encoding
        return r.text
    except Exception as e:
        print(f"error! {e}")
        return None

def get_same_words(html):
    synoid = re.findall(r'<div id="synoid".*?</div>\s*</div>', html)

    if synoid:
        return re.findall(r'b_alink">(.*?)<', synoid[0])
    else:
        return None

def get_picture_url(html):
    url = re.search(r'img_area.*?src="([^"]*)"', html)
    if url:
        return url.group(1)
    return None

def download_picture(picture_url, word):
    if picture_url == None:
        return None
    r = requests.get(picture_url, stream=True)
    try:
        file = open(f"{word}.png", "wb")
        file.write(r.content)
        file.close()
    except Exception as e:
        print(f"error! {e}")

def process_single_word(word):
    download_picture(get_picture_url(get_html(get_url(word))), word)
    print('$', word, sep="")
    same_words = get_same_words(get_html(get_url(word)))
    if same_words:
        for same_word in same_words:
            print(same_word)

if __name__ == "__main__":
    # print(get_same_words(get_html(get_url("new"))))
    # download_picture(get_picture_url(get_html(get_url("new"))), "new")
    with open("words.txt", "r") as words_file:
        for line in words_file:
            word = line.strip()
            if word == "":
                continue
            process_single_word(word)
