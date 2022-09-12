#include "He/Expression.h"
#include <CLI/ArgumentParser.h>
#include <Core/MappedFile.h>
#include <He/Codegen.h>
#include <He/Context.h>
#include <He/Lexer.h>
#include <He/Parser.h>
#include <He/Typecheck.h>
#include <SourceFile.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

[[nodiscard]] static int compile_source(c_string destination_path,
    c_string source_path);

int main(int argc, c_string argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    auto const* program_name = argv[0];
    argument_parser.add_flag("--help", "-h", "show help message",
        [&] {
            argument_parser.print_usage_and_exit(program_name, 0);
        });

    std::string_view output_path = "a.out";
    argument_parser.add_option("--output", "-o", "path",
        "output file path", [&](auto path) {
            output_path = path;
        });

    auto dump_tokens = false;
    argument_parser.add_flag("--dump-tokens", "-dt", "dump tokens",
        [&] {
            dump_tokens = true;
        });

    auto dump_expressions = false;
    argument_parser.add_flag("--dump-expressions", "-de",
        "dump expressions", [&] {
            dump_expressions = true;
        });

    auto export_source = false;
    argument_parser.add_flag("-S", "--export-generated-source",
        "export generated source instead of executable", [&] {
            export_source = true;
        });

    std::string_view source_file_path = {};
    argument_parser.add_positional_argument("file", [&](auto path) {
        source_file_path = path;
    });

    argument_parser.run(argc, argv);

    auto file_or_error = Core::MappedFile::open(source_file_path);
    if (file_or_error.is_error()) {
        file_or_error.error().show();
        return 1;
    }
    auto file = file_or_error.release_value();
    auto source_file = SourceFile { source_file_path, file.view() };
    auto lex_result = He::lex(source_file.text);
    if (lex_result.is_error()) {
        auto result = lex_result.error().show(source_file);
        if (result.is_error())
            result.error().show();
        return 1;
    }
    auto tokens = lex_result.release_value();
    if (dump_tokens) {
        for (u32 i = 0; i < tokens.size(); i++) {
            std::cerr << i << ' ';
            auto token = tokens[i];
            token.dump(source_file.text);
        }
    }

    auto parse_result = He::parse(tokens);
    if (parse_result.is_error()) {
        auto result = parse_result.error().show(source_file);
        if (result.is_error())
            result.error().show();
        return 1;
    }
    auto expressions = parse_result.release_value();

    if (dump_expressions) {
        for (auto const& expression : expressions.expressions) {
            expression.dump(expressions, source_file.text);
            std::cerr << "\n\n";
        }
    }

    auto context = He::Context { source_file, expressions };
    auto typecheck_result = He::typecheck(context);
    if (typecheck_result.is_error()) {
        auto result = typecheck_result.error().show(context);
        if (result.is_error())
            result.error().show();
        return 1;
    }
    auto codegen = typecheck_result.release_value();

    char temporary_file[] = "XXXXXX.c";
    int output_file = STDOUT_FILENO;
    if (isatty(STDOUT_FILENO) == 1 || export_source) {
        if (mkstemps(temporary_file, 2) < 0) {
            perror("mkstemps");
            return 1;
        }
        output_file = open(temporary_file, O_WRONLY);
        if (output_file < 0) {
            perror("open");
            return 1;
        }
    }
    codegen.dump(output_file, context);

    if (output_file != STDOUT_FILENO) {
        if (fsync(output_file) < 0) {
            perror("fsync");
            return 1;
        }
        if (close(output_file) < 0) {
            perror("close");
            return 1;
        }

        if (!export_source) {
            if (output_path[output_path.size()] != '\0')
                __builtin_abort();
            auto const* output_file = output_path.data();
            auto const* input_file = temporary_file;
            if (compile_source(output_file, input_file) < 0)
                return 1;
            (void)remove(temporary_file);
            return 0;
        }
        if (rename(temporary_file, output_path.data()) < 0) {
            perror("rename");
            return 1;
        }
    }
}

[[nodiscard]] static int compile_source(c_string destination_path,
    c_string source_path)
{
    char* argv[] = {
        (char*)(getenv("CC") ?: "clang"),
        (char*)"-Wno-duplicate-decl-specifier",
        (char*)"-o",
        (char*)destination_path,
        (char*)source_path,
        nullptr,
    };
    int pid = -1;
    if (posix_spawnp(&pid, argv[0], 0, 0, argv, environ) != 0) {
        perror("posix_spawnp");
        return -1;
    }
    if (pid < 0)
        return -1;

    int exit_code = 0;
    waitpid(pid, &exit_code, 0);
    if (WIFEXITED(exit_code))
        return WEXITSTATUS(exit_code);
    return -1;
}
