#include <CLI/ArgumentParser.h>
#include <Core/Bench.h>
#include <Core/MappedFile.h>
#include <Core/System.h>
#include <He/Codegen.h>
#include <He/Context.h>
#include <He/Expression.h>
#include <He/Lexer.h>
#include <He/Parser.h>
#include <He/SourceFile.h>
#include <He/Typecheck.h>
#include <He/TypecheckedExpression.h>
#include <Main/Main.h>

[[nodiscard]] static ErrorOr<void> move_file(c_string to,
    c_string from);

[[nodiscard]] static ErrorOr<void> compile_source(
    c_string destination_path, c_string source_path);

ErrorOr<void> Main::main(int argc, c_string argv[])
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

    auto should_display_benchmark
        = Core::BenchEnableShowOnStopAndShow::No;
    argument_parser.add_flag("--benchmark", "-b",
        "benchmark each compiler stage", [&] {
            should_display_benchmark
                = Core::BenchEnableShowOnStopAndShow::Yes;
        });

    c_string source_file_path = nullptr;
    argument_parser.add_positional_argument("file", [&](auto path) {
        source_file_path = path;
    });

    argument_parser.run(argc, argv);
    if (export_source && !output_path_set)
        output_path = "a.c";

    auto bench = Core::Bench(should_display_benchmark);

    auto file = TRY(Core::MappedFile::open(source_file_path));
    auto source_file = He::SourceFile {
        StringView::from_c_string(source_file_path),
        file.view(),
    };

    bench.start();
    auto lex_result = He::lex(source_file.text);
    bench.stop_and_show("lex");
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

    bench.start();
    auto parse_result = He::parse(tokens);
    bench.stop_and_show("parse");
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
    bench.start();
    auto typecheck_result = He::typecheck(context);
    bench.stop_and_show("typecheck");
    if (typecheck_result.is_error())
        TRY(typecheck_result.error().show(context));
    auto typechecked_expressions = typecheck_result.release_value();

    char temporary_file[] = "/tmp/XXXXXX.c";
    int output_file = STDOUT_FILENO;
    if (export_source || Core::System::isatty(STDOUT_FILENO)) {
        output_file
            = TRY(Core::System::mkstemps(temporary_file, 2));
    }
    bench.start();
    auto code = TRY(He::codegen(context, typechecked_expressions));
    bench.stop_and_show("codegen");

    bench.start();
    TRY(Core::System::write(output_file, code));
    bench.stop_and_show("write");

    if (output_file == STDOUT_FILENO)
        return {};

    TRY(Core::System::close(output_file));

    if (!export_source) {
        bench.start();
        TRY(compile_source(output_path, temporary_file));
        bench.stop_and_show("compile c");
        auto remove_result = Core::System::remove(temporary_file);
        if (remove_result.is_error()) {
            auto error_message = remove_result.error().message();
            (void)fprintf(stderr, "remove '%s': %.*s\n",
                temporary_file, error_message.size,
                error_message.data);
        }
        return {};
    }

    bench.start();
    TRY(move_file(output_path, temporary_file));
    bench.stop_and_show("move file");

    return {};
}

static ErrorOr<void> move_file(c_string to, c_string from)
{
    auto from_file = TRY(Core::MappedFile::open(from));
    auto to_fd = TRY(Core::System::open(to, O_WRONLY, 0666));
    TRY(Core::System::write(to_fd, from_file));
    TRY(Core::System::close(to_fd));
    TRY(Core::System::unlink(from));
    return {};
}

[[nodiscard]] static ErrorOr<void> compile_source(
    c_string destination_path, c_string source_path)
{
    c_string argv[] = {
        getenv("CC") ?: "clang",
        "-Wno-duplicate-decl-specifier",
        "-o",
        destination_path,
        source_path,
        nullptr,
    };

    auto pid = TRY(Core::System::posix_spawnp(argv[0], argv));
    auto status = TRY(Core::System::waitpid(pid));
    if (!status.did_exit() || status.exit_status() != 0)
        return Error::from_string_literal(
            "could not compile source");

    return {};
}
