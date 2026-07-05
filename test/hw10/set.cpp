#include <iostream>
#include <set>
using namespace std;

typedef multiset<int> mi;
mi a;
set<int> history;

void Add(int m) {
    a.insert(m);
    history.insert(m);
    cout << a.count(m) << endl;
}

void Ask(int m) {
    mi::iterator p = history.find(m);
    if (p == history.end()) {
        cout << 0 << " " << 0 << endl;
        return;
    }
    cout << 1 << " " << a.count(m) << endl;
}

void Del(int m) {
    cout << a.count(m) << endl;
    a.erase(m);
}

int main() {
    int n;
    cin >> n;
    string cmd;
    int m;
    while(n-- > 0) {
        cin >> cmd >> m;
        if (cmd == "add") {
            Add(m);
        } else if (cmd == "ask") {
            Ask(m);
        } else if (cmd == "del") {
            Del(m);
        }
    }
    return 0;
}