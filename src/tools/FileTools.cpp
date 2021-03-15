#include "tgbot/tools/FileTools.h"

#include <fstream>
#include <sstream>
#include <string>

using namespace std;

namespace FileTools {

std::shared_ptr<std::vector<unsigned char>> read(const string& filePath) {
    ifstream in(filePath, ios::in | ios::binary);
    in.exceptions(ifstream::failbit | ifstream::badbit);
    //ostringstream contents;
    //contents << in.rdbuf();
   // return contents.str();

    in.seekg(0, std::ios::end);
    size_t filesize = in.tellg();
    in.seekg(0, std::ios::beg);

    auto result = std::make_shared<std::vector<unsigned char>>(filesize);

    in.read((char*)result->data(), filesize);
    in.close();

    return result;
}

void write(const string& content, const string& filePath) {
    ofstream out(filePath, ios::out | ios::binary);
    out.exceptions(ofstream::failbit | ofstream::badbit);
    out << content;
    out.close();
}

}
