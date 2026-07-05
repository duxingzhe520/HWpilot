class TaggedList:  #元素带标签的列表
    def __init__(self, ls1, ls2):
        dic = {}
        for i in range(len(ls1)):
            dic[ls2[i]] = ls1[i]
        self.dic = dic
        self.len = len(ls1)
        self.key = ls2
        self.value = ls1
    def __str__(self):
        s = ""
        for k, v in self.dic.items():
            s += "{0}:{1},".format(k, v)
        return s
    def __len__(self):
        return self.len
    def __getitem__(self, key):
        if key in self.dic:
            return self.dic[key]
        elif isinstance(key, int):
            return self.dic[self.key[key]]
    def __setitem__(self, key, value):
        if key in self.dic:
            self.dic[key] = value
        elif isinstance(key, int):
            self.dic[self.key[key]] = value
            # self.value[key] = value
        return value
    def __contains__(self, item):
        return item in self.value
            

a = TaggedList([70, 80, 90, 100], ["语文", "数学", "英语", "物理"])
print(len(a),78 in a, 80 in a) 
print(str(a))
print(a[0],a['数学'])
a[1] = a['物理'] = 85
print(a)