#include <iostream>
#include <string>
#include <cstring>
using namespace std;

class MyString {
	char* p;
public:
	MyString(const char * s) {
		if (s) {
			p = new char[strlen(s) + 1];
			strcpy(p, s);
		}
		else{
            p = NULL;
        }
	}

	~MyString() {
        if (p) {
            delete [] p; 
        }
    }

    MyString(MyString& other) {
        p = new char[strlen(other.p) + 1];
        strcpy(p, other.p);
    }

    void Copy(char* s) {
        delete []p;
        p = new char[strlen(s) + 1];
        strcpy(p, s);
    }

    MyString& operator = (MyString other) {
        delete []p;
        p = new char[strlen(other.p) + 1];
        strcpy(p, other.p);
        return *this;
    }

    MyString& operator = (char* c) {
        delete []p;
        p = new char[strlen(c) + 1];
        strcpy(p, c);
        return *this;
    }

    friend ostream& operator << (ostream& o, const MyString& s);

};

ostream& operator << (ostream& o, const MyString& s) {
    o << s.p;
    return o;
};

int main()
{
	char w1[200], w2[100];
	while (cin >> w1 >> w2) {
		MyString s1(w1), s2 = s1;
		MyString s3(NULL);
		s3.Copy(w1);
		cout << s1 << "," << s2 << "," << s3 << endl;

		s2 = w2;
		s3 = s2;
		s1 = s3;
		cout << s1 << "," << s2 << "," << s3 << endl;
		
	}
}