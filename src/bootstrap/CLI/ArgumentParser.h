#pragma once
#include <Ty/Base.h>
#include <Ty/SmallMap.h>
#include <Ty/SmallVector.h>
#include <Ty/StringView.h>
#include <Ty/Try.h>
#include <functional>

namespace CLI {

struct ArgumentParser {
    ErrorOr<void> add_flag(StringView long_name,
        StringView short_name, StringView explanation,
        std::function<void()>&& callback)
    {
        TRY(flags.append({ long_name, short_name, explanation }));
        auto id = flag_callbacks.size();
        TRY(flag_callbacks.append(std::move(callback)));
        TRY(long_flag_callback_ids.append(long_name, id));
        TRY(short_flag_callback_ids.append(short_name, id));

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
        TRY(option_callbacks.append(std::move(callback)));
        TRY(long_option_callback_ids.append(long_name, id));
        TRY(short_option_callback_ids.append(short_name, id));

        return {};
    }

    ErrorOr<void> add_positional_argument(StringView placeholder,
        std::function<void(c_string)>&& callback)
    {
        TRY(positional_argument_placeholders.append(placeholder));
        TRY(positional_argument_callbacks.append(
            std::move(callback)));

        return {};
    }

    void run(int argc, char const* argv[]) const;

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
    SmallVector<std::function<void(c_string)>> option_callbacks;

    SmallVector<StringView> positional_argument_placeholders {};
    SmallVector<std::function<void(c_string)>>
        positional_argument_callbacks {};
};

}
