# Chapter 3

## Table of content
  - [File Descriptors](#file-descriptors)
  - [`open` syscall](#open-syscall)
  - [O_CREAT and O_EXCL](#o_creat-and-o_excl)
  - [errno](#errno)
  - [Function Pattern](#function-pattern)
  - [Some Things to be Aware of](#some-things-to-be-aware-of)


## File Descriptors

In this chapter we're going to take a filepath provided through the `-f` option and open it in a read-write mode. If the user also supplies the `-n` option, we'll verify that the file doesn't already exist, and if it doesn't, we'll create it and open it in the read-write mode (otherwise, return an error).

We're going to use the `open` system call, that returns a file descriptor.

Let's understand this function. Quoting [the `open` man page](https://man7.org/linux/man-pages/man2/open.2.html):
> The return value of open() is a file descriptor, a small, nonnegative integer that is an index to an entry in the process's table of open file descriptors.

So from that we understand that a **file descriptor** is:
  - An integer
  - Non-negative
  - Small
  - Unique per process (since it's an index)

If you use Linux regularly, you're probably familiar with `lsof` which means "list open files" which lists files that are open and the processes that opened them. We can do something very simple to get an intuition of what is a file descriptor. Let's create a random file `my_file.txt` and let's run this program with `./main_proc`:
```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("my_file.txt", O_RDWR);
    printf("File descriptor number: %d\n", fd);
    sleep(120);
    close(fd);
    return 0;
}
```

('O_RDWR' means to open the file with read and write access but we'll get into that later)

You should see the number 3.

While the program is running, let's get the process's ID by doing `ps aux | grep main_proc` and then let's list its open files with `lsof -p <PID>`. You should see something like this:
```
COMMAND     PID     USER   FD   TYPE DEVICE SIZE/OFF     NODE NAME
main_proc 87873 rf.knowledge    0u   CHR   16,0 0t227392     1099 /dev/ttys000
main_proc 87873 rf.knowledge    1u   CHR   16,0 0t227392     1099 /dev/ttys000
main_proc 87873 rf.knowledge    2u   CHR   16,0 0t227392     1099 /dev/ttys000
main_proc 87873 rf.knowledge    3u   REG   1,16        0 50977813 /Users/rf.knowledge/fd_chapter/my_file.txt
```

What you see in this output is the name of the command/executable running, our case it's `main_proc`. You see its process ID, in this case 87873. You see the user under which the process is running (rf.knowledge). Then you have the **FD** column, which is "File Descriptor" and is composed of two things, a number (0, 1, 2 and 3) and a letter (just 'u' in our case). The number indicates the actual file descriptor. 0, 1 and 2 are, respectively, for standard input (`stdin`), standard output (`stdout`) and standard error (`stderr`). The 3 is the file descriptor for our file, `my_file.txt`, and you can see its path under the **NAME** column. The 'u' letter means that the program can both read from the file and write to it. I recommend you check the [man page for `lsof`](https://man7.org/linux/man-pages/man8/lsof.8.html) to know more about it (like the file descriptor is not always a number). Other common modes under which a file can be open are `r` for read access and `w` for write access. The **TYPE** column indicates the type of the file, in our case we have the "CHR" type which stands for "character special file" and the "REG" type which stands for "regular file".

In the most common cases, when you first open a file in your program it'll get the number 3, because numbers 0, 1 and 2 are reserved for `stdin`, `stdout` and `stderr`.

You should also know that file descriptors don't only describe files, but also sockets and pipes.

So we understand now that each process maintains its own table of open file descriptors and a file descriptor is just an index, an entry, into this table. Each new file descriptor takes the lowest available index in the process' file descriptor's table.

Now, let's do this `echo "hello" > my_file.txt ` and after the program wakes up, let's read its the content of the file:
```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    int fd = open("my_file.txt", O_RDWR);
    printf("File descriptor number: %d\n", fd);
    sleep(30);
    char *fcontent = malloc(6 * sizeof(char));
    int bytes_read = read(fd, fcontent, 5 * sizeof(char));
    fcontent[5] = '\0';
    printf("File content: %s\n", fcontent);
    close(fd);
    return 0;
}
```

(What the added three lines do is allocate $6$ bytes of memory to store the "hello" string's characters and the null-termination symbol, then we fill in that allocated memory the bytes from the file and add the null-termination symbol manually)

While the program is sleeping, let's delete the file, `rm my_file.txt`. You'll notice that when the program wakes up it prints "hello", even though the file was deleted in the meantime. I saw this once in a code that created hard-to-access temporary files. Because what's happening is, something similar to garbage collection, as long as there is a process that has an open file descriptor for that file, the underlying data, the inode, won't be wiped out. `rm my_file.txt` only removes the directory entry for the file. There is a good [layman explanation about inodes and files in this Reddit post](https://www.reddit.com/r/linux4noobs/comments/13g6h1m/what_exactly_are_inodes/). So, the file's data remains on disk as long as there are references (in our case the file descriptor from our program) to it, and our file will live as an "unlinked" file. I don't know how you can use this, but I already saw it used to make data unavailable to outside programs at one point, creating temporary files that you know will be cleaned by the OS. I don't think it's the best way for creating temporary files though, you can use the 'O_TMPFILE' flag in the `open` function if your filesystem supports that.

I think something similar happens if you change the pathname of the file. From the `open` man page:
> A file descriptor is a reference to an open file description; this reference is unaffected if pathname is subsequently removed or modified to refer to a different file.

## `open` syscall

Enough about file descriptors, let's check the `open` system call for a moment. The function signature is `int open(const char *pathname, int flags, ... /* mode_t mode */ );`. The man page says this about the `flag` argument:
> The argument flags must include one of the following access modes: O_RDONLY, O_WRONLY, or O_RDWR.  These request opening the file read-only, write-only, or read/write, respectively.

And:

> In addition, zero or more file creation flags and file status flags can be bitwise ORed in flags.  The file creation flags are O_CLOEXEC, O_CREAT, O_DIRECTORY, O_EXCL, O_NOCTTY, O_NOFOLLOW, O_TMPFILE, and O_TRUNC.  The file status flags are all of the remaining flags listed below.  The distinction between these two groups of flags is that the file creation flags affect the semantics of the open operation itself, while the file status flags affect the semantics of subsequent I/O operations.

So in the flags argument we must specify either `O_RDONLY`, `O_WRONLY` or `O_RDWR`. These are **access modes** for the file. They indicate, respectively, whether the file should be opened for reading only, or writing only or both.

And then we have the two categories:
- **File creation flags**: that affect the "semantics", which means the behavior of the `open` operation itself, so it's mainly whether and how the file is going to be opened. The 'O_TMPFILE' is the one we mentioned earlier, it's used to create temporary files but is not supported by all filesystems, I recommend you read the section from the man page on it. Here, we're going to focus on 'O_CREAT' and 'O_EXCL'.
- **File status flags**: that affect the behavior of the file descriptor during subsequent I/O (input/output) operations like `read` and `write`. We're not going to use them for the moment, but you might be interested in `O_APPEND`, `O_NONBLOCK`, `O_SYNC`, `O_DSYNC` and `O_ASYNC`. I could use `O_APPEND` in my code because I'll only append rows to my file, this will also allow me to show you that we have to think about the offset when writing to the file, which I wouldn't have to think about with `O_APPEND`. If you're doing more complex stuff like row updates, then whether you won't open the file with the `O_APPEND` flag and you'll have to think about the offset for each write operation.

One other element to know about these flags is that they're defined as bit masks and ca, be combined with the bitwise OR operator `|`. Example, I'll be using the combination of flags: `O_RDWR | O_CREAT | O_EXCL`, when I go to their definition (in `fcntl-linux.h`) I see:
```c
#define O_RDWR          02
#ifndef O_CREAT
#define O_CREAT         0100  /* Not fcntl.  */
#endif
#ifndef O_CREAT
#define O_EXCL          0200  /* Not fcntl.  */
#endif
```

So these are defined in the octal base. Let's convert them to binary, they give respectively, 0b10, 0b1000000 and 0b10000000 for O_RDWR, O_CREAT and O_EXCL. If we represent them in 8 bits: 0b00000010, 0b01000000 and 0b10000000.

The bitwise OR operator does this: for each bit position, the resulting bit is 1 if either operand has a 1 in that position, otherwise 0.

So `O_RDWR | O_CREAT | O_EXCL` gives, in 8 bits, '0b11000010', which in octal is '0302' and '194' in decimal.

## O_CREAT and O_EXCL

I just want to focus a little bit here on `O_CREAT` and `O_EXCL`. Nothing special but talk a little bit on atomic operations. So from the `open` man page, we get that `O_CREAT` is used to create the file if it does not exist. But what happens if the file exists? We'd like `open` to return an error or something right? One way of doing, is to open the file in `O_RDONLY` and if the returned file descriptor is $-1$ it means the file doesn't exist and you can use `O_CREAT`. But, you can also use the file creation flag `O_EXCL` with it to ensure that the call fails if the file already exists. There are some portability issues with this flag on old NFS versions but I'll let you check the documentation for that.

From the [POSIX standard](https://pubs.opengroup.org/onlinepubs/9799919799/) we [learn another thing](https://pubs.opengroup.org/onlinepubs/9799919799/) about using both `O_CREAT` and `O_EXCL` together:
> If O_CREAT and O_EXCL are set, open() shall fail if the file exists. The check for the existence of the file and the creation of the file if it does not exist shall be atomic with respect to other threads executing open() naming the same filename in the same directory with O_EXCL and O_CREAT set.

"atomic" is the keyword here. It means that the check for the file's existence and the creation of the file happen in a single step, which protects from race conditions where the file could be created by another process between the check and the creation. We don't have that kind of protection with the other way of doing.

## errno

Earlier we mentioned briefly `errno`. Quoting our favorite manual:
> errno - number of last error

And 

> errno is defined by the ISO C standard to be a modifiable lvalue of type int, and must not be explicitly declared; errno may be a macro.  errno is thread-local; setting it in one thread does not affect its value in any other thread.

You can use `errno` with `strerr` for meaningful messages. Remember to only check for `errno` if a system call or a function that sets it fail, otherwise it's not reset or something! 

## Function Pattern

I don't know if this pattern has some specific name but it's something that I encountered a lot.

First, the main part logic is that each function will return a status code indicating success or a specific error. We define these status codes in an `enum`, where $0$ indicates success and negative values represent different errors. All the logs are going to be handled in `main` and not within the functions.

With this design, we have to pass output parameters as pointers so the function can modify them.

Then, we're going to write our functions in a way it verifies its input arguments as needed. It's not only to the caller to do the verification. This avoids issues when many people are contributing to the same project and reusing each other's code.

There are also a bunch of decisions around naming objects (consistent naming for the `enum`s for status codes, `_t` in structures, `_out` and `_in` in arguments to avoid confusion, spacing between includes, functions etc.), but that is so project and team dependent that it's not important to detail it here.

E.g., in `create_file`, we verify the `filepath` isn’t `NULL` (both for defensive programming and ease of use by others), then we call open. If open fails, we check errno to identify whether the file already exists, and we return the corresponding status code. If it succeeds, we store the file descriptor in fd_out. We use this pattern throughout our code: “read” arguments are passed as normal parameters, while “write” arguments that need modification are passed as pointers.

One last point, we can use `const` for arguments that aren't going to be modified by the function, e.g., `const char* filepath`.

## Some Things to be Aware of

When `open_file` or `create_file` return an error, it means that the system call failed. There is no need to `close(fd)` in that case. The file descriptor is invalid anyways. In our code, we return $-1$ in `main` in that case. But if for some reason you have to continue, and you think you can might accidentally use that file descriptor down the line, be sure to handle that case properly. That's why I initialize `fd` to $-1$.

So even if we don't do it in this chapter, cleaning up the resources when encountering an error is going to be a principle we abide by strictly!

I also added the `close(fd)` *as soon as* I declared `fd` because I might forget it later on.

Finally, remember that even `close` can fail. So you should handle that case.