#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

#pragma pack(push, 1)
struct FileHeader {
    unsigned short type;
    unsigned int size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;
};

struct InformationHeader {
    unsigned int informationSize;
    int width;
    int height;
    unsigned short planes;
    unsigned short bitCount;
    unsigned int compression;
    unsigned int imageSize;
    int xPels;
    int yPels;
    unsigned int colorUsed;
    unsigned int colorImportant;
};
#pragma pack(pop)

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Please input 3 args. " << endl;
        return 0;
    }
    ifstream srcFile(argv[1], ios::binary|ios::in);
    if (!srcFile) {
        cerr << "src BMP File open error! " << endl;
        return 0;
    }
    ofstream destFile(argv[2], ios::binary|ios::out);
    if (!destFile) {
        cerr << "dest BMP File create error! " << endl;
        return 0;
    }

    FileHeader fh;
    InformationHeader ih;
    srcFile.read((char*) &fh, sizeof(fh));
    srcFile.read((char*) &ih, sizeof(ih));
    if (fh.type != 0x4D42 || ih.bitCount != 24 || ih.compression != 0) {
        cerr << "support 24 bits without compression only! " << endl;
        srcFile.close();
        return 0;
    }

    int width = ih.width;
    int height = ih.height;
    int lineWidth = width * 3;
    int fillBytes = (4 - lineWidth % 4) % 4;
    vector<vector<unsigned char>> pixels(height, vector<unsigned char>(lineWidth));
    for (int i = height - 1; i >= 0; --i) {
        srcFile.read((char*) pixels[i].data(), lineWidth);
        srcFile.seekg(fillBytes, ios::cur);
    }
    srcFile.close();

    FileHeader fh_out = fh;
    InformationHeader ih_out = ih;
    int lineWidth_out = height * 3;
    int fillBytes_out = (4 - lineWidth_out % 4) % 4;
    fh_out.size = 54 + ih_out.imageSize;
    ih_out.height = width;
    ih_out.width = height;
    ih_out.imageSize = (lineWidth_out + fillBytes_out) * ih_out.height;
    unsigned char zeros[4] = {0, 0, 0, 0};

    destFile.write((char*) &fh_out, sizeof(fh_out));
    destFile.write((char*) &ih_out, sizeof(ih_out));
    for (int i = width - 1; i >= 0; --i) {
        for (int j = 0; j < height; ++j) {
            destFile.write((char*) &pixels[height - 1 - j][i * 3], 3);
        }
        destFile.write((char*) zeros, fillBytes_out);
    }
    destFile.close();
    return 0;
};