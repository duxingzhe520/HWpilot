#include <map>
#include <set>
#include <iostream>
#include <string>
using namespace std;

typedef map<int, multiset<int>> mim;
mim data0;

void ourNew(int id) {
    if (data0.find(id) == data0.end()) {
        data0[id] = multiset<int>();
    }
}

void ourAdd(int id, int val) {
    mim::iterator p = data0.find(id);
    if (p != data0.end()) {
        data0[id].insert(val);
    }
}

void ourOut(int id) {
    mim::iterator p0 = data0.find(id);
    if (p0 == data0.end()) {
        return;
    }
    multiset<int>::iterator p;
    for (p = data0[id].begin(); p != data0[id].end(); ++p) {
        cout << *p << " ";
    }
    cout << endl;
}

void ourMerge(int id1, int id2) {
    if (id1 == id2) {
        return;
    }
    mim::iterator p1 = data0.find(id1);
    mim::iterator p2 = data0.find(id2);
    if (p1 == data0.end() || p2 == data0.end()) {
        return;
    }
    data0[id1].insert(data0[id2].begin(), data0[id2].end());
    data0[id2].clear();
}

void ourUnique(int id) {
    mim::iterator p0 = data0.find(id);
    if (p0 == data0.end()) {
        return;
    }
    set<int> tmp(data0[id].begin(), data0[id].end());
    data0[id].clear();
    data0[id].insert(tmp.begin(), tmp.end());
}

int main() {
    int n;
    cin >> n;
    while (n-- > 0) {
        string cmd;
        cin >> cmd;
        int n1, n2;
        if (cmd == "new") {
            cin >> n1;
            ourNew(n1);
        } else if (cmd == "add") {
            cin >> n1 >> n2;
            ourAdd(n1, n2);
        } else if (cmd == "out") {
            cin >> n1;
            ourOut(n1);
        } else if (cmd == "merge") {
            cin >> n1 >> n2;
            ourMerge(n1, n2);
        } else if (cmd == "unique") {
            cin >> n1;
            ourUnique(n1);
        }
    }  
    return 0;
}