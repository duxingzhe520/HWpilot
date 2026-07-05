#include <iostream>
using namespace std;

template<class F1, class F2, class T>
struct combine {
    F1 f1;
    F2 f2;
    combine(F1 f1, F2 f2) :f1(f1), f2(f2) {};
    T operator () (T x) {
        return f1(f1(x) + f2(x));
    }
};

int main(){
    auto Square = [] (double a) { return a * a; };
    auto Inc = [] (double a) { return a + 1; };
    cout << combine<decltype(Square),decltype(Inc),int>(Square,Inc)(3) << endl;
    cout << combine<decltype(Inc),decltype(Square),double>(Inc,Square)(2.5) << endl;

    return 0;
}