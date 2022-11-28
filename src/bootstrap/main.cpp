#include <CLI/ArgumentParser.h>
#include <Core/Bench.h>
#include <Core/File.h>
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
#include <Ty/StringBuffer.h>

[[nodiscard]] static ErrorOr<void> move_file(c_string to,
    c_string from);

[[nodiscard]] static ErrorOr<void> compile_source(
    c_string destination_path, c_string source_path);

static ErrorOr<StringBuffer> namespace_from_path(StringView path);

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

    c_string header_output_path = nullptr;
    TRY(argument_parser.add_option("--output-header"sv, "-oh"sv,
        "path"sv, "header file output path"sv, [&](auto path) {
            header_output_path = path;
        }));

    auto should_dump_tokens = false;
    TRY(argument_parser.add_flag("--dump-tokens"sv, "-dt"sv,
        "dump tokens"sv, [&] {
            should_dump_tokens = true;
        }));

    auto should_dump_expressions = false;
    TRY(argument_parser.add_flag("--dump-expressions"sv, "-de"sv,
        "dump expressions"sv, [&] {
            should_dump_expressions = true;
        }));

    auto export_source = false;
    TRY(argument_parser.add_flag("--export-generated-source"sv,
        "-S"sv, "export generated source instead of executable"sv,
        [&] {
            export_source = true;
        }));

    auto should_display_benchmark
        = Core::BenchEnableAutoDisplay::No;
    TRY(argument_parser.add_flag("--benchmark"sv, "-b"sv,
        "benchmark each compiler stage"sv, [&] {
            should_display_benchmark
                = Core::BenchEnableAutoDisplay::Yes;
        }));

    auto stop_after_lex = false;
    TRY(argument_parser.add_flag("--stop-after-lex"sv, "-sl"sv,
        "stop program after lexing"sv, [&] {
            stop_after_lex = true;
        }));

    auto stop_after_parse = false;
    TRY(argument_parser.add_flag("--stop-after-parse"sv, "-sp"sv,
        "stop program after parsing"sv, [&] {
            stop_after_parse = true;
        }));

    auto stop_after_typecheck = false;
    TRY(argument_parser.add_flag("--stop-after-typecheck"sv,
        "-st"sv, "stop program after typecheck"sv, [&] {
            stop_after_typecheck = true;
        }));

    auto stop_after_codegen = false;
    TRY(argument_parser.add_flag("--stop-after-codegen"sv, "-sc"sv,
        "stop program after codegen"sv, [&] {
            stop_after_codegen = true;
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
    auto lex_result = bench("lex"sv, [&] {
        return He::lex(source_file.text);
    });
    if (lex_result.is_error()) {
        TRY(lex_result.error().show(source_file));
        return 1;
    }
    auto tokens = lex_result.release_value();
    if (should_dump_tokens)
        TRY(dump_tokens(source_file.text, tokens.view()));
    if (stop_after_lex)
        return 0;

    auto parse_result = bench("parse"sv, [&] {
        return He::parse(tokens);
    });
    if (parse_result.is_error()) {
        TRY(parse_result.error().show(source_file));
        return 1;
    }
    auto expressions = parse_result.release_value();
    if (should_dump_expressions) {
        expressions.dump(source_file.text);
        TRY(Core::File::stderr().flush());
    }
    if (stop_after_parse)
        return 0;

    auto namespace_
        = TRY(namespace_from_path(source_file.file_name));
    auto context = He::Context {
        source_file.text,
        namespace_.view(),
        expressions,
    };
    auto typecheck_result = bench("typecheck"sv, [&] {
        return He::typecheck(context);
    });
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

    auto code = TRY(bench("codegen"sv, [&] {
        return He::codegen(context, typechecked_expressions);
    }));
    if (stop_after_codegen)
        return 0;

    TRY(bench("write"sv, [&] {
        return Core::System::write(output_file, code);
    }));

    if (output_file == STDOUT_FILENO)
        return 0;

    TRY(Core::System::close(output_file));

    if (!export_source) {
        TRY(bench("compile_source"sv, [&] {
            return compile_source(output_path, temporary_file);
        }));
        auto remove_result = Core::System::remove(temporary_file);
        if (remove_result.is_error()) {
            auto error_message = remove_result.error().message();
            auto file_name_view
                = StringView::from_c_string(temporary_file);
            TRY(Core::File::stderr().writeln("remove '"sv,
                file_name_view, "': "sv, error_message));
        }
        return 0;
    }

    auto header = TRY(bench("codegen_header"sv, [&] {
        return He::codegen_header(context, typechecked_expressions);
    }));

    auto output_path_view = StringView::from_c_string(output_path);
    auto header_name_fallback = TRY(
        StringBuffer::create_fill(output_path_view, ".h\0"sv));

    header_output_path
        = header_output_path ?: header_name_fallback.view().data;

    auto header_file
        = TRY(Core::File::open_for_writing(header_output_path));

    TRY(bench("write header"sv, [&] {
        return header_file.write(header);
    }));

    TRY(bench("move_file"sv, [&] {
        return move_file(output_path, temporary_file);
    }));

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
    auto compiler = Core::System::getenv("CC"sv);
    if (!compiler) {
        if (!TRY(Core::System::has_program("cc"sv))) {
            return Error::from_string_literal(
                "Could not find a C compiler. Try setting the 'CC' "
                "environment variable");
        }
        compiler = "cc";
    }
    c_string argv[] = {
        compiler.release_value(),
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

static ErrorOr<StringBuffer> namespace_from_path(StringView path)
{
    auto namespace_ = TRY(StringBuffer::create(path.size));

    while (path.starts_with("../"sv))
        path = path.shrink_from_start("../"sv.size);

    auto parts = TRY(path.split_on('/'));
    for (u32 i = 0; i < parts.size() - 1;) {
        if (parts[i + 1] == ".."sv) {
            i += 2;
            continue;
        }
        TRY(namespace_.write(parts[i], "$"sv));
        i++;
    }
    TRY(namespace_.write(parts.last().shrink(".he"sv.size)));
    namespace_.replace_all('-', '_');

    return namespace_;
}
