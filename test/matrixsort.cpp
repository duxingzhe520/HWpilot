#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

class Mat{
	int h, w;
public:
	Mat(int height, int width) : h(height),w(width) {
        data = new int[h * w]();
        id = cnt;
        cnt ++;
    }

    static int cnt;

    Mat(const Mat& other) : h(other.h), w(other.w), id(other.id) {
        data = new int[h * w];
        for (int i = 0; i < h * w; ++i) {
            this->data[i] = other.data[i];
        }
    }

    Mat& operator = (const Mat& other) {
        if (this == &other) {
            return *this;
        }
        h = other.h;
        w = other.w;
        id = other.id;
        delete[] data;
        data = new int[h * w];
        for (int i = 0; i < h * w; ++i) {
            this->data[i] = other.data[i];
        }
        return *this;
    }

    int* data;
    int id;

    ~Mat() {
        delete[] data;
    }

    int& get(int i, int j) const {
        return data[i * w + j];
    }

    friend ostream& operator << (ostream& o, Mat& m) {
        for (int i = 0; i < m.h; ++i) {
            for (int j = 0; j < m.w; ++j) {
                o << m.get(i, j) << " ";
            }
            o << "\n";
        }
        return o;
    }

    friend istream& operator >> (istream& is, Mat& m) {
        for (int i = 0; i < m.h; ++i) {
            for (int j = 0; j < m.w; ++j) {
                is >> m.get(i, j);
            }
        }
        return is;
    }

    int sum() const {
        int ret = 0;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                ret += get(i, j);
            }
        }
        return ret;
    }

    friend bool operator < (const Mat& m1, const Mat& m2) {
        if (m1.h * m1.w < m2.h * m2.w) {
            return true;
        } else if (m1.h * m1.w > m2.h * m2.w) {
            return false;
        } else if (m1.id > m2.id) {
            return true;
        }
        return false;
    }
};

int Mat::cnt = 0;

bool comparator_1(const Mat& m1, const Mat& m2) {
    if (m1.sum() < m2.sum()) {
        return true;
    } else if (m1.sum() > m2.sum()) {
        return false;
    } else if (m1.id > m2.id) {
        return true;
    }
    return false;
}

struct comparator_2 {
    bool operator () (const Mat& m1, const Mat& m2) {
        // cout << "m1.id = " << m1.id << endl;
        // cout << "m2.id = " << m2.id << endl; 
        if (m1.id < m2.id) {
            return true;
        }
        return false;
    }
};

int main()
{
	vector<Mat> m;
	m.push_back(Mat(2, 2));
	m.push_back(Mat(3, 4));
	m.push_back(Mat(2, 2));
	cin >> m[0] >> m[1] >> m[2];

	sort(m.begin(), m.end());
	cout << m[0] << endl << m[1] << endl << m[2] << endl;
	cout << "*************" << endl;

	sort(m.begin(), m.end(), comparator_1);
	cout << m[0] << endl << m[1] << endl << m[2] << endl;
	cout << "*************" << endl;

	sort(m.begin(), m.end(), comparator_2());
	cout << m[0] << endl << m[1] << endl << m[2] << endl;
	return 0;
}