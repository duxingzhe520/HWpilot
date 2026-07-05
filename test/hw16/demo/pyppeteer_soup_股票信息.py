#用pyppeteer + beautifulsoup爬取股票网页
import asyncio  # Python 3.6之后自带的协程库
import pyppeteer as pyp
import re
import bs4
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

html = getHtml("https://quote.eastmoney.com/sh600000.html")
#要用pypeteer版的getHtml
#print(html) #下一步编程前可以先打印出来，将打印结果拷贝粘贴到记事本，查找关心的数据如12.17在哪里
soup = bs4.BeautifulSoup(html,"html.parser")
diva = soup.find("div",attrs={"class":"brief_info_c"})
titles = diva.find_all("td",attrs = {"class":"n"})
for t in titles:
	sp = t.findNext("span")
	if 'class' not in sp.attrs:
		print(t.text,sp.text)
	#print(sp) 可以看到sp的整个tag的描述
