#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
using namespace std;

template <class T1, class T2>
struct Closer
{
    T2 cmp_f;
    T1 cmp_std;
    Closer(T1 t, T2 f) {
        cmp_f = f;
        cmp_std = t;
    }

    bool operator () (const T1& t1, const T1& t2) {
        int cmp1 = cmp_f(t1, cmp_std);
        int cmp2 = cmp_f(t2, cmp_std);
        if (cmp1 < cmp2) {
            return true;
        } else if (cmp1 > cmp2) {
            return false;
        } else {
            return t1 < t2;
        }
    }
};

int Distance1(int n1, int n2)
{
    return abs(n1 - n2);
}

int Distance2(const string &s1, const string &s2)
{
    return abs((int)s1.length() - (int)s2.length());
}

int a[10] = {0, 3, 1, 4, 7, 9, 20, 8, 10, 15};

string b[6] = {"American", "Jack", "To", "Peking", "abcdefghijklmnop", "123456789"};

int main()
{
    int n;
    string s;
    while (cin >> n >> s)
    {
        sort(a, a + 10, Closer<int, int (*)(int, int)>(n, Distance1));
        for (int i = 0; i < 10; ++i)
            cout << a[i] << ",";
        cout << endl;
        sort(b, b + 6, Closer<string, int (*)(const string &, const string &)>(s, Distance2));
        for (int i = 0; i < 6; ++i)
            cout << b[i] << ",";
        cout << endl;
    }
    return 0;
}