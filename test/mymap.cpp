#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>
#include <map>
using namespace std;

class MyMap{
public:
    multimap<int, string, greater<int>> info;
};

ostream& operator << (ostream& o, MyMap& m) {
    auto it = m.info.begin();
    while (it != m.info.end()) {
        auto range = m.info.equal_range(it->first);
        vector<string> names;
        for (auto tmp = range.first; tmp != range.second; ++tmp) {
            names.push_back(tmp->second);
        }
        sort(names.begin(), names.end());
        o << it->first;
        for (string name : names) {
            o << " " << name;
        }
        o << "\n";
        it = range.second;
    }
    return o;
}

istream& operator >> (istream& i, MyMap& m) {
    string name;
    int score;
    i >> name >> score;

    bool has_exist = false;
    auto range = m.info.equal_range(score);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == name) {
            has_exist = true;
        }
    }

    if (!has_exist) {
        m.info.insert(make_pair(score, name));
    }
    return i;
}


int main()
{
    int n;
    cin >> n;
    MyMap mm;
    for (int i = 0; i < n; ++i)
        cin >> mm;
    cout << mm;
    return 0; 
}