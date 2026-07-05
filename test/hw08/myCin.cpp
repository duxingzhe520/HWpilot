#include <iostream>
using namespace std;

class MyCin {
    bool valid;
public :
    MyCin() {
        valid = true;
    }

    MyCin& operator >> (int& n) {
        if (!valid) {
            // cout << "case 01" << endl;
            return *this;
        }
        cin >> n;
        if (n == -1) {
            // cout << "case 02" << endl;
            valid = false;
            return *this;
        }
        // cout << "case 03" << endl;
        return *this;
    }

    operator bool () {
        return valid;
    }
};

int main()
{
    MyCin m;
    int n1, n2;
    while (m >> n1 >> n2) 
        cout  << n1 << " " << n2 << endl;
    return 0;
}