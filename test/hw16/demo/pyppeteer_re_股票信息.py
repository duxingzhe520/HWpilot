#用pyppeteer + 正则表达式爬取股票网页
import asyncio  # Python 3.6之后自带的协程库
import pyppeteer as pyp
import re
def getHtml(url):
	import asyncio
	import pyppeteer as pyp
	async def antiAntiCrawler(page):
		# 为page添加反反爬虫手段
		await page.setUserAgent('Mozilla/5.0 (Windows NT 6.1; Win64; x64) \
	    					AppleWebKit/537.36 (KHTML, like Gecko) \
	    					Chrome/78.0.3904.70 Safari/537.36')
		await page.evaluateOnNewDocument(
			'() =>{ Object.defineProperties(navigator, \
			{ webdriver:{ get: () => false } }) }')
	async def asGetHtml(url):
		browser = await pyp.launch(headless=False,
							 userDataDir='C:/Users/pkuwi/code/tutorial_python/tmp',  
							 executablePath = "C:/Program Files/Google/Chrome/Application/chrome.exe")
		page = await browser.newPage()
		await antiAntiCrawler(page)
		await page.goto(url)
		text = await page.content()
		await browser.close()
		return text
	loop = asyncio.new_event_loop()
	asyncio.set_event_loop(loop)
	return  loop.run_until_complete(asGetHtml(url))

html = getHtml("https://quote.eastmoney.com/sh600000.html")#pypeteer版
#print(html) 下一步编程前可以先打印出来，将打印结果拷贝粘贴到记事本，查找关心的数据如12.17在哪里
pt = r'<td class="n">([^<]*)</td><td><span><span class[^<]*>([^<]*)</span></span></td>'
#正则表达式pt对应找到的关心的数据所在字符串的模式:
#<td class="n">今开：</td><td><span><span class="price_down blinkgreen">12.71</span></span></td>
for x in re.findall(pt, html, re.DOTALL):
    if (x[1] != ""):
        print(x[0], x[1])

