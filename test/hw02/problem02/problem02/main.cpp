//
//  main.cpp
//  problem02
//
//  Created by duxingzhe520 on 2026/3/6.
//

#include <iostream>
using namespace std;

class A {
public:
    int val;

    A(int a) {
        val = a;
    }
    
    A() {
        val = 123;
    }
    
    A& GetObj() {
        return *this;
    }
    
    A(const A & a) {
        this->val = a.val;
    }
};

int main()
{
    int m,n;
    A a;
    cout << a.val << endl;
    while(cin >> m >> n) {
        a.GetObj() = m;
        cout << a.val << endl;
        a.GetObj() = A(n);
        cout << a.val << endl;
    }
    return 0;
}
