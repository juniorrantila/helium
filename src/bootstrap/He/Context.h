#pragma once
#include "Parser.h"
#include "SourceFile.h"

namespace He {

struct Context {
    StringView source;
    ParsedExpressions const& expressions;
};

}
