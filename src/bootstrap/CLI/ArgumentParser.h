#pragma once
#include <Ty/Base.h>
#include <Ty/Move.h>
#include <Ty/SmallMap.h>
#include <Ty/SmallVector.h>
#include <Ty/StringBuffer.h>
#include <Ty/StringView.h>
#include <Ty/Try.h>
#include <functional>

namespace CLI {

struct ArgumentParserError {
    ErrorOr<void> show() const;

    ArgumentParserError(StringBuffer&& buffer)
        : m_buffer(move(buffer))
        , m_state(State::Buffer)
    {
    }

    ArgumentParserError(Error error)
        : m_error(error)
        , m_state(State::Error)
    {
    }

    ArgumentParserError(ArgumentParserError&& other)
        : m_state(other.m_state)
    {
        switch (other.m_state) {
        case State::Buffer: {
            new (&m_buffer) StringBuffer(move(other.m_buffer));
        } break;
        case State::Error: {
            m_error = other.m_error;
        } break;
        case State::Invalid: break;
        }
        other.m_state = State::Invalid;
    }

    ~ArgumentParserError()
    {
        switch (m_state) {
        case State::Buffer: m_buffer.~StringBuffer(); break;
        case State::Error: m_error.~Error(); break;
        case State::Invalid: break;
        }
        m_state = State::Invalid;
    }

    StringView message() const
    {
        switch (m_state) {
        case State::Buffer: return m_buffer.view(); break;
        case State::Error: return m_error.message(); break;
        case State::Invalid: return "should never get here"sv;
        }
    }

private:
    union {
        StringBuffer m_buffer;
        Error m_error;
    };
    enum class State : u8 {
        Buffer,
        Error,
        Invalid,
    };
    State m_state;
};
using ArgumentParserResult = ErrorOr<void, ArgumentParserError>;

struct ArgumentParser {
    ErrorOr<void> add_flag(StringView long_name,
        StringView short_name, StringView explanation,
        std::function<void()>&& callback)
    {
        TRY(flags.append({ long_name, short_name, explanation }));
        auto id = flag_callbacks.size();
        TRY(flag_callbacks.append(move(callback)));
        TRY(long_flag_callback_ids.append(long_name, id));
        TRY(short_flag_callback_ids.append(short_name, id));

        return {};
    }

    ErrorOr<void> add_fallible_option(StringView long_name,
        StringView short_name, StringView placeholder,
        StringView explanation,
        std::function<ArgumentParserResult(c_string)>&& callback)
    {
        TRY(options.append({
            long_name,
            short_name,
            explanation,
            placeholder,
        }));
        auto id = option_callbacks.size();
        TRY(option_callbacks.append(move(callback)));
        TRY(long_option_callback_ids.append(long_name, id));
        TRY(short_option_callback_ids.append(short_name, id));

        return {};
    }

    ErrorOr<void> add_option(StringView long_name,
        StringView short_name, StringView placeholder,
        StringView explanation,
        std::function<void(c_string)>&& callback)
    {
        TRY(options.append({
            long_name,
            short_name,
            explanation,
            placeholder,
        }));
        auto id = option_callbacks.size();
        TRY(option_callbacks.append(
            [=](auto argument) -> ArgumentParserResult {
                callback(argument);
                return {};
            }));

        TRY(long_option_callback_ids.append(long_name, id));
        TRY(short_option_callback_ids.append(short_name, id));

        return {};
    }

    ErrorOr<void> add_fallible_positional_argument(
        StringView placeholder,
        std::function<ArgumentParserResult(c_string)>&& callback)
    {
        TRY(positional_placeholders.append(placeholder));
        TRY(positional_argument_callbacks.append(move(callback)));

        return {};
    }

    ErrorOr<void> add_positional_argument(StringView placeholder,
        std::function<void(c_string)>&& callback)
    {
        TRY(positional_placeholders.append(placeholder));
        TRY(positional_argument_callbacks.append(
            [=](auto argument) -> ArgumentParserResult {
                callback(argument);
                return {};
            }));

        return {};
    }

    ArgumentParserResult run(int argc, char const* argv[]) const;

    void print_usage_and_exit(c_string program_name,
        int exit_code = 0) const;

private:
    struct Flag {
        StringView long_name;
        StringView short_name;
        StringView explanation;
    };
    SmallVector<Flag> flags {};

    struct Option {
        StringView long_name;
        StringView short_name;
        StringView explanation;
        StringView placeholder;
    };
    SmallVector<Option> options {};

    SmallMap<StringView, u32> short_flag_callback_ids {};
    SmallMap<StringView, u32> long_flag_callback_ids {};

    SmallMap<StringView, u32> short_option_callback_ids {};
    SmallMap<StringView, u32> long_option_callback_ids {};

    SmallVector<std::function<void()>> flag_callbacks;
    SmallVector<std::function<ArgumentParserResult(c_string)>>
        option_callbacks;

    SmallVector<StringView> positional_placeholders {};
    SmallVector<std::function<ArgumentParserResult(c_string)>>
        positional_argument_callbacks {};
};
}
