#include <queue>
#include <iostream>
#include <math.h>
using namespace std;

int num_prime(int n) {
    if (n == 1 || n ==2 || n == 3) {
        return 0;
    }
    int ret = 0;
    int n_origin = n;
    if (n % 2 == 0) {
        ret += 1;
        while (n % 2 == 0) {
            n /= 2;
        }
    }
    int sup = sqrt(n);
    for (int i = 3; i <= sup; i += 2) {
        if (n % i == 0) {
            ret += 1;
            while (n % i == 0) {
                n /= i;
            }
        }
    }
    if (n == n_origin) {
        return 0;
    }
    if (n > 1) {
       ret += 1;
    }
    return ret;
}

struct myCompareLess {
    bool operator () (const int& a, const int& b) {
        int num1 = num_prime(a);
        int num2 = num_prime(b);
        if (num1 < num2) {
            return true;
        } else if (num1 > num2) {
            return false;
        } else {
            return a < b;
        }
    }
}; 

struct myCompareGreater {
    bool operator () (const int& a, const int& b) {
        int num1 = num_prime(a);
        int num2 = num_prime(b);
        if (num1 < num2) {
            return false;
        } else if (num1 > num2) {
            return true;
        } else {
            return a > b;
        }
    }
};

priority_queue<int, vector<int>, myCompareLess> q1;
priority_queue<int, vector<int>, myCompareGreater> q2;


int main() {
    int num;
    cin >> num;
    while (num-- > 0) {
        for (int i = 0; i < 10; ++i) {
            int x;
            cin >> x;
            q1.push(x); q2.push(x);
        }
        cout << q1.top() << " " << q2.top() << endl;
        q1.pop(); q2.pop();
    }
    return 0;
}