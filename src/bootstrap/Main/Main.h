#pragma once
#include <Ty/Base.h>
#include <Core/ErrorOr.h>

namespace Main {

Core::ErrorOr<void> main(int argc, c_string argv[]);

}

int main(int argc, c_string argv[]);
