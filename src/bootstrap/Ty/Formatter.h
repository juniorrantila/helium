#pragma once
#include "Concepts.h"
#include "Error.h"
#include "ErrorOr.h"
#include "Forward.h"
#include "StringView.h"
#include "Try.h"

namespace Ty {

namespace CompileError {
template <typename T>
consteval ErrorOr<u32> formatter_not_defined_for();
}

template <typename T>
requires(!is_trivially_copyable<T>) struct Formatter<T> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U&, T const&)
    {
        return CompileError::formatter_not_defined_for<T>();
    }
};

template <typename T>
requires is_trivially_copyable<T>
struct Formatter<T> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U&, T)
    {
        return CompileError::formatter_not_defined_for<T>();
    }
};

template <>
struct Formatter<StringView> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, StringView view)
    {
        return TRY(to.write(view));
    }
};

template <>
struct Formatter<u128> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u128 number)
    {
        if (number == 0) {
            return TRY(to.write("0"sv));
        }

        constexpr auto max_digits_in_u128 = 39;
        char buffer[max_digits_in_u128];
        u32 buffer_start = max_digits_in_u128;

        while (number != 0) {
            buffer_start--;
            buffer[buffer_start] = digit_to_character(number % 10);
            number /= 10;
        }

        u32 buffer_size = max_digits_in_u128 - buffer_start;

        auto view = StringView(&buffer[buffer_start], buffer_size);
        return TRY(to.write(view));
    }

private:
    static constexpr char digit_to_character(u8 number)
    {
        return (char)('0' + number);
    }
};

template <>
struct Formatter<u64> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u64 number)
    {
        if (number == 0) {
            return TRY(to.write("0"sv));
        }

        constexpr auto max_digits_in_u64 = 20;
        char buffer[max_digits_in_u64];
        u32 buffer_start = max_digits_in_u64;

        while (number != 0) {
            buffer_start--;
            buffer[buffer_start] = digit_to_character(number % 10);
            number /= 10;
        }

        u32 buffer_size = max_digits_in_u64 - buffer_start;

        auto view = StringView(&buffer[buffer_start], buffer_size);
        return TRY(to.write(view));
    }

private:
    static constexpr char digit_to_character(u8 number)
    {
        return (char)('0' + number);
    }
};

template <>
struct Formatter<u32> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u32 number)
    {
        return TRY(Formatter<u64>::write(to, number));
    }
};

template <>
struct Formatter<u16> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, u16 number)
    {
        return TRY(Formatter<u64>::write(to, number));
    }
};

template <>
struct Formatter<i128> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i128 number)
    {
        u32 size = 0;
        if (number < 0) {
            size += TRY(to.write("-"sv));
            number = -number;
        }

        size += TRY(Formatter<u128>::write(to, number));

        return size;
    }
};

template <>
struct Formatter<i64> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i64 number)
    {
        u32 size = 0;
        if (number < 0) {
            size += TRY(to.write("-"sv));
            number = -number;
        }

        size += TRY(Formatter<u64>::write(to, number));

        return size;
    }
};

template <>
struct Formatter<i32> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i32 number)
    {
        return TRY(Formatter<i64>::write(to, number));
    }
};

template <>
struct Formatter<i16> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, i16 number)
    {
        return TRY(Formatter<i64>::write(to, number));
    }
};

template <>
struct Formatter<f64> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, f64 number)
    {
        u32 size = 0;

        if (number < 0) {
            size += TRY(to.write("-"sv));
            number = -number;
        }
        auto integer_part = (u128)number;
        size += TRY(Formatter<u128>::write(to, integer_part));
        size += TRY(to.write("."sv));
        auto fraction_part = (number - (f64)integer_part);
        auto fraction = (u128)(fraction_part * 1000000000000000.0);
        while (fraction % 10 == 0)
            fraction /= 10;
        size += TRY(Formatter<u128>::write(to, fraction));

        return size;
    }
};

template <>
struct Formatter<f32> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, f32 number)
    {
        return TRY(Formatter<f64>::write(to, number));
    }
};

template <>
struct Formatter<char> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, char character)
    {
        return TRY(to.write(StringView(&character, 1)));
    }
};

template <>
struct Formatter<Error> {
    template <typename U>
    requires Writable<U>
    static constexpr ErrorOr<u32> write(U& to, Error error)
    {
        auto size = TRY(
            to.write(error.function(), ": "sv, error.message()));
        size += TRY(to.writeln(" ["sv, error.file(), ":"sv,
            error.line_in_file(), "]"sv));

        return size;
    }
};

}

using namespace Ty;
