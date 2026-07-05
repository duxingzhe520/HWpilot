#include <iostream>
#include <cstring>
using namespace std;

template <int bitNum>
struct MyBitset 
{
	char a[bitNum / 8 + 1];

	MyBitset() { memset(a, 0, sizeof(a)); };

	void Set(int i, int v) {
		char& c = a[i / 8];
		int bp = i % 8;
		if (v) 
			c |= (1 << bp);
		else 
			c &= ~(1 << bp);
	}

    struct BitRef {
        MyBitset& owner;
        int num_char, offset;

        BitRef(int num_char, int offset, MyBitset& owner) : num_char(num_char), offset(offset), owner(owner) {}

        BitRef& operator = (int val) {
            char& c = owner.a[num_char];
            if (val) {
                c |= (1 << offset);
            } else {
                c &= ~(1 << offset);
            }
            return *this;
        }

        BitRef& operator = (BitRef& other) {
            *this = static_cast<bool>(other);
            return *this;
        }

        operator bool () {
            return (owner.a[num_char] & (1 << offset)) != 0;
        }
    };

    BitRef operator [] (int i) {
        return BitRef(i / 8, i % 8, *this);
    }

    void Print() {
		for(int i = 0; i < bitNum; ++i) 
			cout << (*this)[i];
		cout << endl;
	}
};

int main()
{
	int n;
	int i, j, k, v;
	while (cin >> n) {
		MyBitset<20> bs;
		for (int i = 0; i < n; ++i) {
			int t;
			cin >> t;
			bs.Set(t, 1);
		}
		bs.Print();

		cin >> i >> j >> k >> v;
		bs[k] = v;
		bs[i] = bs[j] = bs[k];
		bs.Print();

		cin >> i >> j >> k >> v;
		bs[k] = v;
		(bs[i] = bs[j]) = bs[k];
		bs.Print();
	}
	return 0;
}