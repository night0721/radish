# Grammar for Radish

> Extremely experimental and subject to completely change

## Hello World

```rd
// import io
const io = #import("io");

// print hello world
pub fn main() -> {
    io.println(stdout, "hello, world!");
}
```

## Variables

```rd
pub fn main() -> {
    // default immutable
    let x = 3;
    // mut keyword gains mutability
    let mut y = 4;
    
    y += x;

    // non-null by default
    let z? = nil;

    // type can be explicitly denoted with `let <name>: <type>`
    let score: i32 = 10;

    // strings are by default 0 terminated u8 array
    let str = "hello world";

    // explicit 0 terminated
    let greet: u8[:0] = "hello world";

    // explicit 2 terminated
    let greet2: u8[:2] == "hello again";
}
```

## some maths

```rd
fn x3summation(n: i32) i32 -> {
    return n * (n + 1) * (2 * n - 1) / 3;
}
```

```rd
// can use single statement instead of block
fn abs_deg(deg: f64) f64 -> deg % 360;
```

## if statements

```rd
fn greet(is_human: bool) -> {
    if is_human {
        io.println(stdout, "Hello, human!");
    } else {
        io.println(stdout, "Hello, robot!");
    }
}
```

```
fn greet_more(is_human: bool) -> {
    // if statements can return values
    let target = if is_human "human" else "robot";
    io.println(stdout, "Hello, {s}!", target);
}
```

## Loops

```
fn game_loop() -> {
    loop {
        game_logic();

        if need_exit() {
            break;
        }
    }
}
```

```
fn number_print() -> {
    for i in 0..=10 {
        if i == 3 {
            continue;
        }
        
        // prints 0 to 10 inclusive
        io.println(stdout, "{}", i);
    }
}
```

## Union return

```
fn read_input() u8[:0] | IOErr -> {
    // takes input from stdin until newline or eof, no newline included
    return io.scan_line(stdin, 0);
}
```

## Switch case

```
fn number(n :i32) u8[:0] -> {
    return match n {
        0 -> "zero";
        1 -> "one";
        2..=9 -> "single digits";
        n >= 10 -> "big numbers";
        _ -> "negative numbers";
    }
}
```

## Pattern matching

```rd
struct User {
    name: u8[:0]
}

fn username() User | CreationFailed -> {
    // pattern match to return username or failed
    return match io.scan_line(stdin, 0) {
        u8[] line -> User{name: line};
        IOErr -> CreationFailed{err.st()};
    }
}
```

## Type Alias

```rd
type Success = i32;
type Failure = IOErr | ParseErr | UserNotFoundErr;
type Signal = Success | Failure;

fn get_a_signal() Signal -> {
    // do a lot of stuff
    // open files
    if cannot_open return IOErr{err.st()};

    // parse data
    if failed_parsing return ParseErr{err.st()};

    // get user
    if cannot_find_user return UserNotFoundErr{err.st()};

    // integer data found
    return data;
}

fn signal_handler(signal: Signal) -> {
    match signal {
        Success data -> io.println(stdout, "{}", data);
        UserNotFoundErr -> io.println(stdout, "user not found");
        _ -> io.println(stdout, "internal error");
    }
}
```

