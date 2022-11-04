#include "ArgumentParser.h"
#include <Core/File.h>
#include <Mem/Sizes.h>
#include <Ty/Move.h>

namespace CLI {

ErrorOr<void> ArgumentParserError::show() const
{
    switch (m_state) {
    case State::Buffer: {
        TRY(Core::File::stderr().write(m_buffer));
    } break;
    case State::Error: {
        TRY(Core::File::stderr().write(m_error));
    } break;
    case State::Invalid: break;
    }

    return {};
}

ArgumentParserResult ArgumentParser::run(int argc,
    c_string argv[]) const
{
    c_string program_name = argv[0];
    auto program_name_view
        = StringView::from_c_string(program_name);
    size_t used_positional_arguments = 0;
    for (int i = 1; i < argc; i++) {
        auto argument = StringView::from_c_string(argv[i]);
        if (auto id = short_flag_callback_ids.find(argument);
            id.is_valid()) {
            flag_callbacks[id.raw()]();
        } else if (auto id = long_flag_callback_ids.find(argument);
                   id.is_valid()) {
            flag_callbacks[id.raw()]();
        } else if (auto id
                   = short_option_callback_ids.find(argument);
                   id.is_valid()) {
            if (i + 1 >= argc) {
                auto out = TRY(StringBuffer::create(1 * Mem::KiB));
                TRY(out.writeln(
                    "No argument provided for argument \""sv,
                    argument, "\""sv));
                TRY(out.writeln("\nSee help for more info ("sv,
                    program_name_view, " --help)"sv));
                return ArgumentParserError { move(out) };
            }
            c_string value = argv[++i];
            auto result = option_callbacks[id.raw()](value);
            if (result.is_error()) {
                auto out = TRY(StringBuffer::create(1 * Mem::KiB));

                auto value_view = StringView::from_c_string(value);
                auto reason = result.error().message();
                TRY(out.writeln("Invalid value \""sv, value_view,
                    "\" for argument \""sv, argument, "\""sv));
                TRY(out.writeln("Reason: "sv, reason));
                TRY(out.writeln("\nSee help for more info ("sv,
                    program_name_view, " --help)"sv));

                return ArgumentParserError { move(out) };
            }
        } else if (auto id
                   = long_option_callback_ids.find(argument);
                   id.is_valid()) {
            if (i + 1 >= argc) {
                auto out = TRY(StringBuffer::create(1 * Mem::KiB));
                TRY(out.writeln("No argument provided for \""sv,
                    argument, "\"\n"sv));
                TRY(out.writeln("See help for more info ("sv,
                    program_name_view, " --help)"sv));
                return ArgumentParserError { move(out) };
            }
            c_string value = argv[++i];
            auto result = option_callbacks[id.raw()](value);
            if (result.is_error()) {
                auto out = TRY(StringBuffer::create(1 * Mem::KiB));

                auto value_view = StringView::from_c_string(value);
                auto reason = result.error().message();
                TRY(out.writeln("Invalid value \""sv, value_view,
                    "\" for argument \""sv, argument, "\""sv));
                TRY(out.writeln("Reason: "sv, reason, "\n"sv));
                TRY(out.writeln("See help for more info ("sv,
                    program_name_view, "\" --help)"sv));

                return ArgumentParserError { move(out) };
            }
        } else if (used_positional_arguments
            < positional_argument_callbacks.size()) {
            auto id = used_positional_arguments++;
            auto result
                = positional_argument_callbacks[id](argument.data);
            if (result.is_error()) {
                auto out = TRY(StringBuffer::create(1024));

                auto placeholder = positional_placeholders
                    [used_positional_arguments - 1];
                TRY(out.writeln("Invalid positional argument \""sv,
                    argument, "\" for \""sv, placeholder, "\""sv));

                auto reason = result.error().message();
                TRY(out.writeln("Reason: "sv, reason, "\n"sv));
                TRY(out.writeln("See help for more info ("sv,
                    program_name_view, " --help)"sv));

                return ArgumentParserError { move(out) };
            }
        } else {
            auto out = TRY(StringBuffer::create(1 * Mem::KiB));

            TRY(out.writeln("Unrecognised argument: \""sv, argument,
                "\""sv));
            TRY(out.writeln("\nSee help for more info ("sv,
                program_name_view, " --help)"sv));

            return ArgumentParserError { move(out) };
        }
    }

    if (used_positional_arguments
        != positional_placeholders.size()) {
        auto out = TRY(StringBuffer::create(1 * Mem::KiB));

        if (positional_placeholders.size()
                - used_positional_arguments
            == 1) {
            auto out = TRY(StringBuffer::create(1 * Mem::KiB));
            auto placeholder = positional_placeholders
                [used_positional_arguments];

            TRY(out.writeln("Missing positional argument: "sv,
                placeholder));
        } else {
            TRY(out.writeln("Missing positional arguments: "sv));
            for (size_t i = used_positional_arguments;
                 i < positional_placeholders.size(); i++) {
                auto placeholder = positional_placeholders[i];
                TRY(out.writeln("\t"sv, placeholder));
            }
        }

        TRY(out.writeln("\nSee help for more info ("sv,
            program_name_view, " --help)"sv));

        return ArgumentParserError { move(out) };
    }

    return {};
}

void ArgumentParser::print_usage_and_exit(c_string program_name,
    int exit_code) const
{
    auto& out = exit_code != 0 ? Core::File::stderr()
                               : Core::File::stdout();
    auto program_name_view
        = StringView::from_c_string(program_name);
    (void)out.write("USAGE: "sv, program_name_view,
        " [flags|options] "sv);
    for (auto positional_argument : positional_placeholders)
        (void)out.write(positional_argument, " "sv);
    (void)out.write("\n\n"sv);
    (void)out.writeln("FLAGS:"sv);
    for (auto flag : flags) {
        auto pad = flag.short_name.size == 2 ? " "sv : ""sv;
        auto bytes = out.write("        "sv, flag.short_name,
                            ", "sv, pad, flag.long_name)
                         .release_value();
        for (; bytes < 40; bytes++)
            (void)out.write(" "sv);
        (void)out.writeln(flag.explanation);
    }
    (void)out.writeln("\nOPTIONS:"sv);
    for (auto option : options) {
        auto pad = option.short_name.size == 2 ? " "sv : ""sv;
        auto bytes = out.write("        "sv, option.short_name,
                            ", "sv, pad, option.long_name, "  <"sv,
                            option.placeholder, "> "sv)
                         .release_value();
        for (; bytes < 40; bytes++)
            (void)out.write(" "sv);
        (void)out.writeln(option.explanation);
    }
    (void)out.writeln();
    (void)out.flush();

    _Exit(exit_code);
}
}
