# Chapter 1

## Table of content
  - [Command Line Parsing](#comamnd-line-parsing)
  - [Getopt](#getopt)
  - [Some Notes about the Code](#some-notes-about-the-code)
  - [Switch Statement](#switch-statement)
  - [Other Tidbits of Information](#other-tidbits-of-information)
    - [fprintf](#fprintf)
    - [Strings, arrays and pointers](#strings-arrays-and-pointers)

## Comamnd Line Parsing

The goal of this chapter is to parse command line options and arguments in order to code our logic around them.

We want the following options:
- `-f`: for *file*. It'll take a file path, so it requires an argument.
- `-n`: for *new*. It doesn't require an argument but it'll influence our logic.
- `-s`: for *schema*. It requires an argument and it's to provide the schema of the columns in the following format `(my_col1:int _my_col:float mycol:string)`.
- `-a`: for *append*. It also requires an argument and it's to provide the row values to append to the file in the format `(value1 && value2 && value3)`, e.g., `(123 && 1.4 && Hello world!)`.

When it comes to parsing, it's important to understand the arguments of `main`: `int argc` and `char *argv[]`.
- `int argc` (argument count): this is an integer that represents the number of command-line arguments passed to the program, **including the program's name**. E.g., if we invoke our program with `./bin/edu-picodb -f filename -n -s "(my_col1:int _my_col:float mycol:string)" -a "(123 && 1.4 && Hello world!)"` then `argc` will be equal to $8$.
- `char *argv[]` (argument vector): so this is an array of character pointers, each pointer points to an argument's name. In the example above, `argv[0]` points to the argument's name './bin/edu-picodb', so if you do `printf("argv0: %s\n", argv[0]);` you'll see `./bin/edu-picodb`. From the standard, `argv[argc]` is a `NULL` pointer.

You can print `argv`'s content like this:
```c
int main(int argc, char *argv[]) {
    printf("Argument count (argc): %d\n", argc);

    for (int i = 0; i < argc; i++) {
        printf("Argument at position: %d\n", i);
        printf("    Argument address: %p\n", (void *)argv[i]);
        printf("    Argument name: %s\n", argv[i]);
        printf("\n");
    }

    return 0;
}
```

Both `int argc` and `char *argv[]` don't differentiate between options and non-options. That's the job of the parsing logic you'll implement or the parsing library you'll use.

According to [this Stackoverflow post](https://stackoverflow.com/questions/9642732/parsing-command-line-arguments-in-c), besides doing the parsing yourself using `argv`, you can use [Getopt](https://www.gnu.org/software/libc/manual/html_node/Getopt.html) or [Argp](https://www.gnu.org/software/libc/manual/html_node/Argp.html). I'll go with **Getopt** since I'm mostly familiar with it and its fairly simple, though it has some limitations.

## Getopt

**Note**: when you try checking the man page for `getopt` make sure you're reading the library functions manual and not the user commands.

So according to [`getopt`'s man page](https://www.man7.org/linux/man-pages/man3/getopt.3.html), there are three `getopt` functions, `getopt`, `getopt_long` and `getopt_long_only`. We'll only be using `getopt` here which is practical for short options (one dash `-`). The other two functions are for long options (with two dashes `--`).

You can also see that there a bunch of external variables related to `getopt`, namely `char *optarg`, `int optind`, `int opterr`, `int optopt` and on top of `int argc` and `char *argv[]`, the `getopt` function also takes `const char *optstring` (argument of the function).

`const char *opstring` is the string tells `getopt` which characters are valid options, and which of those require or optionally accept arguments.

`int optind` is the **index of the next element to be processed** in `argv`. It is **initialized** to `1` because `argv[0]` is the name of the program. It's important to understand that it's the next element to be processed.

`char *optarg`: When an option that **requires** or **accepts** (optional) an argument is processed, `optarg` is set to **point to that argument**. If the option has an **optional** argument and none is provided, `optarg` is set to `NULL`.

**How does the function identify arguments?** (quoting the man page) "An element of `argv` that starts with '-' (and is not exactly "-" or "--") is an option element." **And how do we specify our options and which require arguments and which don't?** You do that thanks in `opstring`. E.g., `optsring = f:ns:a:`. The characters are the options and when a character is followed by a colon, then the option requires an argument. 

The way `getopt` finds the arguments might be a little bit confusing in the beginning so let me explain it here:
- If a character in `opstrings` **requires an argument** (is followed by a colon, e.g. the `-f` option in the example above), so `getopt` makes `optarg` point to the following text *in the same* `argv`-element (e.g., `-oarg`), *or the text of the following* `argv`-element (e.g., `-o arg`). So when an option requires an argument, `optarg` is never `NULL` unless you have provided that option at the end of your invokation and you didn't provide the argument, e.g., `./main -o` where `-o` requires an argument. In that case you'll get an error (we'll talk about errors below).
- But! If a character in `optstrings` is followed by two colons, meaning the option **takes an optional arg**, then if there is text in the current `argv`-element (i.e., in the same word as the option name itself, for example, `-oarg`), then it `optarg` will point to it, otherwise `optarg` is set to `NULL`. So if you invoke your program with the option `-o` that accepts an optional argument with `./main -o arg`, `getopt` won't recognize `arg` as the argument for `-o`.  
This is a GNU extension (we briefly mentioned them in the standards option for GCC in chapter 1).

There was also another point of confusion for me in the beginning, it's how `optind` is updated and the state of `argv` at the end of processing it. To quote the man page, "The variable `optind` is the index of the next element to be processed in `argv`." and that really summarizes and says a lot about how the function works. Because the next element to be processed in `argv` is either a required argument or an option (dash + character). Everything else is ignored. So `optind` is incremented as options and their arguments are processed.  
**During option parsing**:
- Increments by $1$ for each option processed or required/optional argument in the same `argv`-element (e.g., `-oarg`). 
- Increments by 2 when consuming a required argument that is not in the same `argv`-element as the option (e.g., `-o arg`).
- Increments by as much as there are non-option arguments until an option is found. (e.g., `-<o1> arg1 arg2 arg3 -<o2>`, then `arg2` and `arg3` are skipped if `o1` requires an argument, if not or if the argument is optional, then even `arg1` is skipped).

**After parsing (`getopt` returns `-1`)**:
- `optind` **points to the first non-option argument** in `argv`.

Nothing better than an example to illustrate this. Imagine this is your invokation: `./main -o1 arg1 arg2 arg3 -o2 arg4` with `optstring = "o1o2"`, so none of the options require or accept an argument.  
During processing `optind` will be, respectively, $1$ (initial value), $5$ (skips `arg1`, `arg2` and `arg3` to point to the first element to be processed here which is the option `-o2`), and at the end it'll be $3$ because **`argv` will be permuted** to `./main -o1 -o2 arg1 arg2 arg3 arg4`. 

**What if you wanted to provide positional parameters** (and in the same occasion not permute `argv` because now these parameters have a meaning)**?** Well you can start `opstring` with '-'. In that case, each non-option `argv` element is handled as if it were the argument of an option with character `1`. This will allow the program to accept options and arguments in any order while preserving their positions (`argv` is not permuted). 

The permutation of `argv` was another element of confusion. So remember that if you want to *rescan* `argv` with `getopt`, then you have to set it to $1$, and if you use `optind` as is (after scanning `argv` a first time) then it's going to be the first non-option argument. Keep in mind that it's only the non-option arguments (also called *positional parameters*) that are placed at the end (in order of appearance). So if you execute `./main -o arg1 arg2 arg3 -parg4 arg5` with `opstring = o:p::`, then `argv` at the end will be `./main -o arg1 -parg4 arg2 arg3 arg5`.

There are two other external variables that we didn't talk about:
- `int opterr`: **If non-zero**, `getopt` prints error messages to `stderr` when it encounters an unknown option or a missing option argument. By default it is `1`. You can suppress messages to `stderr` by setting `opterr` to `0`.
- `int optopt`: Contains the option character that caused an error, such as an unknown option or a missing required argument.

The behavior of how `getopt` handles errors changes with whether `opstring` starts with ':' or not, and the value of `opterr`. Here's how `getopt` it does that:
- Missing option: No error. We talked about that at the beginning of the section. You can't expect from `getopt` some kind of logic on combinations of presence / absence of options.
- Missing required argument:
    - If `optstring` starts with ':':
        - `getopt` returns ':'.
        - No output to `stderr`.
    - Else:
        - `getopt` returns '?'.
        - Output to `stderr` (if `opterr != 0`).
- Illegal/unknown option:
    - If `optstring` starts with ':':
        - `getopt` returns '?'.
        - No output to `stderr`.
    - Else:
        - `getopt` returns '?'.
        - Output to `stderr` (if `opterr != 0`).

When there is an error, whether `optstring` starts with `:` or not, `optopt` will contain the character that caused the error.

Personally, I like to be able to customize the error message myself so I'll be using ':' as first character of `opstring`. And I'll handle the output to `stderr` myself.

There is one last thing to know about `getopt`, earlier we said that it returns $-1$ when it finishes parsing. Well, that's if you don't encounter an error obviously, but also you can change the behavior so that `getopt` returns when it encounters the first non-option argument. You can do that by either setting the environment variable `POSIXLY_CORRECT` or by starting `opstring` with '+'.  
If POSIXLY_CORRECT behavior is required and we want to have `'+'` as an option, then `optstring` will contain two '+' symbols. Start `opstring` with '+' to enforces POSIX scanning mode and then put somewhere (even right after it second place) '+' to treat it as a normal option character.  
And, obviously, if '+' figures in `opstring` but is not the first character, then it's considered a normal option and the POSIX scanning mode is not turned on.

Also, the special argument `--` forces an end of option-scanning regardless of the scanning mode (so if `argv[optind] == '--'` then `getopt` returns -1).

This closes our coverage of `getopt`. You'll find the actual code that uses it in `main.c` (alongside the logic for checking if everything is correct).

**Disclaimer**: I haven't checked the source code of `getopt`. The above notes are what I gathered from understanding the map page and playing with the function on `Ubuntu 24.0.1 LTS`. So the behavior on your platform (if you're using MacOS for example) might be slightly different. Your best source is the man page and your own tests, but hopefully this section could help clarify some points of confusion where you might give your attention to.

## Some Notes about the Code

Notice how we evaluate the truthiness of the pointers, e.g., `if (!filepath)`, or, `if (schema)`. This wouldn't work if we didn't initialize our pointers as `NULL`. If we don't control the pointer's initialization and just do `char *filepath`, then it'll contain whatever arbitrary data happens to be at the arbitrary memory location that it'll point to at the time of declaration. We can't know the type of data or its value because the pointer could point anywhere in memory and it could point to a non-zero garbage value and make our condition of `if (!filepath)` fail.

But what we have done is still not enough. All, we're doing here is ensuring that the pointer is properly initialized to `NULL` and only used when non-`NULL`, it doesn't mean we're "safe". The pointer could still point to a non-valid memory location or contain "corrupt" data. We should properly verify its content before doing some operations on it (like printing its content). In our case we're getting the data from the CLI so all strings are null terminated, but if our program is used inside a script and it passes a non-null terminated schema for example and we try to print it out, then we'll get an undefined behavior and that's a possible security risk.

## Switch Statement

I wanted to add this section (and the following) for what a quick overview of `swtich` statements.

So `switch` statements can and are used as replacement of a long series of `if`/`else if`/`else` statements. They simplify the code and make it more readable, and sometimes they're faster. But they're limited to an integer controlling expression (meaning you can't evaluate a string against other strings for example).

I had this misconception that the compiler, in this case `gcc`, always converted `switch` statements into [jump tables](https://stackoverflow.com/questions/48017/what-is-a-jump-table) allowing the program to jump directly to the correct case (in $O(1)$ time whatever the case compared to almost $O(N)$ for the worst case scenario if `if`/`else if`/`else`). But **it's not always the case**! This author of this [article](https://xoranth.net/gcc-switch/) shows that the compiler might compressing multiple case labels into a bitset, transforming the `switch` statement into a jump table or transforming the `switch` statement into a binary decision tree depending on the density of your case labels and whether you're accessing a contiguous memory block or not across your case labels. I highly recommend the read as well as [this Reddit post for a quick overview](https://www.reddit.com/r/C_Programming/comments/195sn4c/how_does_the_compiler_construct_a_jump_table_map/).

We can only use `switch` statements with integer constant expressions, so no runtime variables or function calls. All integers work with `switch` statements, whether signed or unsigned, characters and enums! And it makes a lot of sense with enums because you'll get that dense case labels.

Make sure your case labels have the same type as your controlling expression, or at least make sure the comparisons hold if you convert the case labels in the type of your controlling expression because that's what happens. From the working draft — April 1, 2023, ISO/IEC 9899:2023:
> The integer promotions are performed on the controlling expression. The constant expression in
each case label is converted to the promoted type of the controlling expression.

So don't have negative case labels when your controlling expression is unsigned for example.

That's what I wanted to say about `switch` statements, but I'll just compile some other minor things you may or may not know about `switch` statements:
- There is an implementation limit of $1023$ case labels (excluding those for nested `switch` statements) in one `switch` statement. It's a guaranteed minimum but some compilers might allow for more. From the working draft — April 1, 2023, ISO/IEC 9899:2023:
> 5.2.4.1 Translation limits  
...  
— 1023 case labels for a switch statement (excluding those for any nested switch statements).
- You can't have two case labels with the same value in the same `switch` statement.
- You can have at most one `default` label in a single `switch` statement.
- `switch` statements don't require the `default` case. If you have one, then it'll run if no match was found, otherwise the program will run whatever statement comes after the `switch` one.
- Multiple labels can share the same block of statements:
```c
switch (my_int) {
    case 1:
    case 2:
    case 3:
        printf("Cases 1, 2 and 3 share this log.\n");
        break;
}
```
- If you don't use a jump statement (`continue`, `break`, `return`, `goto`) inside the cases you'll fall through all the cases starting from the case that matches your controlling expression. Meaning that if you do:
```c
switch (my_int) {
    case 1:
        printf("Case 1.\n");
    case 2:
        printf("Case 2.\n");
    case 3:
        printf("Case 3.\n");
}
```
And `my_int` is $2$, then both `printf("Case 2.\n");` and `printf("Case 3.\n");`.
- It's important to understand that your program jumps to the case that matches your controlling expression. So if you have code like this:
```c
switch (my_int) {
    case 1:
        printf("Case 1.\n");
    case 2:
        printf("Case 2.\n");
    printf("Some code here.\n");
    case 3:
        printf("Case 3.\n");
}
```
And `my_int` is equal to $2$, then `printf("Some code here.\n");` will run as well. But if `my_int` is equal to $3$, it won't.  
I have not encountered a use case before where you'd like to fall through some cases but if you happen to need that, make sure you do it correctly.

## Other Tidbits of Information

### fprintf
Using `fprintf(stderr, ...)` ensures that the user sees the message, and also it doesn't interfere with any pipelines or scripts that are processing your program’s standard output so it's cool, but if you're using its formatting capabilities make sure you do that correctly. E.g., if you have:
```c
#include <stdio.h>

int main() {
    printf("Normal message.\n");
    
    fprintf(stderr, "Error message.\n");
    
    return 0;
}
```
Running your program will show you both messages. You can redirect `stdout` to a file with `./main > out` and `stderr` to another file with `./main 2> err`. You can verify that each only contains the correct messages by doing `cat out` or `cat err`.

### Strings, arrays and pointers

Since we're going to work with strings a lot, it's important to know the basics about them.

"Valid" strings in C are null-terminated. It means that the last symbol in the array of characters that represents the string is the null-termination symbol `\0`.

- There are two ways to declare strings:
    - **String literals**: `char *my_str = "hello"` or `char my_str[] = "hello"`. These are utomatically null-terminated by the compiler. So the length of `my_str` is $6$ and not $5$. `char *my_str = "h"` is similar to 'h' plus '\0'.
    - **Character arrays**: `char my_str[] = {'h', 'e', 'l', 'l', 'o', '\0'};`. Here you have to take care of adding the null-termination symbol yourself. You can't use the character pointer syntax.

If you want to **get the value of an element**, you either index in the array `my_str[i]` or you can handle the pointer arithmetic yourself: you point to the element you want and dereference it, `*(my_str + i)`. **Be wary of operator precedence!** If you do `*my_str + i` it'll add `i` to the ASCII code to the first element of the string ('h' here) and give you a completely different character.

If you want to **get the address of an element**, then again you either index in the array `&my_str[i]`, or use the `my_str + i` pointer directly.

This works because *arrays are contiguous blocks of memory* **and** *arrays in C decay to the pointer to the first element*. Which means that the pointer to the array is exactly the same that points to the first element. **But! It's not always the case!** The array to pointer decay happens only in some contexts, like passing a character array as a function that expects a character pointer, pointer arithmetics etc. Let's examine two examples and explain them:

- Doing `sizeof(my_str)` with `char *my_str = "hello"` will lead to $8$ bytes (the size of the pointer), whereas doing it with `char my_str[] = "hello"` will lead to $6$ bytes (which is the size of the array because it contains $6$ symbols and each is $1$ byte, which is `sizeof(char)`).
- Try doing something like this:
    ```c
    #include <stdio.h>

    int main() {
        char *literal_ptr = "hello";
        char literal_arr[] = "hello";
    
        printf("literal_ptr\n");
        printf("Address of first element using index: %p\n", (void *)&literal_ptr[0]);  // address_1
        printf("Address of first element (no index): %p\n", (void *)literal_ptr);  // address_1
        printf("Address of array (using the variable): %p\n", (void *)literal_ptr);  // address_1 
        printf("Address of array (using ampersand): %p\n", (void *)&literal_ptr);  // address_2 =/= address_1

        printf("\n");

        printf("literal_arr\n");
        printf("Address of first element using index: %p\n", (void *)&literal_arr[0]);  // address_3
        printf("Address of first element (no index): %p\n", (void *)literal_arr);  // address_3
        printf("Address of array (using the variable): %p\n", (void *)literal_arr);  // address_3
        printf("Address of array (using ampersand): %p\n", (void *)&literal_arr);  // address_3

        return 0;
    }
    ```

You notice that sometimes the behavior between `char *` and `char (*)[size]` and other times it's not (`sizeof` and applying the ampersand unary operator `&`). 

**Why?** 
- When the behavior is similar (e.g., pointer arithmetics), the `char (*)[size]` decays to the pointer to the first element and acts similarly as `char *`. 
- When it's not, it's because `char (*)[size]` is actually a *"true" array*. It's not a pointer to an array. It declares an array on the (either global storage on the function's stack frame etc.). The array's name (`literral_arr`) *is* the array. So applying `sizeof` to it will return the size of the array. Applying the unary operator will return the address of the array which is the address of the first element. But in the case of `char * literal_ptr`, it's a pointer to an array, that's why `sizeof` returns the size of the pointer ($8$ bytes on my machine). It's a pointer stored somewhere that points to an array stored somewhere else and that's why `&literal_ptr` gives a different address than the address of the first element of the array.

The above description works for all arrays, not only strings, it just so happens that strings are arrays of characters.

Now, there's something specific to string literals. Doing `char literal_arr[] = "hello"` creates an array of characters, which you can modify, but doing `char *literal_ptr = "hello"` creates a pointer to a string literal and you can't modify it. The string literal lives in a special section of the program's memory called read-only data, `.rodata`, that's filled at compile time. Doing `literal_ptr[0] = 'H'` is undefined and can crash the program. Which is not the case of `literal_arr[0] = 'H'`, totally valid.
