class Student:
    id = 0
    def __init__(self, _name, _score):
        self.name = _name
        self.score = _score
        self.id = Student.id
        Student.id += 1
    def __gt__(self, other):
        if isinstance(other, Student):
            return self.score > other.score
        if isinstance(other, int):
            return self.score > other
        return False
    def __hash__(self):
        return self.id
    
s1_data = input().split()
s2_data = input().split()
compare_num = int(input().strip())

s1 = Student(s1_data[0], int(s1_data[1]))
s2 = Student(s2_data[0], int(s2_data[1]))

# 创建字典
grade_dict = {s1: "A", s2: "B"}

print(grade_dict[s1])
print(s1 > s2)
print(s1 > compare_num)