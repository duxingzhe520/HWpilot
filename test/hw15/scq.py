def times(n):
	class gen:
		def __init__(self):
			self.n = n
			self.ele = 0
		def __next__(self):
			self.ele += self.n
			return self.ele - self.n
		def __iter__(self):
			return self
	return gen()
			

n,m = map(int, input().split())
seq = times(n)
if str(type(seq) == "<class 'generator'>"):
	i = 0
	for x in seq:
		print(x)
		i += 1
		if i == m:
			break