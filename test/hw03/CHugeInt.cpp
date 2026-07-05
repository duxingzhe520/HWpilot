#include <iostream> 
#include <cstring> 
#include <cstdlib> 
#include <cstdio> 
using namespace std;

const int MAX = 110; 

class CHugeInt {
    //倒序存储，有效范围是0 <= index < scale
    int* value;
    int scale;

    public:
        CHugeInt(int n) {
            if (n < 0) {
                value = NULL;
                scale = 0;
                return;
            }
            value = new int[202]();
            if (n == 0) {
                scale = 1;
                return;
            }
            int tmp_pos = 0;
            while (n > 0) {
                value[tmp_pos] = n % 10;
                tmp_pos += 1;
                n = n / 10;
            }
            scale = tmp_pos;
        }

        CHugeInt(char* s) {
            value = new int[202]();
            scale = strlen(s);
            for (int i = 0; i < scale; ++i) {
                value[i] = s[scale - 1 - i] - '0';
            }
        }

        CHugeInt(const CHugeInt & n) {
            value = new int[202]();
            for (int i = 0; i < n.scale; ++i) {
                value[i] = n.value[i];
            }
            scale = n.scale;
        }

        ~CHugeInt() {
            delete[] value;
        }

        CHugeInt& operator = (const CHugeInt& b) {
            for (int i = 0; i < b.scale; ++i) {
                value[i] = b.value[i];
            }
            scale = b.scale;
            return *this;
        }

        CHugeInt operator + (CHugeInt b) {
            CHugeInt sum(0);
            int len = max(scale, b.scale);      
            for (int i = 0; i < len; ++i) {
                sum.value[i] += (value[i] + b.value[i]);
                if (sum.value[i] >= 10) {
                    sum.value[i + 1] += sum.value[i] / 10;
                    sum.value[i] %= 10;
                }
            }
            if (sum.value[len] > 0) {
                sum.scale = len + 1;
            } else {
                sum.scale = len;
            }
            return sum;
        }

        CHugeInt& operator += (int n) {
            CHugeInt tmp(n);
            *this = *this + tmp;
            return *this;
        }

        CHugeInt operator ++ (int n) {
            CHugeInt tmp(*this);
            value[0] += 1;
            for (int i = 0; i < scale; ++i) {
                if (value[i] >= 10) {
                    value[i + 1] += value[i] / 10;
                    value[i] %= 10;
                }
            }
            if (value[scale] > 0) {
                scale += 1;
            }
            return tmp;
        }

        CHugeInt& operator ++ () {
            value[0] += 1;
            for (int i = 0; i < scale; ++i) {
                if (value[i] >= 10) {
                    value[i + 1] += value[i] / 10;
                    value[i] %= 10;
                }
            }
            if (value[scale] > 0) {
                scale += 1;
            }
            return *this;
        }

        friend CHugeInt operator + (int n, const CHugeInt& a);
        friend ostream& operator << (ostream& o, const CHugeInt& a);
};

CHugeInt operator + (int n, const CHugeInt& a) {
    CHugeInt sum(a);
    return sum + n;
}

ostream& operator << (ostream& o, const CHugeInt& a) {
    for (int i = 0; i < a.scale; ++i) {
        o << a.value[a.scale - 1 - i];
    }
    return o;
}

int main() { 
	char s[210];
	int n;

	while (cin >> s >> n) {
		CHugeInt a(s);
		CHugeInt b(n);

		cout << a + b << endl;
		cout << n + a << endl;
		cout << b << endl;
		b += n;
		cout  << ++b << endl;
		cout << b++ << endl;
		cout << b << endl;
	}
	return 0;
}