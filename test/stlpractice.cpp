#include <stack>
#include <queue>
#include <map>
#include <iostream>
#include <memory>
using namespace std;

template<class T1, class T2>
auto add(T1 x, T2 y) -> decltype(x + y) {
    return x + y;
}

int main() {
    // stack<int> s;
    // s.push(1);
    // s.pop();
    // int x = s.size();
    // cout << "x = " << x << "\n";
    // s.push(2);
    // s.push(3);
    // int y = s.top();
    // cout << "y = " << y << "\n";
    // s.top() = 5;
    // cout << s.top() << "\n";

    // queue<int> q;
    // queue<int> q2;
    // q2.push(100);
    // q.push(1);
    // q.push(2);
    // q.push(3);
    // int x = q.front();
    // int y = q.back();
    // cout << "x = " << x << "\n";
    // cout << "y = " << y << "\n";
    // q.pop();
    // cout << q.front() << "\n";
    // q.swap(q2);
    // cout << q.front() << "\n";

    // map<int, int> m;
    // m.emplace(1, 2);
    // m.emplace(3, 4);
    // m[1] = 5;
    // int y = m.erase(3);
    // cout << m[1] << "\n";
    // cout << y << "\n";
    // cout << m.erase(100) << "\n";

    shared_ptr<int> p(new int[10]);
    *p = 1;
    

    
    return 0;
}