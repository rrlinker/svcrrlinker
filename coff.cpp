#include "coff.h"

template<typename T>
std::istream& operator>>(std::istream &is, T &out) {
    return is.read((char*)&out, sizeof(T));
}

coff::coff(std::istream &file) :
    file(file)
{}

void coff::read_headers() {
    file.seekg(0, file.beg);

    file >> file_header;

    if (file_header.SizeOfOptionalHeader > 0) {

    }

    sections = std::make_unique<IMAGE_SECTION_HEADER[]>(file_header.NumberOfSections);
    for (int i = 0; i < file_header.NumberOfSections; i++) {
        file >> sections[i];
    }
}

coff::~coff() {
}

