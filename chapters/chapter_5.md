# Chapter 5

## Table of content
  - [Header](#header)
  - [Some Notes about the Code](#some-notes-about-the-code)
  - [Additional robustness when dealing with files](#additional-robustness-when-dealing-with-files)
    - [Better header](#better-header)
    - [Partial reads and writes](#better-header)
    - [Endianness](#endianness)
    - [`fsync`](#fsync)
    - [Multithreading](#multithreading)

## Header

The goal here is to write the structure and the functions to hold the header of the file. The structure is going to be:
```c
typedef struct {
    uint8_t magic[3];
    uint8_t version;
    size_t num_rows;
    size_t num_cols;
    column_t *columns;
} header_t;
```

- `uint8_t magic[3]`: this identifies the file format, it's like its signature. Here, it's set to `'r'`, `'f'`, `'k'` which is in hex: `0x72, 0x66, 0x6b`. This will ensure that we recognize the file is something that our program can process.
- `uint8_t version`: this indicates the version of this file format. This way if our software changes, for example the the way how the data is represented, written in the file and read from it, we ensure that our software version can handle that file's version, either the versions match or we're backwards compatible.
- `size_t num_rows`: how many rows are stored in the file.
- `size_t num_cols`: how many columns are in the file. This will help easily know when to stop reading the columns from the file.
- `column_t *columns`: a pointer to the previous column structure. This will store the columns to read from the file or to write to the file.

## Some Notes about the Code

One generic advice I could give that I forgot to write before is that before you use a system call or a `glibc` function, check the manual to check the return values, the errors, when it fails etc. Sometimes the man pages also provide examples of usage.

For `num_rows` and `num_cols` you probably want to fix the maximum range for deterministic behavior instead of `size_t` that might be $4$ bytes on some platforms.

Notice how in the code the only function that uses `free` is the `read_columns`. You're not responsible for handling memory that comes into your function, but if you allocate memory in your function, make sure to handle it correctly.

In the code I check if `num_cols == 0` because it makes sense in my code, but remember to check for the specific behavior of `malloc(0)` or `calloc(0, sizeof(something))` on your platform.

When I first revisited the code, I wasn't aware that `htonl` and `ntohl` deal with `uint32_t`. On my platform `size_t` is $8$ bytes long, while `uint32_t` is always $4$ bytes long (well as long as your machine's word size is at least $32$ bits but if even if it's not the case, the representation of the number will be in $4$ bytes). So if our `num_cols` and `num_rows` exceed the maximum value of `uint32_t` which is $2^{32} - 1$, we're at risk of data loss. You can read about how to do the operations on $64$ bits in [this Stackoverflow post](https://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c).

This is not really C related but notice how in `update_header_num_rows` we update the header structure after writing in the file. If for some reason you want to do the opposite, remember to go back to the original value in case the write operation fails.

I also want to note that in my `main` function I got lazy and didn't want to write meaningful error messages but you should do that if you're building a real project that users will use. You should also have better code practices, as you might have observed, my code requires some refactoring.

You'll also notice in `main` that we have a block of code that reads the header from the file after writing it and prints it, and that block of code is wrapped in:
```c
#ifdef VERIFY_HEADER
// ...
#endif /* VERIFY_HEADER */
```
That's what we call a compile time conditional so the code only executes if the `VERIFY_HEADER` macro is defined. And we have also modified the `Makefile` to include a new rule, `all-verify` that will clean the build artifacts and compile the program with `-DVERIFY_HEADER`:
```make
noverify: clean
	@echo "Compiling with VERIFY_HEADER"
	$(MAKE) CFLAGS="$(CFLAGS) -DVERIFY_HEADER" all
```

Also notice how we have to get to the beginning to the file to read a correct header because after writing it to the file, the offset was moved. You should be wary of such small things.

ALso you have to pay attention when you're using structures that contain pointers to dynamically allocated memory. E.g., when to do `free_columns(columns, num_allocated_columns)` and `free_columns(header.columns, header.num_cols)`.

## Additional robustness when dealing with files

This section will just mention some ideas when it comes to adding additional robustness when dealing with reads/writes to files. I won't cover them in detail because I didn't have to implement them properly, but I plan on going back to these sections.

### Better header

I just want to say here that you probably want to come with a better header for your own use case. You might want to include checksums ([example of discussion](https://stackoverflow.com/questions/16122067/md5-vs-crc32-which-ones-better-for-common-use) on hash algorithms for this purpose) to verify file integrity or some other metadata that might help you recover from crashes or corruption.

### Partial reads and writes

A single call to `read` often returns all the bytes requested when reading from a local file. And that's why we have those strict checks in the code on the number of bytes read. But if in most network operations or non-blocking I/O scenarios, partial reads are common. Ideally we'd loop until we've read the full amount of data or an error occurs. And we'd adapt our checks accordingly.

A similar logic applies to writing to files.

### Endianness

Endianness is how a multi byte representation is stored and retrieved in memory. Single-byte data (e.g., ASCII characters) is unaffected by endianness because each character occupies only one byte, so there's only one way to store that data. Big endian means we start with the most significant byte while little endian is the opposite.

Example: the 32-bit number `0x12345678` on a little endian machine. The physical memory contains the bytes `[78, 56, 34, 12]` in that order (least significant byte first). When the CPU reads this 32-bit value, it reassembles the bytes into `0x12345678`. *But*, if you inspect the memory manually one byte at a time or two bytes at a time, you might see `0x7856` or `0x3412`. So be wary when you're debugging your program.
This discrepancy occurs because the hardware applies endianness at the word boundary during normal reads and writes, which doesn't happen with manual memory inspection obviously.

In our program we handle endianness ourselves in case because we want to ensure that files written on one machine can be read correctly on another machine that may have a different native byte order. If you know that your code will only run on a very specific architecture with no portability constraints, then you don't have to care about this.

### `fsync`

From [`fsync`'s man page](https://man7.org/linux/man-pages/man2/fsync.2.html): 
> fsync() transfers ("flushes") all modified in-core data of (i.e., modified buffer cache pages for) the file referred to by the file descriptor fd to the disk device (or other permanent storage device) so that all changed information can be retrieved even if the system crashes or is rebooted.  This includes writing through or flushing a disk cache if present. The call blocks until the device reports that the transfer has completed.
<br><br>
As well as flushing the file data, fsync() also flushes the metadata information associated with the file (see inode(7)).

So `fsync` is a system call that, when it returns successfully, it ensures that the file's data and metadata are saved safely on disk or any other reliable storage that even if the system fails suddenly for one reason or another, the changes made before `fsync` are preserved.

In our code we don't bother with it and we just rely on the OS to eventually flush the writes to disk. but I think real databases and any system that's prone to failure uses it extensively to preserve data integrity or use some kind of sophisticated variant of it, like a non-blocking one or something, because with `fsync` there is some kind of performance penalty that comes with it since you force a hard flush to disk which is a relatively expensive operation.  

The man page also mentions `fdatasync` as a variant that only guarantees data and minimal metadata (things needed to correctly read the file’s data afterward). For instance, it might skip updating a file's `st_atime` or `st_mtime` on disk if those aren’t needed to read the data correctly.

You could even write your own version of that and combined with a better header you can ensure better data integrity, especially if you do a lot of concurrency on your data.

### Multithreading

I'll just put some generic advice that holds true in general when it comes to multithreading in case you want to do that in your project.
- **Data Races**: when multiple threads read and write shared data simultaneously without proper synchronization (mutexes, semaphores, etc.), you can get corrupt data or inconsistent states.
- **Deadlocks**: if your synchronization strategy is not carefully planned (e.g., acquiring locks in different orders in different threads), threads can block each other in a cycle forever.
- **Signal handling**: signals can arrive in any thread, so avoid doing complicated logic in signal handlers that might conflict with the rest of the multithreaded program.
- **`errno` usage**: remember that `errno` is thread-local, so keep that in mind if you rely on it.
- **Performance trade-offs**: adding more threads doesn’t always improve performance—there can be overhead in context switching, synchronization, and so forth. You could look into asynchronous non-blocking operations, especially when it comes to I/O, that way you avoid sleeping threads that cost to create them but aren't useful.
