//
//  main.cpp
//  Problem03
//
//  Created by duxingzhe520 on 2026/3/8.
//

#include <iostream>
using namespace std;

class Sample {
    public:
        int v;
    // 在此处补充你的代码
    Sample() {
        v = NULL;
    }
    
    Sample(int n) {
        v = n;
    }
    
    Sample(const Sample & s) {
            this->v = s.v + 2;
    }
};

void PrintAndDouble(Sample o)
{
    cout << o.v;
    cout << endl;
}

int main()
{
    Sample a(5);
    Sample b = a;
    PrintAndDouble(b);
    Sample c = 20;
    PrintAndDouble(c);
    Sample d;
    d = a;
    cout << d.v;
    return 0;
}
