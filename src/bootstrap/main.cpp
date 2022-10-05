#include <CLI/ArgumentParser.h>
#include <Core/Defer.h>
#include <Core/MappedFile.h>
#include <He/Context.h>
#include <He/Expression.h>
#include <He/Lexer.h>
#include <He/Parser.h>
#include <He/Typecheck.h>
#include <He/TypecheckedExpression.h>
#include <SourceFile.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

[[nodiscard]] static Core::ErrorOr<void> move_file(c_string to,
    c_string from);

[[nodiscard]] static Core::ErrorOr<void> compile_source(
    c_string destination_path, c_string source_path);

namespace He {

Core::ErrorOr<void> main(int argc, c_string argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    auto const* program_name = argv[0];
    argument_parser.add_flag("--help", "-h", "show help message",
        [&] {
            argument_parser.print_usage_and_exit(program_name, 0);
        });

    bool output_path_set = false;
    c_string output_path = "a.out";
    argument_parser.add_option("--output", "-o", "path",
        "output file path", [&](auto path) {
            output_path_set = true;
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
    argument_parser.add_flag("--export-generated-source", "-S",
        "export generated source instead of executable", [&] {
            export_source = true;
        });

    c_string source_file_path = nullptr;
    argument_parser.add_positional_argument("file", [&](auto path) {
        source_file_path = path;
    });

    argument_parser.run(argc, argv);
    if (export_source && !output_path_set)
        output_path = "a.c";

    auto file = TRY(Core::MappedFile::open(source_file_path));
    auto source_file = SourceFile {
        source_file_path,
        file.view(),
    };
    auto lex_result = He::lex(source_file.text);
    if (lex_result.is_error())
        return lex_result.error().show(source_file);
    auto tokens = lex_result.release_value();
    if (dump_tokens) {
        for (u32 i = 0; i < tokens.size(); i++) {
            std::cerr << i << ' ';
            auto token = tokens[i];
            token.dump(source_file.text);
        }
    }

    auto parse_result = He::parse(tokens);
    if (parse_result.is_error())
        return parse_result.error().show(source_file);
    auto expressions = parse_result.release_value();
    if (dump_expressions) {
        for (auto const& expression : expressions.expressions) {
            expression.dump(expressions, source_file.text);
            std::cerr << "\n\n";
        }
    }

    auto context = He::Context { source_file, expressions };
    auto typecheck_result = He::typecheck(context);
    if (typecheck_result.is_error())
        TRY(typecheck_result.error().show(context));
    auto typechecked_expressions = typecheck_result.release_value();

    char temporary_file[] = "/tmp/XXXXXX.c";
    int output_file = STDOUT_FILENO;
    if (isatty(STDOUT_FILENO) == 1 || export_source) {
        output_file = mkstemps(temporary_file, 2);
        if (output_file < 0)
            return Core::Error::from_errno();
    }
    typechecked_expressions.codegen(output_file, context);

    if (output_file == STDOUT_FILENO)
        return {};

    if (fsync(output_file) < 0)
        return Core::Error::from_errno();
    if (close(output_file) < 0)
        return Core::Error::from_errno();

    if (!export_source) {
        auto const* input_file = temporary_file;
        TRY(compile_source(output_path, input_file));
        (void)remove(temporary_file);
        return {};
    }
    TRY(move_file(output_path, temporary_file));

    return {};
}

}

int main(int argc, c_string argv[])
{
    auto result = He::main(argc, argv);
    if (result.is_error()) {
        result.error().show();
        return 1;
    }
    return 0;
}

static Core::ErrorOr<void> move_file(c_string to, c_string from)
{
    auto from_file = TRY(Core::MappedFile::open(from));
    auto to_fd = open(to, O_WRONLY | O_CREAT, 0666);
    if (to_fd < 0)
        return Core::Error::from_errno();

    if (write(to_fd, from_file.m_data, from_file.m_size) < 0)
        return Core::Error::from_errno();

    close(to_fd);

    if (unlink(from) < 0)
        return Core::Error::from_errno();

    return {};
}

[[nodiscard]] static Core::ErrorOr<void> compile_source(
    c_string destination_path, c_string source_path)
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
    if (posix_spawnp(&pid, argv[0], 0, 0, argv, environ) != 0)
        return Core::Error::from_errno();
    if (pid < 0)
        return Core::Error::from_errno();

    int exit_code = 0;
    waitpid(pid, &exit_code, 0);
    if (!WIFEXITED(exit_code) || WEXITSTATUS(exit_code) != 0)
        return Core::Error::from_string_literal(
            "could not compile source");

    return {};
}
