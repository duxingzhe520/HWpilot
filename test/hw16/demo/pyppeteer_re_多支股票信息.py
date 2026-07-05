#用pyppeteer + re跳转查询多支股票网页，并爬取股票信息
import asyncio  # Python 3.6之后自带的协程库
import pyppeteer as pyp
import re
async def getStockCodes(page):
    #从"https://quote.eastmoney.com/stocklist.Html"对应的page获取所有股票名称和代码
    codes = []
    #最终内容：["四川路桥(600039)","包钢股份(600010)"......]
    html = await page.content()
    #print(html)
    pt = r'<a target="_blank" href="http://quote.eastmoney.com/[^"]*">([^<]*\(\d+\))</a>'
    #对应 <li><a target="_blank" href="http://quote.eastmoney.com/sh600000.html">浦发银行(600000)</a></li>
    for x in re.findall(pt, html):
        codes.append(x)
    return codes

async def getStockInfo(url):
    browser = await pyp.launch(headless=False,
                            userDataDir='C:/Users/pkuwi/code/tutorial_python/tmp',  
                            executablePath = "C:/Program Files/Google/Chrome/Application/chrome.exe")
    # 启动Chromium,browser即为Chromium浏览器，非隐藏启动
    page = await browser.newPage()# 在浏览器中打开一个新页面（标签）
    await page.setUserAgent('Mozilla/5.0 (Windows NT 10.0; Win64; x64)  \
        AppleWebKit/537.36 (KHTML, like Gecko)  \
        Chrome/81.0.4044.138 Safari/537.36') #反反爬措施
    await page.evaluateOnNewDocument(
    '() =>{ Object.defineProperties(navigator, \
            { webdriver:{ get: () => false } }) }' ) #反反爬措施
    await page.goto(url) # 装入url对应的网页
    codes = await getStockCodes(page)
    for x in codes: #只取前2个股票信息
        pos1, pos2 = x.index("("), x.index(")")
        code =x[pos1 + 1:pos2] #取股票代码,如600000
        if int(code) < 600006 or int(code) > 600007:
            continue
        print("-----",x) #x形如 "浦发银行(600000)"
        url = "https://quote.eastmoney.com/sh" + code + ".html"
        await page.goto(url)
        html = await page.content() #往下编程前可以先print(html)看一看
        pt = r'<td class="n">([^<]*)</td><td><span><span class[^<]*>([^<]*)</span></span></td>'
        # 正则表达式pt对应找到的关心的数据所在字符串的模式:
        # <td class="n">今开：</td><td><span><span class="price_up blinkred">11.73</span></span></td>
        for x in re.findall(pt, html, re.DOTALL):
            if (x[1] != ""):
                print(x[0], x[1])
    await browser.close() #关闭浏览器
    
url = "https://quote.eastmoney.com/stocklist.Html"
loop = asyncio.get_event_loop()
loop.run_until_complete(getStockInfo(url))