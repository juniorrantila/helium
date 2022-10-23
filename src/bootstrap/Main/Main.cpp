#include "Main.h"

int main(int argc, c_string argv[])
{
    auto result = Main::main(argc, argv);
    if (result.is_error()) {
        result.error().show();
        return 1;
    }
    return 0;
}

