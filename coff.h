#pragma once

#include <iostream>
#include <memory>

#include "vendor/wine/windef.h"

class coff {
    public:
        coff(std::istream &file);
        virtual ~coff();

        void read_headers();


    protected:
        std::istream &file;

        IMAGE_FILE_HEADER file_header;
        union {
            std::unique_ptr<IMAGE_OPTIONAL_HEADER32> optional_header32;
            std::unique_ptr<IMAGE_OPTIONAL_HEADER64> optional_header64;
        };
        std::unique_ptr<IMAGE_SECTION_HEADER[]> sections;
};

