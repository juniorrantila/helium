#include <Types.h>
#include <string_view>

// clang-format off
[[maybe_unused]]
constexpr static auto to_string_view(c_string data)
{
    return std::string_view { data };
}

[[maybe_unused]]
constexpr static auto to_string_view(char const& c)
{
    return std::string_view { &c, 1 };
}

[[maybe_unused]]
constexpr static auto to_string_view(std::string_view data)
{
    return data;
}
// clang-format on

struct FileBuffer {
    constexpr FileBuffer(char* data, u32 capacity)
        : data(data)
        , capacity(capacity)
    {
    }

    char* data { nullptr };
    u32 size { 0 };
    u32 capacity { 0 };

    template <typename... Args>
    constexpr void write(Args... args)
    {
        const std::string_view strings[] = {
            to_string_view(args)...,
        };
        for (auto const string : strings)
            write(string);
    }

    template <typename... Args>
    constexpr void writeln(Args... args)
    {
        write(args..., '\n');
    }

    constexpr void write(std::string_view string)
    {
        if (size + string.size() >= capacity) {
            __builtin_printf("overflow file buffer");
            __builtin_abort();
        }
        string.copy(&data[size], string.size());
        size += string.size();
    }

    constexpr void writeln(std::string_view string)
    {
        if (size + string.size() >= capacity) {
            __builtin_printf("overflow file buffer");
            __builtin_abort();
        }
        string.copy(&data[size], string.size());
        size += string.size();
        write('\n');
    }

    constexpr void write(char c)
    {
        if (size + 1 >= capacity) {
            __builtin_printf("overflow file buffer");
            __builtin_abort();
        }
        data[size++] = c;
    }

    constexpr void writeln(char c)
    {
        if (size + 2 >= capacity) {
            __builtin_printf("overflow file buffer");
            __builtin_abort();
        }
        data[size++] = c;
        data[size++] = '\n';
    }
};
