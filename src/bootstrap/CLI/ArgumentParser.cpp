#include "ArgumentParser.h"
#include <exception>

namespace CLI {

void ArgumentParser::run(int argc, char const* argv[]) const
{
    auto program_name = argv[0];
    size_t used_positional_arguments = 0;
    for (int i = 1; i < argc; i++) {
        auto argument = argv[i];
        if (short_flag_callback_ids.find(argument)
            != short_flag_callback_ids.end()) {
            auto id = short_flag_callback_ids.at(argument);
            flag_callbacks[id]();
        } else if (long_flag_callback_ids.find(argument)
            != long_flag_callback_ids.end()) {
            auto id = long_flag_callback_ids.at(argument);
            flag_callbacks[id]();
        } else if (short_option_callback_ids.find(argument)
            != short_option_callback_ids.end()) {
            if (i + 1 >= argc) {
                std::cerr << "No argument provided for argument \""
                          << argument << "\"" << std::endl;
                std::cerr << std::endl
                          << "See help for more info ("
                          << program_name << " --help)"
                          << std::endl;
                exit(1);
            }
            auto value = argv[++i];
            try {
                auto id = short_option_callback_ids.at(argument);
                option_callbacks[id](value);
            } catch (std::exception& e) {
                std::cerr << "Invalid value \"" << value
                          << "\" for argument \"" << argument
                          << "\"" << std::endl
                          << "Reason: " << e.what() << std::endl;
                std::cerr << std::endl
                          << "See help for more info ("
                          << program_name << " --help)"
                          << std::endl;
                exit(1);
            }
        } else if (long_option_callback_ids.find(argument)
            != long_option_callback_ids.end()) {
            if (i + 1 >= argc) {
                std::cerr << "No argument provided for argument \""
                          << argument << "\"" << std::endl;
                std::cerr << std::endl
                          << "See help for more info ("
                          << program_name << " --help)"
                          << std::endl;
                exit(1);
            }
            auto value = argv[++i];
            try {
                auto id = long_option_callback_ids.at(argument);
                option_callbacks[id](value);
            } catch (std::exception& e) {
                std::cerr << "Invalid value \"" << value
                          << "\" for argument \"" << argument
                          << "\"" << std::endl
                          << "Reason: " << e.what() << std::endl;
                std::cerr << std::endl
                          << "See help for more info ("
                          << program_name << " --help)"
                          << std::endl;
                exit(1);
            }
        } else if (used_positional_arguments
            < positional_argument_callbacks.size()) {
            try {
                positional_argument_callbacks
                    [used_positional_arguments++](argument);
            } catch (std::exception& e) {
                std::cerr << "Invalid positional argument \""
                          << argument << "\" for \""
                          << positional_argument_placeholders
                                 [used_positional_arguments - 1]
                          << "\"" << std::endl
                          << "Reason: " << e.what() << std::endl;
                std::cerr << std::endl
                          << "See help for more info ("
                          << program_name << " --help)"
                          << std::endl;
            }
        } else {
            std::cerr << "Unrecognised argument \"" << argument
                      << "\"" << std::endl;
            std::cerr << std::endl
                      << "See help for more info (" << program_name
                      << " --help)" << std::endl;
            exit(1);
        }
    }

    if (used_positional_arguments
        != positional_argument_placeholders.size()) {
        if (positional_argument_placeholders.size()
                - used_positional_arguments
            == 1) {
            std::cerr << "Missing positional argument: "
                      << positional_argument_placeholders
                             [used_positional_arguments]
                      << std::endl;
        } else {
            std::cerr << "Missing positional arguments: "
                      << std::endl;
            for (size_t i = used_positional_arguments;
                 i < positional_argument_placeholders.size(); i++)
                std::cerr << "\t"
                          << positional_argument_placeholders[i]
                          << std::endl;
        }
        std::cerr << std::endl
                  << "See help for more info (" << program_name
                  << " --help)" << std::endl;
        exit(1);
    }
}

void ArgumentParser::print_usage_and_exit(
    std::string_view program_name, int exit_code) const
{
    auto& out = exit_code != 0 ? std::cerr : std::cout;
    out << "USAGE: " << program_name << " [flags|options] ";
    for (auto positional_argument :
        positional_argument_placeholders)
        out << positional_argument << " ";
    out << std::endl << std::endl;
    out << "FLAGS:" << std::endl;
    for (auto flag : flags) {
        out << '\t' << flag.short_name << ", " << flag.long_name
            << "\t\t " << flag.explanation << std::endl;
    }
    out << std::endl;
    out << "OPTIONS:" << std::endl;
    for (auto option : options) {
        out << '\t' << option.short_name << ", " << option.long_name
            << "\t <" << option.placeholder << ">\t\t "
            << option.explanation << std::endl;
    }
    exit(exit_code);
}

}
