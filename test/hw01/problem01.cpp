#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <cstdlib>
using namespace std;

class Student {
    string name;
    int age;
    int stuNum;
    int score[4];
    double scoreAll;
    char comma;

    public:
    void input() {
        getline(cin, name, ',');
        cin >> age >> comma >> stuNum >> comma >> score[0] >> comma >> score[1] >> comma >> score[2] >> comma >> score[3];
    }

    void calculate() {
        scoreAll = (score[0] + score[1] + score[2] + score[3]) / 4.0;
    }

    void output() {
        cout << name << ',' << age << ',' << stuNum << ',' << scoreAll;
    }
};

int main() {
	Student student;        // 定义类的对象
	student.input();        // 输入数据
	student.calculate();    // 计算平均成绩
	student.output();       // 输出数据
}