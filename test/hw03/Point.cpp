#include <iostream> 
using namespace std;

class Point { 
	private: 
		int x; 
		int y; 
	public: 
		Point() {}
        friend ostream& operator << (ostream& o, Point& p);
        friend istream& operator >> (istream& o, Point& p);
// 在此处补充你的代码
}; 

ostream& operator << (ostream& o, Point& p) {
    o << p.x << ',' << p.y;
    return o;
}

istream& operator >> (istream& o, Point& p) {
    o >> p.x >> p.y;
    return o;
}

int main() 
{ 
 	Point p;
 	while(cin >> p) {
 		cout << p << endl;
	 }
	return 0;
}