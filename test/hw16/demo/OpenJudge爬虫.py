import asyncio
import pyppeteer as pyp
async def antiAntiCrawler(page): #为page添加反反爬虫手段
	await page.setUserAgent('Mozilla/5.0 (Windows NT 6.1; \
		Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) \
    		Chrome/78.0.3904.70 Safari/537.36')
	await page.evaluateOnNewDocument(
		'() =>{ Object.defineProperties(navigator, \
		{ webdriver:{ get: () => false } }) }')
async def getOjSourceCode(loginUrl):
    width, height = 1400, 800  #网页宽高
    browser = await pyp.launch(headless=False,
                               executablePath = "C:/Program Files/Google/Chrome/Application/chrome.exe",
                               userdataDir = "C:/tmp",
                               args=[f'--window-size={width},{height}'])
    page = await browser.newPage()
    await antiAntiCrawler(page)
    await page.setViewport({'width': width, 'height': height})
    await page.goto(loginUrl)
    #若手动登录，则以下若干行可以去掉
    #element = await page.querySelector("#email")  #寻找帐号输入框
    #await element.type("XXXX@pku.edu.cn")  		#输入帐号（邮箱）
    #element = await page.querySelector("#password") #寻找密码输入框
    #await element.type("XXXX")  			#输入密码
    #element = await page.querySelector(
    #          "#main > form > div.user-login > p:nth-child(2) > button")
    #await element.click()  					#点击登录按钮
    #若手动登录，则以上若干行可以去掉
    # OpenJudge 登录成功的一瞬间，网页会发生一次由服务器端控制的强制重定向（Redirect）跳转
    # 不要去抓取过渡状态的 #main > h2，而是直接等待登录成功后跳转到的最终新页面的独有元素
    # 使用 waitForNavigation 监听，直到网络完全安静下来（说明登录跳转彻底结束了）
    await page.waitForNavigation({'waitUntil': 'networkidle0', 'timeout': 0})
    await page.waitForSelector("#main>h2",
            timeout=30000)   #等待"正在进行的比赛...."标题出现
    element = await page.querySelector("#userMenu>li:nth-child(2)>a")
    #找"个人首页”链接
    await element.click()       		#点击个人首页链接
    await page.waitForNavigation()  	#等新网页装入完毕
    elements = await page.querySelectorAll(".result-right")
    #找所有"Accepted"链接, 其有属性 class="result-right"
    page2 = await browser.newPage() 	#新开一个页面 (标签)
    await antiAntiCrawler(page2)
    for element in elements[:2]: 	#只打印前两个程序
        obj = await element.getProperty("href") #获取href属性
        url = await obj.jsonValue()
        await page2.goto(url)      #在新页面(标签)中装入新网页
        element = await page2.querySelector("pre") #查找pre tag
        obj = await element.getProperty("innerText")
        text = await obj.jsonValue()
        print(text)
        print("-------------------------")
    await browser.close()
def main():
    url = "http://openjudge.cn/auth/login/"
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(getOjSourceCode(url))
main()
