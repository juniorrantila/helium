#pragma once
#include "Context.h"
#include "TypecheckedExpression.h"
#include <Ty/ErrorOr.h>
#include <Ty/StringBuffer.h>

namespace He {

ErrorOr<StringBuffer> codegen_header(Context const&,
    TypecheckedExpressions const&);

ErrorOr<StringBuffer> codegen(Context const&,
    TypecheckedExpressions const&);

}
