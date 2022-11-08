#pragma once

namespace Ty {

struct StringBuffer;
struct Error;

template <typename T, typename U = Error>
struct ErrorOr;

template <typename T>
struct Vector;

struct StringView;

template <typename T>
struct Formatter;

template <typename Signature>
class Function;

}

using namespace Ty;
