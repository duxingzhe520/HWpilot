#include <iostream>
#include <string>
using namespace std;

template<class T, class T2>
void MyForeach(T* start, T* end, T2 f) {
    T* tmp = start;
    while (tmp != end) {
        f(*tmp);
        tmp += 1;
    }
}

void Print(string s){
	cout << s;
}
void Inc(int & n){
	++ n;
}

string array0[100];
int a[100];
int main() {
	int m,n;
	while(cin >> m >> n) {
		for(int i = 0;i < m; ++i)
			cin >> array0[i];
		for(int j = 0; j < n; ++j)
			cin >> a[j];
		MyForeach(array0, array0+m, Print);		 
		cout << endl;
		MyForeach(a, a+n, Inc);		 
		for(int i = 0;i < n; ++i)
			cout << a[i] << ",";
		cout << endl;
	}
	return 0;
}