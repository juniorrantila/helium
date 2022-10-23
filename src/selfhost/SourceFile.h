#pragma once
#include <Ty/Base.h>

typedef struct SourceFile {
    c_string name;
    c_string text;
    u32 name_size;
    u32 text_size;
} SourceFile;
