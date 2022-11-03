# The Helium programming language

**Helium** is a light systems programming language.

It currently transpiles to C.

**NOTE** If you're cloning to a Windows PC (not WSL), make sure that
your Git client keeps the line endings as `\n`. You can set this as
a global config via `git config --global core.autocrlf false`.

## Build instructions

This project is only tested to compile with the `clang` compiler on
linux and macOS. You will need to install it along with `meson` and
`ninja`. For faster rebuilds you may install `ccache` as well.

### Setup:

```sh
meson build
```

### Build:

```sh
ninja -C build
```

After building, the Helium executable will be found in
`./build/src/bootstrap`.

When developing you may want to run the following command:

```sh
. meta/environment.sh
```

This will add `/path/to/src/bootstrap` to your `PATH`, meaning you
will be able write `helium` instead of `./build/src/bootstrap/helium`
to execute the compiler.

## Usage

Compilation currently requires a C compiler. Make sure you have one
installed. Helium will use the one set by the `CC` environment variable
if it's set, otherwise it defaults to the program `cc`.

```sh
helium file.he
./a.out
```

## Goals

1. Being light
2. Compile performance
3. Code readability
4. Easy interoperability with C
5. Executable performance
6. Fun!

## Being light

Helium wants to be like C in its simplicity, meaning the programmer
should be able to understand how data is structured and operated on
very easily. On top of this, Helium does not aim to be a
"batteries included" language, but rather one where the programmer
is expected to write their own library to fit their own needs.

## Code readability

Far more time is spent reading code than writing it. For that reason,
**Helium** puts a high emphasis on readability.

Some of the features that encourage more readable programs:

- [ ] Immutable by default
- [ ] Member functions
- [ ] Argument labels in call expressions (`object.function(width: 10, height: 5);`)
- [ ] Inferred `enum` scope. (You can say `Foo` instead of `MyEnum::Foo`)
- [ ] Pattern matching with `match`
- [ ] None coalescing for optionals (`foo ?? bar` yields `foo` if `foo` has a value, otherwise `bar`)
- [ ] `defer`, `errdefer` statements
- [ ] Pointers are always dereferenced with `.` (never `->`)
- [ ] Error propagation with `ErrorOr<T>` return type and dedicated `try` / `must` keywords

## Function calls

When calling a function, you must specify the name of each argument
as you're passing it:

```helium
rect.set_size(width: 640, height: 480)
```

There are two exceptions to this:

- [ ] If the parameter in the function declaration is declared as `anon`, omitting the argument label is allowed.
- [ ] When passing a variable with the same name as the parameter.

## Structures

There are five structure types in Helium:

- `struct`
- `c_struct`
- `enum`
- `union`
- `variant`

### `struct`

These are like structs in C, except they may reorder their fields
to make the type smaller.

Basic syntax:

```helium
let Point = struct {
    x: i64,
    y: i64,
};

impl Point {
    fn size(self: Self) {
        return sqrt(self.x * self.x + self.y + self.y);
    }
}
```

### `c_struct`

These are like `struct`s, except the memory layout is exactly as it
would be in C.

### `enum`

Like `enum class` in C++.

### `union`

Like `union` in C.

### `variant`

Like algebraic enums in `rust`.

```helium
let SomeVariant = variant {
    some_i32: i32,
    foo: Foo,
};

let some_variant: SomeVariant = Foo {
    .a = 42,
    .b = 11,
};

match some_variant {
    some_i32 => {
        printf("%d\n", some_i32);
    }
    foo => {
        printf("%d, %d\n", foo.a, foo.b);
    }
}
```

### Member functions

All structure types can have member functions.

There are two kinds of member functions:

**Static member functions** don't require an object to call.
They have no `self` parameter.

```helium
let Foo = struct {
    a: i32,
    b: i32,
};

impl Foo {
    fn static_func() {
        printf("Hello!\n");
    }
}

// Foo::static_func() can be called without an object.
Foo::static_func();
```

**Normal member functions** require a self parameter to be called.
The programmer may specify how the self parameter should be passed
to the function.

```helium
impl Foo {
    fn get_a(self: Self) -> i32 {
        return self.a;
    }

    fn get_b(self: &Self) -> i32 {
        return self.b;
    }

    fn set_a(self: &mut Self, value: i32) -> void {
        self.a = value;
    }
}

let x = Foo {};
x.get_a(); // x is passed by value.

let y = Foo {};
y.get_b(); // y is passed by immutable reference.

var z = Foo {};
z.set_a(42); // z is passed by mutable reference.
```

## Id types

Id types are array indexes which may only be used to index an array
of matching type.

```helium
let foos = [
    Foo {
        .a = 42,
        .b = 11,
    },
];
let foo_id: [Foo] = 0;
foos[foo_id].a; // Yields 42.

let bars = [
    Bar { },
];
bars[foo_id]; // Error.
```

## Error handling

Functions that can fail with an error instead of returning normally
are marked with trailing `!ErrorType` in their return type:

```helium
fn task_that_might_fail() -> u32!Error {
    if problem {
        throw Error::from_errno(EPROBLEM);
    }
    // ...
    return result
}

fn task_that_cannot_fail() -> u32 {
    // ...
    return result
}
```

Unlike languages like C++ and Java, errors don't unwind the call
stack automatically. Instead, they bubble up to the nearest caller.

When calling a function that may throw you must precede the call
with either `must` or `try`, alternatively you may follow the call
with `catch` to handle the error manually.

```helium
try task_that_might_fail(); // Bubble up error to caller if any.

must task_that_might_fail(); // Abort on error.

task_that_might_fail() catch error {
    printf("Caught error: %s\n", error.message());
}
```

## Inline C

For better interoperability with C code, the possibility of
embedding inline C code into the program exists in the form of 
`inline_c` expressions and blocks:

```helium
inline_c struct stat st;
if fstat(some_file, &mut st) < 0 {
    throw Error::from_errno();
}

inline_c {
    void some_c_function() {
        printf("%s\n", __FILE__);
    }
}

some_c_function();
```

### Reference type syntax

- `&T` is an immutable reference to a value of type `T`.
- `&mut T` is a mutable reference to a value of type `T`.

### Reference expression syntax

- `&foo` creates an immutable reference to the variable `foo`.
- `&mut foo` creates a mutable reference to the variable `foo`.

### Closures feature list:

- [ ] Function as parameter to function
- [ ] Functions as variables
- [ ] Explicit captures

