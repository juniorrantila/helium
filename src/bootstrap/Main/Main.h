#pragma once
#include <Ty/Base.h>
#include <Ty/ErrorOr.h>

namespace Main {

ErrorOr<void> main(int argc, c_string argv[]);

}

int main(int argc, c_string argv[]);
