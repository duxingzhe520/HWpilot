#include <iostream>
#include <cstring>
using namespace std;

class Array2 {
    int* array;
    int h;
    int w;

    public:
        Array2() {
            array = new int[1];
            h = 0;
            w = 0;
        }

        Array2(int w, int h) {
            this->h = h;
            this->w = w;
            array = new int[h * w + 1];
        }

        ~Array2() {
            delete[] array;
        }

        int* operator [] (int i) {
            return array + i * h;
        }

        int operator () (int i, int j) {
            return array[i * h + j];
        }

        Array2& operator = (const Array2 & other) {
            if (array == other.array) {
                return *this;
            }
            delete[] this->array;
            array = new int[other.h * other.w + 1];
            for (int i = 0; i <other.h * other.w; ++i) {
                array[i] = other.array[i];
            }
            h = other.h;
            w = other.w;
            return *this;
        }
};

int main() {
    Array2 a(3,4);
    int i,j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 4; j++ )
            a[i][j] = i * 4 + j;
    for (i = 0; i < 3; ++i) {
        for(j = 0; j < 4; j++) {
            cout << a(i, j) << ",";
        }
        cout << endl;
    }
    cout << "next" << endl;
    Array2 b;     b = a;
    for(  i = 0;i < 3; ++i ) {
        for(  j = 0; j < 4; j ++ ) {
            cout << b[i][j] << ",";
        }
        cout << endl;
    }
    return 0;
}