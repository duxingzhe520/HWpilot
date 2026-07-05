//
//  main.cpp
//  problem02
//
//  Created by duxingzhe520 on 2026/3/6.
//

#include <iostream>
#include <cstring>
#include <cstdlib>
using namespace std;
class Complex {
    private:
        double r,i;
    public:
        void Print() {
            cout << r << "+" << i << "i" << endl;
        }
    
        Complex() {
            r = 0;
            i = 0;
        }
        
        Complex(char c[]) {
            r = c[0] - '0';
            i = c[2] - '0';
        }
// 在此处补充你的代码
};
int main() {
    Complex a;
    a = "3+4i"; a.Print();
    a = "5+6i"; a.Print();
    return 0;
}
