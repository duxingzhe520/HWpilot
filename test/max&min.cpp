#include <iostream>
#include <vector>
using namespace std;

template<class Iter, class Comp = greater<int>>
int getWanted(Iter begin, Iter end, Comp comp = Comp()) {
    int min_val = *begin;
    for (auto it = begin; it != end; ++it) {
        if (!comp(*it, min_val)) {
            min_val = *it;
        }
    }
    return min_val;
}

struct myComp {
	bool operator ()(int a,int b) {
		return a % 10 < b % 10;
	}
};

int main()
{
	int n;
	cin >> n;
	vector<int> a,b,c;
	for(int i=0;i<n;++i) {
		int x;
		cin >> x;
		a.push_back(x);
	}
	int cmd;
	while( cin >> cmd ) {
		switch(cmd) {
			case 0:
				cout << getWanted<vector<int>::iterator,less<int>>(a.begin(),a.end()) << endl;
				break;
			case 1:
				cout << getWanted<vector<int>::iterator>(a.begin(),a.end(), less<int>()) << endl;	
				break;
			case 2:
				cout << getWanted<vector<int>::iterator>(a.begin(),a.end(),myComp()) << endl;
				break;
			case 3:
				cout << getWanted<vector<int>::iterator>(a.begin(),a.end()) << endl; 
				break;
		}
	}
	return 0;
}