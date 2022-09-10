#pragma once
#include <He/Parser.h>
#include <SourceFile.h>

namespace He {

struct Context {
    SourceFile source;
    ParsedExpressions& expressions;
};

}
