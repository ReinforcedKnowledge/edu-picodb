# edu-picodb

This is a lightweight, educational command-line database utility written in C. Designed for my own personal learning and experimentation.

It allows users to create and manipulate simple database files with custom-defined schemas.

It operates on simple binary files, storing data according to a custom-defined schema. The focus is on exploring C features, such as pointers, memory management, file I/O, unions, enums, and more.

## Table of content
- [Project objective and introduction](#introduction)
- [Chapter 1:](#chapter-1)
  - [Project structure](#project-structure)
  - [Makefile](#makefile)
  - [Some GCC options and usage](#some-gcc-options-and-usage)
  - [Command line parsing](#command-line-parsing)
- [Chapter 2: File operation and function design](#chapter-2)

## Introduction

### Why This Project?

This project is purely educational and its goal is not to create a database management system or understand the fundamentals of such systems, but to understand some fundamentals of C that go beyond its syntax.

I wanted to aggregate whatever bits of knowledge I learned while using C. I tried to put together all of my old notes, what I learned by consulting various material, and my own learnings after making mistakes into a series of chatpers, each targeting one piece of code from an old school project that I revisited. 

This is also an opportunity for me to revisit some C concepts such as pointer arithmetic, memory allocation patterns, file I/O, and apply some concepts such as struct packing, better software design by separating responsibilities and passing status codes around to signal success or error etc.

And for you, if you're new to C, you might find it helpful to look at each part of the code—from schema parsing to file header management—and see how pointers and memory addresses are manipulated. It'll reinforce your familiarity C fundamentals and maybe you'll discover some patterns. And you can read my notes in parallel to understand the thoughts behind the code.

Please understand that this project, won't make you a C expert or advanced C developer. But if you're a beginner, maybe you can absorb what I've learned to accelerate your own learning in that stage.

And if you're already an expert in C, I don't think there is anything you can learn from this, but I'd love to learn from you. So if you could give a quick peek here and there and give me some advice and recommendations, I'd be really thankful!

This project assumes you're familiar with C syntax and the compilation process.

### Command-Line Options

- `-f <file_path>`: Specify the path of the database file to open or create.
- `-n`: Create a new file (requires a schema).
- `-s <schema>`: Provide a schema, in parentheses, for the database. Example: `(mycol1:int mycol2:float mycol3:string)`.
  - Column names can start with a letter or underscore but not digits.
  - Data types can be 'int', 'float', or 'string'.
  - Parentheses are compulsory.
  - Columns are space-separated.
- `-a <row>`: Provide the values for a row in parentheses. Each value is separated by " && " (space, ampersand-ampersand, space). For instance: `(123 && 4.56 && hello)`.

### Design

1. Header  
   The file header contains a magic number, version number, the total number of rows, and the number of columns. It also stores the columns’ metadata (name length, name, data type).

2. Column  
   Each column is defined by its name length, name and a data type (int, float, or string).

3. Row  
   A row is simply the number of cells (columns) and the list of cells (column values).

4. Cell  
   Every cell stores its type (int, float, or string) and the value. For strings, the length is tracked as well.

Please note that many limitations exist—there is no support for updating or deleting rows or columns, no key constraints, no advanced search, and no concurrency (yet).
  
### Limits:
- The data types are limited to `int` (which is a `uint32_t` behind the scenes), `float` (just `float`) and `string` (which is a `char` array with a maximum length is the maximum number that can be represented in `uint32_t`, which is $4294967295$).
- All CRUD operations are not implemented except for appending rows.
- No column name unicity verification.
- No concepts of key, primary key and foreign key.
- Number of columns in a file is limited.
- No search feature.
- Can't store whatever name as a column name. As for string values only ASCII is accepted.
- The header is quite simple.

These limits are due to this being a school project which I'm revisiting to augment with what I learned after using C professionnally.

## Chapter 1

### Project Structure

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

### Makefile

### Some GCC options and usage

### Command Line Parsing

## Chapter 2

1 - First thing I got to learn is to store strings' lengths!