#pragma once
#include <Ty/StringView.h>

namespace He {

struct SourceFile {
    StringView file_name {};
    StringView text {};
};

}
