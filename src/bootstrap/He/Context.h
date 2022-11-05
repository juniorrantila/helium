#pragma once
#include "Parser.h"
#include "SourceFile.h"

namespace He {

struct Context {
    StringView source;
    StringView namespace_;
    ParsedExpressions const& expressions;
};

}
