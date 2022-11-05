#pragma once

namespace Ty {

struct StringBuffer;
struct Error;

template <typename T, typename U = Error>
struct ErrorOr;

template <typename T>
struct Vector;

}

using namespace Ty;
