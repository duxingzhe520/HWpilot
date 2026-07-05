#include <iostream>
#include <map>
using namespace std;

typedef multimap<int, int> mii;
typedef pair<multimap<int, int>::iterator, multimap<int, int>::iterator> pmii;
mii vip;

void regi(int id, int power) {
    auto range = vip.equal_range(power);
    int minID = 0x3f3f3f3f;
    if (range.first != range.second) {
        for (auto p = range.first; p != range.second; p++) {
            minID = min(minID, p->second);
        }
    } else {
        auto p = vip.lower_bound(power);
        pmii rangep;
        if (p == vip.begin()) {
            rangep = vip.equal_range(p->first);
        } else if (p == vip.end()) {
            --p;
            rangep = vip.equal_range(p->first);
        } else {
            auto range1 = vip.equal_range(p->first);
            --p;
            auto range2 = vip.equal_range(p->first);
            if (power - range2.first->first < range1.first->first - power) {
                rangep = range2;
            } else if (power - range2.first->first > range1.first->first - power) {
                rangep = range1;
            } else {
                rangep = make_pair(range2.first, range1.second);
            }
        }
        for (auto pp = rangep.first; pp != rangep.second; ++pp) {
            minID = min(minID, pp->second);
        }
    }
    cout << id << " " << minID << endl;
    vip.insert(make_pair(power, id));
}

int main() {
    vip.insert(make_pair(1000000000, 1));
    int n;
    cin >> n;
    while (n-- > 0) {
        int id, power;
        cin >> id >> power;
        regi(id, power);
    }
    return 0;
}