# Chapter 1

## Table of content
  - [Project structure](#project-structure)
  - [Make](#make)
  - [Some GCC options and usage](#some-gcc-options-and-usage)

## Project Structure

As with every project, having some kind of structure and organization is a must, to keep things manageable and modular. Here we're going with a typical C project structure, meaning:
- `src`: a directory for the source code, will contain all of our `.c` files.
- `include`: a directory for the header files `.h`, we call it include because we include them in the source code. These files contain function declarations, type definitions and constants. You don't have to declare every implemented function or type definition or constant, only those you want to share. If in some source file you're using an auxiliary function that's not needed to be exposed to other parts of your program, then you might not include it in the header files.
- `build`: a directory for the build artifacts, in our case it'll only be `.o` files which are the result of an intermediary step before linking them all together into the final executable. It's a good practice to have this so that you do incremental builds and only recompile the files that changed.
- `bin`: a directory to store the executable file (or files). It's just good practice to separate the executables from the source code and keep the repository clean.

We'll also have a `Makefile`, which is a file that defines how the project is built. 

Initially this looks like:
```
.
├── LICENSE
├── Makefile
├── README.md
├── bin
├── build
├── include
└── src
    └── main.c
```

## Make

We learn from the [GNU Make](https://www.gnu.org/software/make/) page that `make` is a is a build automation tool that is not constrained to building programs, but can be used for automating a wide array of use cases. Thus, it is also not constrained to C/C++. As long as you can write down how to do what you want to do, `make` will do it.

`make` reads a file called `Makefile`, in which you write down how to build your program for example (or run tests or generate documentation etc.).

One other great thing about `make` is that it'll only recompute the files that depend "directly or indirectly" from source files that have changed.

Let's write our `Makefile`. Our goal is to build a *target* right, which is our executable. But, in order to do that, we need the object files first, which are the *dependencies* of our target. And when we get our hands on those dependencies, we can run the appropriate `gcc` *command* to build our target (or a set of commands in other cases). This is what's called as a *rule* in `Makefile`.

From the [GNU Make](https://www.gnu.org/software/make/) page:
> A rule in the makefile tells Make how to execute a series of commands in order to build a target file from source files. It also specifies a list of dependencies of the target file. This list should include all files (whether source files or other targets) which are used as inputs to the commands in the rule.

And a *rule* looks like this:
```
target:   dependencies ...
          commands
          ...
```

In our case it's going to be:
```
bin/edu_picodb: build/*.o
	gcc build/*.o -o bin/edu_picodb
```

Now we need to specify to `make` how to build the `build/*.o` files.
```
build/*.o: build/*.c
	gcc -c src/*.c -o build/*.o
```

Running `make` works (for the moment), but if you look at the `build` folder, you'll see a `*.o` file. This is bad, it should be `main.o`. What we did here wouldn't work if we had multiples `.c` files.

A better way of doing this would be:
```
bin/edu_picodb: build/main.o
    gcc build/main.o -o bin/edu_picodb

build/main.o: src/main.c
    gcc -c src/main.c -o build/main.o
```

Running `make` works again and now we have the `main.o` file in `build`, but what if we had many source files? It's not practical to do and it makes the `Makefile` cluttered:
```
bin/my_program: build/main.o build/file1.o build/file2.o ...
    gcc build/main.o build/file1.o build/file2.o ... -o bin/my_program

build/main.o: src/main.c
    gcc -c src/main.c -o build/main.o

build/file1.o: src/file1.c
    gcc -c src/file1.c -o build/file1.o

build/file2.o: src/file2.c
    gcc -c src/file2.c -o build/file2.o
...
```

Fortunately, `make` has *variables* and *functions*.

We can define a `SRCS` variable like using the `wildcard` function like this: `SRCS = $(wildcard src/*.c)`.

From the [text functions documentation](https://www.gnu.org/software/make/manual/make.html#Text-Functions):
>**$(wildcard pattern)**  
The argument pattern is a file name pattern, typically containing wildcard characters (as in shell file name patterns). The result of wildcard is a space-separated list of the names of existing files that match the pattern.

The '*' here is the typical wildcard character you'd encounter in Bourne shell. Do not confuse it with the `$*` [automatic variable](https://www.gnu.org/software/make/manual/make.html#Automatic-Variables).

So `$(wildcard src/*.c)` expands into a list of all the `.c` file paths in `src`.

We can also define variables without using functions, like `SRC_DIR = src`, and use it in our `SRCS` variable like: `SRCS = $(wildcard $(SRC_DIR)/*.c)`.

And now we need something to transform each `.c` from `src` into its `.o` equivalent in `build`.

Well, there's another function called `patsubst` that takes a *pattern*, a *replacement* and a white-space separated *text*. Every word in text that matches the pattern gets replaced with whatever is in replacement. Remember, our `SRCS` is a spaced-separated list, so we can use it as the "text" in `patsubst`. The pattern is `$(SRC_DIR)/%.c`, which is a bit confusing because `patsubst` uses '%' as the wildcard character instead of '*'. And the replacement is `build/%.o`. We can have a `BUILD_DIR = build` variable and use it there as well. So the variable that will hold our intermediary object files is `OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))`.

Now, in our `Makefile` we can do this and use our variables:
```
SRC_DIR = src
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

bin/edu_picodb: $(OBJS)
	gcc $(OBJS) -o bin/edu_picodb

build/main.o: src/main.c
	gcc -c src/main.c -o build/main.o
```

But we can't replace the rule with something like:
```
$(OBJS): $(SRCS)
	gcc -c $(OBJS) -o $(SRCS)
```

It won't work, we need to compile each object file individually. Fortunately, `make` provides what's called [pattern rules](https://www.gnu.org/software/make/manual/make.html#Pattern-Intro). And, as you can guess, they're rules that use patterns:
> A pattern rule contains the character ‘%’ (exactly one of them) in the target; otherwise, it looks exactly like an ordinary rule. The target is a pattern for matching file names; the ‘%’ matches any nonempty substring, while other characters match only themselves.

So we can do something like:
```
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
      gcc -c $< -o $@
```

The `$<` and `$@` are automatic variables. You can check the [automatic variables link](https://www.gnu.org/software/make/manual/make.html#Automatic-Variables) to get a full list of automatic variables and how they work. You should keep in mind that they're limited to the scope of the rule and they're computed afresh each time the rule is executed.

The `$@` is the file name of the target of the rule, so the name of a `.o` file. The `$<` is the name of the first dependency, so a `.c` file. So what `gcc -c $< -o $@` is compile the source file obtained by `$<` (which is the automatically substituted, thanks to '%', name for the first dependency) into the target output object file named by `$@`.

Our complete `Makefile` for the moment is:
```
SRC_DIR = src
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

bin/edu_picodb: $(OBJS)
	gcc $(OBJS) -o bin/edu_picodb

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	gcc -c $< -o $@
```

And running `make` works. But it lacks many things. First, we're not including heading files. Let's do that. We'll define an `INCLUDE_DIR` variable and use it inside the rule for compiling object files:
```
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

bin/edu_picodb: $(OBJS)
	gcc $(OBJS) -o bin/edu_picodb

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	gcc -c $< -I $(INCLUDE_DIR) -o $@
```

We can also replace a bunch of things with variables, that way we can override their values when invoking `main`. What people use the most are the `CC` and `CFLAGS` variables for the C Compiler and its options. In our case we'll use `CC = gcc` and `CFLAGS = -std=gnu17`, which is the C17 standard plus some GNU extensions. We'll add some options in the next section. We'll also use a `BIN_DIR = bin` variable and `TARGET = $(BIN_DIR)/edu-picodb` variable just to standardize things, I doubt we'll have to change this when invoking `main`. 

Now our `Makefile` is:
```
CC = gcc
CFLAGS = -std=gnu17

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
BIN_DIR = bin
TARGET = $(BIN_DIR)/edu-picodb

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -I $(INCLUDE_DIR) -o $@
```

So this is a clean, full working `Makefile`. Invoking `make` will do what we wanted it to do. But there's one last thing we can add, what I'd call "convenience" targets. Technically, they're called [phony targets](https://www.gnu.org/software/make/manual/make.html#Phony-Targets) and they don't correspond to an actual file. You declare them by adding to your `Makefile`: `.PHONY: phony_target_1 phony_target_2 ...`, which tell `make` that `phony_target_1` etc., aren't files.

I call them "convenience" targets because they help automate a bunch of things. There are many that are common like `clean`, `run`, `all` etc.

`clean` will clean build artifacts, `run` will rebuild, if necessary, the target and run it, and `all` will do a full rebuild and run. We can make these targets do anything we want really:
```
CC = gcc
CFLAGS = -std=gnu17

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
BIN_DIR = bin
TARGET = $(BIN_DIR)/edu-picodb

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: clean run all

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -I $(INCLUDE_DIR) -o $@

clean:
    rm -rf $(OBJS) $(TARGET)

run: $(TARGET)
    ./$(TARGET)

all: clean run
```

It's not necessary to declare the phony targets, but it's recommended to do so to prevent name conflicts with actual files for example.

This concludes our section on `make`, I hope it was useful for you and made you familiar with `make` a little bit in a way that makes the GNU Make documentation less daunting.

## Some GCC options and usage

### Standard

Here we'll just explore some common GCC options used for development.

`-std`: this controls which language standard to use. You can find all the information on this [here](https://gcc.gnu.org/onlinedocs/gcc/Standards.html). According to the documentation, the default is `-std=gnu23` which is the C23, published in 2024 as ISO/IEC 9899:2024, plus GNU extensions. You can find all the extensions to the C language family [here](https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html).  
If you want strict conformance without language extensions to a given standard, you should do something like `-std=c{version} -pedantic` for strict `C{version}` (or `-pedantic-errors` for errors instead of warnings). You can find all the options for controlling the C dialect [here](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html).  
There is a good mention in [GCC standards link](https://gcc.gnu.org/onlinedocs/gcc/Standards.html) about the two major "environments" or "classes" of conforming implementations. I, personnally, find it's cool to know such things.  
Note, not all standards are available on all GCC versions obviously, the C23 might not be available on older versions for example.

### Warnings

Now, we'll look at [options to request (or suppress) warnings](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html):

`-Wall`: it tells GCC to enable a broad set of warnings that the developers believe are the most useful to catch questionable or error-prone constructs. You'll find in the link a list of what it caches.  
Beware though, despite its name, it does not literally enable all possible warnings. It enables what the GCC maintainers consider the most valuable warnings that might help improve your code and help you catch errors prematurely without overwhelming you with too false positives or style-related suggestions.  
You should also know that some of these warnings have different levels of configuration, and you can find on the link the levels that `-Wall` uses by default.  
You can also individually disable some warnings that are included in `-Wall` or add warnings that are not included in it.
`-Wall` can be used with `-Wextra` to enable additional warnings that are either more specialized or that can cause many false positives etc. So I recommended you read about them and pick up what you think suits your codebase and use case.

`-Wextra`: it enables stricter checks that can be coding style "issues" or that can help find subtle bugs. It can be "noisy" and produce too many warnings, sometimes even false positives so many people don't enable `-Wextra`.  
An example that `-Wextra` catches but `-Wall` doesn't:
```c
#include <stdio.h>

int x_is_inferior_to_y(int x, unsigned int y) {
    if (x < y) {
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    int x = -10;
    // printf("Unsigned x: %u\n", (unsigned int) x);
    int y = 20;
    int comparison_result = x_is_inferior_to_y(x, y);
    if (comparison_result == 0) {
        printf("x is inferior to y.\n");
    } else {
        printf("x is not inferior to y.\n");
    }
    return 0;
}
```
Invoking `make "CFLAGS = -std=gnu17 -Wall"` builds the program with no issues. And surprise surprise, when you run `bin/edu-picodb` you get "x is not inferior to y.". This is because when a signed integer is compared to an unsigned integer, the signed integer is implicitly converted to an unsigned integer before the comparison. If you uncomment the `printf("Unsigned x: %u\n", (unsigned int) x);` line, you'll see that the unsigned value of `x` is $4294967286$, which is obviously not inferior to `y`.  
So `-Wall` didn't catch that. What if we add `-Wextra`? Invoking `make "CFLAGS = -std=gnu17 -Wall -Wextra" all` gives:  
```
rm -rf  build/main.o bin/edu-picodb
gcc -std=gnu17 -Wall -Wextra -c src/main.c -I include -o build/main.o
src/main.c:4:11: warning: comparison of integers of different signs: 'int' and 'unsigned int' [-Wsign-compare]
    4 |     if (x < y) {
      |         ~ ^ ~
src/main.c:10:14: warning: unused parameter 'argc' [-Wunused-parameter]
   10 | int main(int argc, char *argv[]) {
      |              ^
src/main.c:10:26: warning: unused parameter 'argv' [-Wunused-parameter]
   10 | int main(int argc, char *argv[]) {
      |                          ^
3 warnings generated.
gcc -std=gnu17 -Wall -Wextra  build/main.o -o bin/edu-picodb
./bin/edu-picodb
Unsigned x: 4294967286
x is not inferior to y.
```
So it does compile and run but it warns us "comparison of integers of different signs: 'int' and 'unsigned int'" thanks to the `-Wsign-compare` warning.  
But as you can notice, it also warns us about unused parameters. Which is a hassle. We can turn that off with `-Wno-unused-parameter`.  
If you don't want the code to compile when it encounters errors, you can add `-Werror`.

You can come up with many similar examples by looking at the warnings of `-Wextra` that are not part of `-Wall`, I encourage you try that! Note that the documentation mentions "C++ and Objective-C++ only" neary `-Wsign-compare` in both `-Wall` and `-Wextra` but in the specific documentation for `-Wsign-compare` it says "In C, it is also enabled by -Wextra".

And keep in mind that some codebases do write code in a way that might seem sketchy to the compiler but that is legitimate so you should weigh the benefits and the drawbacks and don't hesitate to select individually the warnings that seem the most concerning to your project.

So going back to my `Makefile`, I'll change `CFLAGS = -std=gnu17` to `-std=gnu17 -Wall -Wextra -Werror -Wno-unused-parameter`.

### Static analysis

Let's examine what the documentation of GCC says about static analysis:
> This option enables an static analysis of program flow which looks for “interesting” interprocedural paths through the code, and issues warnings for problems found on them.
<br><br>
This analysis is much more expensive than other GCC warnings.
<br><br>
In technical terms, it performs coverage-guided symbolic execution of the code being compiled. It is neither sound nor complete: it can have false positives and false negatives. It is a bug-finding tool, rather than a tool for proving program correctness.

Let's dissect it:
- First, we learn that it's not a run time thing, thus the name static analysis. It looks at the source code as it is. 
- It looks for "interesting" paths, this just means that it looks for chains of calls and conditional branches in your code that might lead to bugs. E.g. calling `malloc` and somewhere down the line forgetting to free it, or calling `close` twice on the same file descriptor etc. The interesting here means that if the path of code looks safe or if it just repeats what's already been explored by the static analyzer, then it might not explore it in details. "Interprocedural" means the interaction between different functions and branches. E.g., if you have a branch within function where it calls two different functions, the analyzer will try to see how data flows from the top function to the functions within the branches. 
- "Symbolic execution" means that it looks at the variables and data as symbols and not their concrete values. E.g., if you have a branch like `if (x > 0)`, the analyzer will consider `x` as a symbol rather than its actual content / data and explore both paths.    
- "Coverage-guided" means that the analyzer tries to explore every "interesting" path possible.
- Static analysis can lead to both false positives and false negatives. It's not perfect. It helps to find bugs, but if you static analysis didn't find any bugs doesn't mean you don't have any issues with your code. Even a 100% coverage doesn't guarantee correctness of the code. So please be wary. Especially when it comes to concurrency.
- Static analysis is expensive so it might not be something you run a lot but maybe in some circumstances like during CI, before your PR, when working on a critical part of the code etc.

Note: this option is not available on all platforms.

Here is a somewhat wicked example that plays on the drawbacks of static analysis because it uses concurrency and I think the analyzer from `-fanalyzer` doesn't have that capability yet. It was hard to come up with an example off the top of my head within its capabilities.

```c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int *global_ptr = NULL;

void *thread1_func(void *arg) {
    global_ptr = malloc(sizeof(int));
    if (!global_ptr) {
        fprintf(stderr, "malloc failed\n");
        pthread_exit(NULL);
    }

    *global_ptr = 42;

    usleep(1000);

    free(global_ptr);
    global_ptr = NULL;

    return NULL;
}

void *thread2_func(void *arg) {
    printf("Value read by thread2: %d\n", *global_ptr);

    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
```

Running this with `make "CFLAGS = -std=gnu17 -fanalyzer" all` will compile with no errors, and sometimes it'll output "make: *** [Makefile:25: run] Segmentation fault (core dumped)" and sometimes "Value read by thread2: 42". This is just a common case of data-race combined with use-after-free concurrency problem: there is no locking or memory barrier ensuring thread2 sees a valid pointer.

So static analysis didn't catch this. But once again, I think concurrency is not part of its capabilities yet but even within what it can do, it might not find some issues. 

**What is the difference between static analysis and the usage of `-Wall` and `-Wextra`?**

Well, static analysis runs a deeper and different kind of analysis than your typical warnings from `-Wall` or `-Wextra`. It performs coverage-guide symbolic execution and "interesting" interprocedural path exploration to quote the documentation. So it kind of simulates how data flows through your program which makes it capable of catching subtle issues. But the tradeoff is that it takes longer. Which is not the case for `-Wall` and `-Wextra` that look, primarly, for sketchy code constructs (like the comparison of a signed integer with an unsigned one). So these two look at the surface of the code, but they're quicker to run.  
Remember that both static analysis and `-Wall`/`-Wextra` and any other tool that will only look at the code in a static way are not perfect. They'll have both false negatives and false positives.

### Other options

So there are other options that are commonly used, some are:
- `g`: with this flag enable gcc will include debug information in the generated binary. You can read more about other debugging options [here](https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html). It's common to use with `-g` the `Og` optimization option:
- `Og`: from the [documentation](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html):
> Optimize debugging experience. -Og should be the optimization level of choice for the standard edit-compile-debug cycle, offering a reasonable level of optimization while maintaining fast compilation and a good debugging experience. It is a better choice than -O0 for producing debuggable code because some compiler passes that collect debug information are disabled at -O0.  
Like -O0, -Og completely disables a number of optimization passes so that individual options controlling them have no effect. Otherwise -Og enables all -O1 optimization flags except for those that may interfere with debugging.

So it retains some optimizations from `-O1` so the code runs faster than `-O0` but retains sensible debugging information so that when debugging you can check every single line in your code without confusion.  
- Then you have the other optimization flags:
  - `-O0` which is the default, it generates code that very closely reflects the source, which makes debugging straightforward, and is used when compilation time matters more than runtime performance. 
  - `-O` or `-O1` which enables a small set of optimizations in a way to trade-off the compile time and memory use. Compilation time is longer than `-O0` but not that much, and it tries to improve performance and reduce code size. 
  - `-O2`, I think is the *go-to* for production builds if you want good performance without pushing as aggressively as with `-O3`. It turns on nearly all supported optimizations that do not involve a space–speed tradeoff. 
  - `-O3` is used when performance is desired above all else (e.g. scientific computing), and where a potential increase in compile time and code size is acceptable. I think many general-purpose code won't benefit much from this flag while compute intensive programs will gain a lot more from it.
  - `-Os`: from the documentation: "Optimize for size. -Os enables all -O2 optimizations except those that often increase code size".
  - `-Ofast`: uses same optimizations as `-O3` so it's highly aggressive and on top of that disregards some strict standard compliance by using some transformations that might not be valid within the standard, especially around floating-point arithmetic. This is very specialized and may break many programs that rely on strict compliance to floating-point behavior etc.
  - `-Oz`: "-Oz behaves similarly to -Os including enabling most -O2 optimizations".

I only scratched the surface of the most used options and even in the above mentioned options there are way more things to know about than what I talked about so don't hesitate to consult [GCC command options' documentation](https://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html).
