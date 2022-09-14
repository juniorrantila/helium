#pragma once
#include <Types.h>
#include <functional>
#include <iostream>
#include <string_view>
#include <map>

namespace CLI {

class ArgumentParser {
public:
    void add_flag(std::string_view long_name,
        std::string_view short_name, std::string_view explanation,
        std::function<void()> const& callback)
    {
        flags.push_back({ long_name, short_name, explanation });
        auto id = flag_callbacks.size();
        flag_callbacks.push_back(callback);
        long_flag_callback_ids[long_name] = id;
        short_flag_callback_ids[short_name] = id;
    }

    void add_option(std::string_view long_name,
        std::string_view short_name, std::string_view placeholder,
        std::string_view explanation,
        std::function<void(c_string)> const& callback)
    {
        options.push_back({
            long_name,
            short_name,
            explanation,
            placeholder,
        });
        auto id = option_callbacks.size();
        option_callbacks.push_back(callback);
        long_option_callback_ids[long_name] = id;
        short_option_callback_ids[short_name] = id;
    }

    void add_positional_argument(std::string_view placeholder,
        std::function<void(c_string)> const& callback)
    {
        positional_argument_placeholders.push_back(placeholder);
        positional_argument_callbacks.push_back(callback);
    }

    void run(int argc, char const* argv[]) const;

    void print_usage_and_exit(std::string_view program_name,
        int exit_code = 0) const;

private:
    struct Flag {
        std::string_view long_name;
        std::string_view short_name;
        std::string_view explanation;
    };
    std::vector<Flag> flags {};

    struct Option {
        std::string_view long_name;
        std::string_view short_name;
        std::string_view explanation;
        std::string_view placeholder;
    };
    std::vector<Option> options {};

    std::map<std::string_view, u32> short_flag_callback_ids {};
    std::map<std::string_view, u32> long_flag_callback_ids {};

    std::map<std::string_view, u32> short_option_callback_ids {};
    std::map<std::string_view, u32> long_option_callback_ids {};

    std::vector<std::function<void()>> flag_callbacks;
    std::vector<std::function<void(c_string)>>
        option_callbacks;

    std::vector<std::string_view>
        positional_argument_placeholders {};
    std::vector<std::function<void(c_string)>>
        positional_argument_callbacks {};
};

}
