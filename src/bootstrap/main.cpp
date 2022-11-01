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

ErrorOr<int> Main::main(int argc, c_string argv[])
{
    auto argument_parser = CLI::ArgumentParser();

    c_string program_name = argv[0];
    TRY(argument_parser.add_flag("--help"sv, "-h"sv,
        "show help message"sv, [&] {
            argument_parser.print_usage_and_exit(program_name, 0);
        }));

    bool output_path_set = false;
    c_string output_path = "a.out";
    TRY(argument_parser.add_option("--output"sv, "-o"sv, "path"sv,
        "output file path"sv, [&](auto path) {
            output_path_set = true;
            output_path = path;
        }));

    auto dump_tokens = false;
    TRY(argument_parser.add_flag("--dump-tokens"sv, "-dt"sv,
        "dump tokens"sv, [&] {
            dump_tokens = true;
        }));

    auto dump_expressions = false;
    TRY(argument_parser.add_flag("--dump-expressions"sv, "-de"sv,
        "dump expressions"sv, [&] {
            dump_expressions = true;
        }));

    auto export_source = false;
    TRY(argument_parser.add_flag("--export-generated-source"sv,
        "-S"sv, "export generated source instead of executable"sv,
        [&] {
            export_source = true;
        }));

    auto should_display_benchmark
        = Core::BenchEnableShowOnStopAndShow::No;
    TRY(argument_parser.add_flag("--benchmark"sv, "-b"sv,
        "benchmark each compiler stage"sv, [&] {
            should_display_benchmark
                = Core::BenchEnableShowOnStopAndShow::Yes;
        }));

    auto stop_after_lex = false;
    TRY(argument_parser.add_flag("--lex"sv, "-l"sv,
        "stop program after lexing"sv, [&] {
            stop_after_lex = true;
        }));

    auto stop_after_parse = false;
    TRY(argument_parser.add_flag("--parse"sv, "-p"sv,
        "stop program after parsing"sv, [&] {
            stop_after_parse = true;
        }));

    auto stop_after_typecheck = false;
    TRY(argument_parser.add_flag("--typecheck"sv, "-t"sv,
        "stop program after typecheck"sv, [&] {
            stop_after_typecheck = true;
        }));

    c_string source_file_path = nullptr;
    TRY(argument_parser.add_positional_argument("file"sv,
        [&](auto path) {
            source_file_path = path;
        }));

    if (auto result = argument_parser.run(argc, argv);
        result.is_error()) {
        TRY(result.error().show());
        return 1;
    }
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
    if (lex_result.is_error()) {
        TRY(lex_result.error().show(source_file));
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
    if (stop_after_lex)
        return 0;

    bench.start();
    auto parse_result = He::parse(tokens);
    bench.stop_and_show("parse");
    if (parse_result.is_error()) {
        TRY(parse_result.error().show(source_file));
        return 1;
    }
    auto expressions = parse_result.release_value();
    if (dump_expressions) {
        expressions.dump(source_file.text);
    }
    if (stop_after_parse)
        return 0;

    auto context = He::Context {
        source_file.text,
        expressions,
    };
    bench.start();
    auto typecheck_result = He::typecheck(context);
    bench.stop_and_show("typecheck");
    if (typecheck_result.is_error()) {
        TRY(typecheck_result.error().show(context));
        return 1;
    }
    auto typechecked_expressions = typecheck_result.release_value();
    if (stop_after_typecheck)
        return 0;

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
        return 0;

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
        return 0;
    }

    bench.start();
    TRY(move_file(output_path, temporary_file));
    bench.stop_and_show("move file");

    return 0;
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
    c_string compiler = getenv("CC");
    if (!compiler) {
        return Error::from_string_literal(
            "could not fetch compiler, try setting environment "
            "variable 'CC'");
    }
    c_string argv[] = {
        compiler,
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
