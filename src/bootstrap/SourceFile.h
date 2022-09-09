#pragma once
#include <string_view>

struct SourceFile {
    std::string_view file_name {};
    std::string_view text {};
};
