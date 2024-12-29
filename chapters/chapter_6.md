# Chapter 6

## Table of content
  - [Appending Rows](#appending-rows)
  - [Struct Packing](#struct-packing)
    - [Structures and Unions](#structures-and-unions)
      - [Struct Generalities and Flexible Array](#struct-generalities-and-flexible-array)
      - [Struct Layout and Padding](#struct-layout-and-padding)
      - [Struct Packing](#struct-padding)
    - [WIP: Union](#wip-union)
    - [WIP: Bitfield](#wip-bitfield)
  - [WIP: Pointers to pointers](#wip-pointers-to-pointers)

## Appending Rows

The goal of this part is to create a row data structure and to append it to a file. We model a `row_t` as a structure that contains a number of cells (the intersection of a column and a row so the number of cells should be equal to the number of columns in a file) and an array of cells, specifically a pointer to a `cell_t` structure.

```c
typedef struct {
    size_t num_cells;
    cell_t *cells;
} row_t;
```

A cell should contain what type of data it contains and its value. We only have three data types modeled by $0$ for 'int', $1$ for 'float' and $2$ for 'string' so we can model the cell's type as an `enum`. Now when it comes to the value of a cell, or the actual data it holds, there is a particularity of strings, we should also store the length of the string that way we have a way to read it correctly. To do that we'll create a `union`. We'll talk in the following sections about unions. So our `cell_t` structure would look like:

```c
typedef enum {
    CELL_TYPE_INT = 0,
    CELL_TYPE_FLOAT = 1,
    CELL_TYPE_STRING = 2
} cell_type_t;

typedef struct {
    size_t length;
    char *string;
} string_cell_t;

typedef union {
    int32_t int_value;
    float float_value;
    string_cell_t string_cell;
} cell_value_t;

typedef struct {
    cell_type_t type;
    cell_value_t data;
} cell_t;
```

If you read up to this chapter you know that this project is purely educational to learn some C fundamentals and ideas and it's not about learning database management so we don't use sophisticated structures like parse trees and related algorithms. So with our manual parsing of the `-a` option's argument we'll encounter the same issues as with parsing the schema. And we'll use the same techniques. So for example we'll set up a maximum number of cells of $438$ to limit the number of iterations in our while loop. Same techniques as before.

And in a similar fashion we pay attention to free our resources when our function that allocated them fail, we make sure not to have memory leaks when reallocating by overwriting the pointer prematurely, before checking that the returned pointer is valid etc. So we handle the code with the same care.

The only real new thing here are the functions `float_to_network_bytes` and `network_bytes_to_float` that do some gymnastics to serialize/deserialize a float because the `htonl` and `ntohl` work with `uint32_t` and not float. So what we do is, when writing to the file, we copy the bytes as they are from a `float`, which is 32 bits, to a 32 bits memory space stored in a `uint32_t` and we apply `htonl` on it. And when reading from a file, we read into a 32 bits memory space, we apply `ntohl` which we then copy into a float. The 32 bit representation copied into a float will allow the processor to get the correct IEEE 754 reprensetation.

By the way if you have a hard type remember `htonl` and `ntohl`, the 'hton' means host to network byte order, and ntohl means from network to host byte order. The 'l' means long for 32 bits integers and there is `htons` and `ntohs` where the 's' means short. You can figure that out from the [man page](https://man7.org/linux/man-pages/man3/htonl.3p.html).

## Structures and Unions

### Struct

#### Struct Generalities and Flexible Array

A structure is a user defined type that groups heterogenous data (as opposed to arrays that group data of the same type) into a sequence of elements called *members*. But not all data types are allowed in structures:
> A structure or union shall not contain a member with incomplete or function type (hence, a structure
shall not contain an instance of itself, but may contain a pointer to an instance of itself)

And:
> A member of a structure or union may have any complete object type other than a variably modified
type.$^{145)}$ In addition, a member may be declared to consist of a specified number of bits (including a
sign bit, if any). Such a member is called a bit-field $^{146)}$; its width is preceded by a colon.

A complete object type means the type’s size and layout are fully known at compile time. So everything from `int` to `float`, another defined structure, a pointer, a pointer to the structure itself etc. And what you cannot have is an incomplete type as a direct member, including the structure itself (the total size of the structure will be unknown at compile time due to infinite recursion), or a function type. There is an exception though, the *flexible array* member:
> except that the last member of a structure with more than one named member may have incomplete array type; such a structure (and any union containing, possibly recursively, a member that is such a structure)
shall not be a member of a structure or an element of an array.

What does that mean? So to have a flexible member array in a structure you need at least one named member (so a member with a name), and the flexible member array must be the last member of your structure. But if you do that, you can't use that structure as a member of another structure or as an element of an array. Why? Well, it's what the standard said, you can't have incomplete object types. A flexible array's size is unknown at compile at time. Try the following code:
```c
typedef struct {
    size_t count;
    int prices[];
} toys;

int main() {
    size_t num_toys = 5;

    printf("Size of `toys`: %zu\n", sizeof(toys));
    printf("Size of `size_t`: %zu\n", sizeof(size_t));

    toys *my_toys = malloc(sizeof(toys) + num_toys * sizeof(int));
    
    if (!my_toys) {
        printf("`malloc` error.\n");
        return 1;
    }

    my_toys->count = num_toys;

    for (size_t i = 0; i < my_toys->count; i++) {
        my_toys->prices[i] = (int)i + 10;
    }

    printf("my_toys has %zu elements:\n", my_toys->count);
    for (size_t i = 0; i < my_toys->count; ++i) {
        printf("Price of toy %zu: %d\n", i + 1, my_toys->prices[i]);
    }

    free(my_toys);
    return 0;
}
```
What you'll notice is that `sizeof(toys)` excludes the flexible array `int prices[]`. And you also notice that we take allocate the memory for it manually.
> As a special case, the last member of a structure with more than one named member may have an
incomplete array type; this is called a flexible array member. In most situations, the flexible array
member is ignored. In particular, the size of the structure is as if the flexible array member were
omitted except that it may have more trailing padding than the omission would imply.

(We'll talk about padding in the section below)

There's also one specificity of structures with flexible array members, is that assignment doesn't copy the data from flexible array members. You can try the following code and check for yourself:
```c
typedef struct {
    size_t count;
    int prices[];
} toys;

int main() {
    size_t num_toys = 5;

    toys *my_toys = malloc(sizeof(toys) + num_toys * sizeof(int));
    if (!my_toys) {
        printf("`malloc` error.\n");
        return 1;
    }
    my_toys->count = num_toys;
    for (size_t i = 0; i < my_toys->count; i++) {
        my_toys->prices[i] = (int)i + 10;
    }
    printf("my_toys has %zu elements:\n", my_toys->count);
    for (size_t i = 0; i < my_toys->count; ++i) {
        printf("Price of toy %zu: %d\n", i + 1, my_toys->prices[i]);
    }

    printf("\n");

    toys *new_toys = malloc(sizeof(toys) + num_toys * sizeof(int));
    if (!new_toys) {
        free(my_toys);
        printf("`malloc` error.\n");
        return 1;
    }
    new_toys->count = 10;
    for (size_t i = 0; i < num_toys; i++) {
        new_toys->prices[i] = (int)i + 50;
    }

    printf("Before assignment:\n");
    printf("new_toys has %zu elements:\n", new_toys->count);
    for (size_t i = 0; i < num_toys; ++i) {
        printf("Price of toy %zu: %d\n", i + 1, new_toys->prices[i]);
    }

    printf("\n");

    *new_toys = *my_toys;
    printf("After assignment:\n");
    printf("new_toys has %zu elements:\n", new_toys->count);
    for (size_t i = 0; i < num_toys; ++i) {
        printf("Price of toy %zu: %d\n", i + 1, new_toys->prices[i]);
    }

    free(my_toys);
    free(new_toys);
    return 0;
}
```

The standard also says when copying data through assignment:
> if any of the array elements are within the first sizeof(struct s) bytes of the structure, they are set to an indeterminate representation, that may or may not coincide with a
copy of the representation of the elements of the source array

That conludes our coverage of flexible arrays in structures for the moment, we'll come back to them in padding.

We mentioned previously the named member. You'll always require at least least one named member, whether with a flexible array member or not:
> If the member declaration list does not contain any named members, either directly or via an
anonymous structure or anonymous union, the behavior is undefined.

So you must declare at least one member that has an identifier in your structures otherwise it cannot meaningfully store user accessible data in any fashion imaginable. And that's also why you require at least one named member when using a flexible array otherwise you can't guarantee that flexible arrays are truly flexible and dynamically sizable OR you'd get something akin to a zero-member structure.

You can access a member of structure with the `.` notation (`my_struct.member`) or if you have a pointer to a structure you can use the `->` notation (`my_ptr_to_struct->member`).

You can also declare bit-fields (more about them below) in structures with the following constraint:
> The expression that specifies the width of a bit-field shall be an integer constant expression with a
nonnegative value that does not exceed the width of an object of the type that would be specified
were the colon and expression omitted $^{143)}$. If the value is zero, the declaration shall have no
declarator.

So bit-fields must have widths that fit within the type they claim to be using. You can't specify a width that's larger than the underlying type.

You can also have what's called *anonymous structures*.
> An unnamed member whose type specifier is a structure specifier with no tag is called an anonymous
structure ... The members of an anonymous structure or union are members of the containing
structure or union, keeping their structure or union layout. This applies recursively if the containing
structure or union is also anonymous.

So if you have:
```c
struct {
    struct {
        int a;
        int b;
    };
    int c;
} anon_within;
```
Then `a` and `b` are considered members of `anon_within` and you can do `anon_within.a` and `anon_within.b`.

One last thing about structures, a pointer to a structure is a pointer to its first element.
> A pointer to a structure object, suitably converted, points to its initial member (or if that member is a bit-field, then to the unit in which it resides), and vice versa

#### Struct Layout and Padding

So we said that a structure groups heterogeneous data into a sequence of members laid out in memory in declaration order. The C standard specifies that:
- Each structure member has an address that increases in the order in which it is declared.
    > a structure is a type consisting of a sequence of members, whose storage is
    allocated in an ordered sequence
- There may be unnamed padding inserted between members, and also possibly at the end of the structure, but never before the first member.
    > There may be unnamed padding within a structure object, but not
    at its beginning.
- The structure’s address is the same as the address of its first member, it's the vice versa in the excerpt below, unless that first member is a bit-field, in which case it points to the underlying storage unit for the bit-field.
    > Within a structure object, the non-bit-field members and the units in which bit-fields reside have
    addresses that increase in the order in which they are declared.
    <br>
    ...
    <br>
    A pointer to a structure object, suitably converted, points to its initial member (or if that member is a bit-field, then to the unit in which it resides), and vice versa.

What do we mean by padding? Well, objects in memory can start in theory at any address space but in practice they don't for efficiency reasons. If you take an `int` that's $4$ bytes long, it'll take $4$ bytes of memory right, the first byte will start on a memory address that's divisible by $4$, for example it'll span over the memory addresses $16, 17, 18, 19$.  
If a data type takes $N$ bytes, it's better if it starts on memory addresses that are divisible by $N$.  
Now, that's what we call *alignment*. *Padding* is when we pad with dummy or useless bytes to attain that alignment for some of our data. It doesn't happen only within structures or unions, even within arrays and global variables. But let's focus on structures, it's also the most visible example. Imagine the following structure:
```c
typedef struct {
    int x;
    char dummy;
    int *p;
} padded_struct;
```
`x` takes $4$ bytes right. The address of a structure is the address of its first element right. So it'll start on an address that's divisible by $4$, let's say $12$. Then $12, 13, 14, 15$ contain the byte values of `x`. Since `dummy` is a `char` that only requires $1$ byte, it can start everywhere in memory, but since structures are sequences of their members, it'll take place on memory address $16$. But what do we have after that? An `int *` which is on my platform $8$ bytes long, it can't start on memory address $17$ because that's not divisible by $8$. So what will happen is that we'll pad. We'll add $7$ bytes, $17, 18, 19, 20, 21, 22, 23$, and then we'll unroll our pointer starting from $24$. That's an example of padding. And as I said, it doesn't happen only within structures.

Now, I've lied to you, a structure's alignment (which effectively means on which memory addresses it can start) is not of its first member, but of larger one. In our case it's the pointer. So the memory alignment of our structure is $8$ and it can't start on $12$ but it might start on $16$. And even then we'll still require padding, just lesser padding, we'll only require $3$ bytes. That's why the alignment of the structure is the same as the largest individual alignment of its members. That padding within a structure is what's called unnamed padding.

Padding can never happen at the beginning of a structure because the structure will always be self aligned, but you can have padding at the end of a structure, it's called *trailing padding* (nothing to have with unnamed vs named, we call it unnamed padding because that's not something we can access within the structure, the term trailing just refers to that it happens at the end, it's also unnamed). Let's take the above example and add another `char` member:
```c
typedef struct {
    int x;
    char dummy1;
    int *p;
    char dummy2;
} padded_struct;
```

And let's see the size of the structure and the offset of each element, let's also create an array with two of these structures and see the address of each one. Remember arrays are contiguous blocks of memory.
```c
#include <stdio.h>
#include <stddef.h>

typedef struct {
    int x;       // 4 bytes
    char dummy1; // 1 byte
    int *p;      // 8 bytes on many 64-bit systems
    char dummy2; // 1 byte
} padded_struct;

int main(void)
{
    printf("sizeof(padded_struct) = %zu bytes\n", sizeof(padded_struct));

    printf("offsetof(padded_struct, x)       = %zu\n", offsetof(padded_struct, x));
    printf("offsetof(padded_struct, dummy1)  = %zu\n", offsetof(padded_struct, dummy1));
    printf("offsetof(padded_struct, p)       = %zu\n", offsetof(padded_struct, p));
    printf("offsetof(padded_struct, dummy2)  = %zu\n", offsetof(padded_struct, dummy2));

    padded_struct arr[2];

    printf("\nAddress of arr[0] = %p\n", (void*)&arr[0]);
    printf("Address of arr[1] = %p\n", (void*)&arr[1]);
    printf("Difference (arr[1] - arr[0]) = %ld bytes\n", (char*)&arr[1] - (char*)&arr[0]);

    return 0;
}
```

On my platform it shows:
```
sizeof(padded_struct) = 24 bytes
offsetof(padded_struct, x)       = 0
offsetof(padded_struct, dummy1)  = 4
offsetof(padded_struct, p)       = 8
offsetof(padded_struct, dummy2)  = 16

Address of arr[0] = 0x16b827258
Address of arr[1] = 0x16b827270
Difference (arr[1] - arr[0]) = 24 bytes
```

So `padded_struct`'s size is $24$. Let's do the computation again: $4$ bytes of `x`, $1$ byte of `dummy1`, $3$ bytes for the padding between `dummy1` and `p`, $8$ bytes of `p`, then `dummy2` can start right afterwards so $1$ byte. This amounts to $4 + 1 + 3 + 8 + 1 = 17$ bytes. But the size of the structure is $24$ bytes. That's because we padd at the end of the structure as well so that space fits within alignment. And we also notice that within the array, the difference in the addresses of the two structures is $24$.

We'll talk about the bit-fields and padding in the section reserved for bit-fields but some interesting excerpts from the standard:
> There may be unnamed padding within a structure object, but not at its beginning.
<br>
...
<br>
There may be unnamed padding at the end of a structure or union.
<br>
...
<br>
In particular, the size of the structure is as if the flexible array member were omitted except that it may have more trailing padding than the omission would imply.

Now let's go back to the flexible array members and check what happens with their offsets and padding.
> As a special case, the last member of a structure with more than one named member may have an incomplete array type; this is called a flexible array member. In most situations, the flexible array member is ignored. In particular, the size of the structure is as if the flexible array member were omitted except that it may have more trailing padding than the omission would imply. However, when a. (or ->) operator has a left operand that is (a pointer to) a structure with a flexible array member and the right operand names that member, it behaves as if that member were replaced with the longest array (with the same element type) that would not make the structure larger than the object being accessed; the offset of the array shall remain that of the flexible array member, even if this would differ from that of the replacement array. If this array would have no elements, it behaves as if it had one element but the behavior is undefined if any attempt is made to access that element or to generate a pointer one past it.

Let's take out our `toys` structure example again:
```c
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
    size_t count; 
    char dummy; 
    int prices[]; 
} toys;

int main() {
    printf("Structure layout:\n");
    printf("  sizeof(size_t)          = %zu\n", sizeof(size_t));
    printf("  sizeof(char)            = %zu\n", sizeof(char));
    printf("  sizeof(int)             = %zu\n", sizeof(int));
    printf("  sizeof(toys)            = %zu\n", sizeof(toys));
    printf("  offsetof(toys, count)   = %zu\n", offsetof(toys, count));
    printf("  offsetof(toys, dummy)   = %zu\n", offsetof(toys, dummy));
    printf("  offsetof(toys, prices)  = %zu\n\n", offsetof(toys, prices));


    size_t num_toys = 5;
    toys *my_toys = malloc(sizeof(toys) + num_toys * sizeof(int));
    if (!my_toys) {
        printf("`malloc` error.\n");
        return 1;
    }
    my_toys->count = num_toys;
    for (size_t i = 0; i < my_toys->count; i++) {
        my_toys->prices[i] = (int)i + 10;
    }
    printf("my_toys has %zu elements:\n", my_toys->count);
    for (size_t i = 0; i < my_toys->count; ++i) {
        printf("Price of toy %zu: %d\n", i + 1, my_toys->prices[i]);
    }

    free(my_toys);
    return 0;
}
```

So, what we already know is that the compiler computes `sizeof(toys)` as if `toys.prices[]` doesn't exist However, it can insert extra padding after `dummy` to align the start of `prices[]` to the alignment needed for `int`. Because `size_t` might have a stricter alignment requirement, the compiler might align the whole struct to that boundary, possibly having trailing padding as well.

In my case I get:
```
Structure layout:
  sizeof(size_t)          = 8
  sizeof(char)            = 1
  sizeof(int)             = 4
  sizeof(toys)            = 16
```

So $3$ bytes of unnamed padding to align `princes[]` on a multiple of $4$ address, and no trailing padding. So even though the array is incomplete at compile time, the compiler can add trailing padding or alignment to ensure that if you we store elements, they start at an aligned address for `int`.

In our case we allocate `sizeof(toys) + num_toys * sizeof(int)`, so our `my_toys->prices` is n array of length `num_toys` within that allocation, at runtime. But if we had allocated no extra space for the array, we'd still offset `prices[]` but with a length of $0$ . The standard says it "behaves as if it had one element, but accessing it is undefined".

#### Struct Packing

So, from what we've learned about a structure's layout and padding, if we want to minimize a structure’s size and get rid of the wasteful padding bytes, we can reorder the members as an attempt to reduce or eliminate padding. Typically, placing members in decreasing order of alignment requirements helps. Larger alignment types (pointers, long on 64 bit systems, etc.) can be placed first, followed by $4$ byte aligned types, then $2$ byte ones, then $1$ byte types. Though this reordering can reduce a structure’s overall size and its memory footprint, it can also hinder its readability. So use it wisely.

It is also common to pack multiple boolean flags (or other small fields) into bit-fields which allow to reduce padding because instead of taking 1 byte per field you'll end up taking 1 byte for many fields.

We'll get into bit-fields later on but keeping in mind that they can have performance implications and accessing them is less straightforward to read and debug. So it's again a question of tradeoffs. maintainability.

The following blog post explains in great details all the [art of struct packing](http://www.catb.org/esr/structure-packing/), I discovered it around 2019 and it was like a revelation at the time.

### WIP: Union

> a union is a type consisting of a sequence of members whose
storage overlap

### WIP: Bitfield

## WIP: Pointers to pointers

Though we don't use pointers to pointers in this part, we did that in the last part and I forgot to talk about. I think some people get confused about pointers to pointers and generally its due to a confusion about pointers themselves.

