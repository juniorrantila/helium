#include "Main.h"
#include "Core/File.h"

int main(int argc, c_string argv[])
{
    auto result = Main::main(argc, argv);
    if (result.is_error()) {
        (void)Core::File::stderr().writeln(result.error());
        return 1;
    }
    return result.value();
}
