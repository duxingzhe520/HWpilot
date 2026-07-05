#用pyppeteer和浏览器自身的查找元素功能爬取股票网页
import asyncio  # Python 3.6之后自带的协程库
import pyppeteer as pyp
import re
async def antiAntiCrawler(page):
	# 为page添加反反爬虫手段
	await page.setUserAgent('Mozilla/5.0 (Windows NT 6.1; Win64; x64) \
						AppleWebKit/537.36 (KHTML, like Gecko) \
						Chrome/78.0.3904.70 Safari/537.36')
	await page.evaluateOnNewDocument(
		'() =>{ Object.defineProperties(navigator, \
		{ webdriver:{ get: () => false } }) }')
async def asGetStockInfo(url):
	browser = await pyp.launch(headless=False,
							 userDataDir='C:/Users/pkuwi/code/tutorial_python/tmp',  
							 executablePath = "C:/Program Files/Google/Chrome/Application/chrome.exe")
	page = await browser.newPage()
	await antiAntiCrawler(page)
	await page.goto(url)
	#html = await page.content()
	#print(html)
	for k in range(1,3):
		for i in range(1,14,2):
			selector1 = '#app > div > div > div.zsquote3l.zs_brief ' + \
						'> div.quote3l_c > div > table ' + \
						'> tbody > tr:nth-child(%d) > td:nth-child(%d)' % (k,i)
			selector2 = ('#app > div > div > div.zsquote3l.zs_brief ' + \
						'> div.quote3l_c > div > table > tbody > tr:nth-child(%d) ' + \
						'> td:nth-child(%d) > span > span') % (k,i+1)
			title = await page.querySelector(selector1)
			if title is None:
				break
			else:
				value = await page.querySelector(selector2)
			#obj = await title.getProperty("text")  # 还可以 a.getProperty("href")
			# 上面这句不行就改成:
			obj = await title.getProperty("innerText")
			text = await obj.jsonValue()  # 固定写法
			print(text,end = " ")
			obj = await value.getProperty("innerText")
			text = await obj.jsonValue()  # 固定写法
			print(text)
	await browser.close()

loop = asyncio.new_event_loop()
asyncio.set_event_loop(loop)
url = "https://quote.eastmoney.com/sh600000.html"
loop.run_until_complete(asGetStockInfo(url))
