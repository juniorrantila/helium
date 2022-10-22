#pragma once
#include "Parser.h"
#include "SourceFile.h"

namespace He {

struct Context {
    SourceFile source;
    ParsedExpressions& expressions;
};

}
