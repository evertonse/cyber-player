<html><head><title>stb_ds.h</title></head>
<body>
<!--!HTML
!TITLE stb_ds.h

## stb_ds.h

<a href="stb_ds.h">stb_ds.h</a> is a public-domain single-header-file C library
for type-safe manipulation
of two basic container types: dynamic arrays and hash tables. (The code also
works in C++, although if you do not need C/C++ interoperation you'd probably
want to use something template-based like `vector\<\>`.)

## Arrays

Dynamic arrays are accessed using a pointer to the type of data you want in the array.

Here's some sample code for creating and manipulating a dynamic array:

  int main(void)
  {
    int *array = NULL;
    arrput(array, 2);
    arrput(array, 3);
    arrput(array, 5);
    for (int i=0; i < arrlen(array); ++i)
      printf("%d ", array[i]);
  }

`arrput` pushes the value onto the end of the dynamic array (like std::vector.push_back),
so the above code will print `2 3 5`.

Note that *these macros write to the `array` variable if the array has to be resized. This
means that if you pass a dynamic array into a function which increases its length, you
need to return the updated array from the function to its caller.* In other words, the
semantics are the same as any `realloc()`-ed pointer, and are unlike the semantics of
things like `std::vector<>`, where the core object is stable even if the array elements
aren't.

The following functions are defined:

* `arrlen` - the length of the dynamic array
* `arrlenu` - the length of the dynamic array as an unsigned type
* `arrput` - copy an object into the dynamic array
* `arrfree` - free the entire array
* `arraddn` - adds n uninitialized values to the dynamic array
* `arrsetlen` - sets the length of the array (leaves new slots uninitialized)
* `arrlast` - the last item in the array as an lvalue
* `arrins` - inserts an item in the middle of the array
* `arrdel` - deletes an item from the middle of the array, moving the following items over
* `arrswapdel` - deletes an item from the middle of the array, replacing it with the formerly last item
* `arrcap` - returns the internal capacity, the maximum length the array can be without reallocating it
* `arrsetcap` - sets the internal capacity. it is not possible to shrink the capacity (currently)

The name 'arrpush' is an alias for 'arrput' for compatibility with older stb libraries.

## Hash map

Hash maps (i.e. hash tables) are created by defining a structure with a `key` field and a `value` field.
Here's some sample code for creating and manipulating a hash table:

```c
  int main(void)
  {
    float f;
    struct { float key; char value; } *hash = NULL;
    f=10.5; hmput(hash, f, 'h');
    f=20.4; hmput(hash, f, 'e');
    f=50.3; hmput(hash, f, 'l');
    f=40.6; hmput(hash, f, 'X');
    f=30.9; hmput(hash, f, 'o');

    f=40.6; printf("%c - ", hmget(hash, f));

    f=40.6; hmput(hash, f, 'l');
    for (int i=0; i < hmlen(hash); ++i)
      printf("%c ", hash[i].value);
  }
```

`hmput` appends new entries to the end of the array, but updates existing ones, so the
above code will print `X``-``h``e``l``l``o`. Note that if there are deletions, the table
will become reordered (using swap-delete), so you typically should not make assumptions
about the order of items in the table; this is just convenient for the simple demonstration
above.

Note that on Visual Studio, the hash key provided to `hmput` must be an lvalue so the library
can take the address of it. In practice this is not normally a burden, but on GCC and Clang,
this limitation is eliminated, so you can write:

```c
  int main(void)
  {
    float f;
    struct { float key; char value; } *hash = NULL;
    hmput(hash, 10.5, 'h');
    hmput(hash, 20.4, 'e');
    hmput(hash, 50.3, 'l');
    hmput(hash, 40.6, 'X');
    hmput(hash, 30.9, 'o');

    printf("%c - ", hmget(hash, 40.6));

    hmput(hash, 40.6, 'l');
    for (int i=0; i < hmlen(hash); ++i)
      printf("%c ", hash[i].value);
  }

```

You can specify a default value which is returned when you try to get a key that's not in the map.
The default value is all-bits-0.

```c
  int main(void)
  {
    int i,n;
    struct { int key; char value; } *hash = NULL;

    hmdefault(hash, 'l');
    i=0; hmput(hash, i, 'h');
    i=1; hmput(hash, i, 'e');
    i=4; hmput(hash, i, 'o');
    for (int i=0; i <= 4; ++i)
      printf("%c ", hmget(hash,i));
  }
```

The above code prints `h``e``l``l``o`.

### Keys are arbitrary

The key can be any type we want; two keys are considered equal if they are bit-identical, so
this may or may not be what you want for pointers, and is definitely not what you want for
strings. For example:

  typedef struct { int x,y; } pair;
  
  int main(void)
  {
    int i;
    pair p;
    struct { pair key; char value; } *hash = NULL;
    p = (pair){2,3}; hmput(hash, p, 'h');
    p = (pair){7,4}; hmput(hash, p, 'e');
    p = (pair){1,1}; hmput(hash, p, 'l');
    p = (pair){5,5}; hmput(hash, p, 'x');
    p = (pair){3,5}; hmput(hash, p, 'o');

    p = (pair){5,5}; hmput(hash, p, 'l');
    for (int i=0; i < hmlen(hash); ++i)
      printf("%c ", hash[i].value);
  }

The basic hashmap functions are:

* `hmlen` - The length of the hashmap array data for iterating the hashmap contents
* `hmlenu` - As above, but unsigned
* `hmget` - Get the value corresponding to a key, or return the default
* `hmgeti` - Return the index into the array of a given key, or -1
* `hmput` - Update an existing `key,value` pair, or add a new one if the key isn't present
* `hmdel` - Delete a `key,value` pair if the key is present
* `hmdefault` - Set the default value used by `hmget`

### Structures can have more fields

In addition, structures can contain multiple value fields. The only requirement is that there is a key field.
To do this, you must use structure values explicitly.

```c
  typedef struct { int key; float my_val; char *my_string; } my_struct;
  int main(void)
  {
    my_struct *hash = NULL;
    my_struct s;
    s = (my_struct) { 20, 5.0 , "hello " };
    hmputs(hash, s);
    s = (my_struct) { 40, 2.5 , "failure"};
    hmputs(hash, s);
    s = (my_struct) { 40, 1.1 , "world!" };
    hmputs(hash, s);
    s = (my_struct) { 0,0,0 };
    printf("%s", hmgets(hash, 20).my_string);
    printf("%s", hmgets(hash, 40).my_string);
  }
```

Because the structure value is copied into the hash table by hmputs, the above code prints `hello world!`.

The hashmap functions for manipulating structures without a `value` field are:

* `hmlen` - The length of the hashmap array data for iterating the hashmap contents
* `hmgets` - Get the value corresponding to a key, or return the default
* `hmgeti` - Return the index into the array of a given key, or -1
* `hmputs` - Update an existing structure, or add a new one if the key isn't present
* `hmdel` - Delete a structure if the key is present
* `hmdefaults` - Set the default struct used by `hmget`

### String hashmaps

A separate set of functions are provided for when the key is a char* pointer.
In this case the keys do not need to be lvalues in Visual Studio.

```c
  int main(void)
  {
    struct { char *key; char value; } *hash = NULL;
    shput(hash, "bob"   , 'h');
    shput(hash, "sally" , 'e');
    shput(hash, "fred"  , 'l');
    shput(hash, "jen"   , 'x');
    shput(hash, "doug"  , 'o');

    char name[4] = "jen";
    shput(hash, name    , 'l');

    for (int i=0; i < shlen(hash); ++i)
      printf("%c ", hash[i].value);
  }
```

Even though the original `"jen"` and the final `shput()` of `name` have different addresses,
the code above will print `h``e``l``l``o`.

Here is sample code to count duplicate lines in a file.

```c
  int main(int argc, char **argv)
  {
    struct { char *key; int value; } *hash = NULL;
    int i,n;
    char **dict = stb_stringfile(argv[1], &n);
    for (i=0; i < n; ++i) {
      int z = shget(hash, dict[i]); // default value is 0
      shput(hash, dict[i], z+1);
    }
    for (i=0; i < shlen(hash); ++i)
      if (hash[i].value > 1)
        printf("%4d %s\n", hash[i].value, hash[i].key);
  }
```

Note that by default shput stores whatever `char*` pointer was passed in.
Using a string stored on the stack as in the earlier "`jen`" code will not usually
work, since the string will be overwritten later. To address this, string hashmaps
provide two optional facilities for storing permanent strings.

You can have the internal copy of keys each be allocated separately with malloc:

```c
  struct { char *key; char *value } *hash;
  sh_new_strdup(hash);
  char name[4] = "bob";
  shput(hash, name, "hi there");
  name[0] = x;
  printf("%s", shget(hash, "bob").value);
```

Or you can have string keys be stored in an arena private to this hash table:

```c
  struct { char *key; char *value } *hash;
  sh_new_arena(hash);
  char name[4] = "bob";
  shput(hash, name, "hi there");
  name[0] = x;
  printf("%s", shget(hash, "bob").value);
```

Each of the above code fragments will print `hi there`.

Use `sh_new_arena` for hash tables if you never delete keys.
Using `sh_new_strdup` is slower, because the strings must be allocated and freed
on each `shput` and `shdel`, whereas `sh_new_arena` bundles multiple allocations
together to minimize memory allocations. However, if you use `shdel` with
`sh_new_arena`, the memory for the deleted string
will be wasted until you delete the entire table with `shfree`.

The string hashmap also provides structure functions `shgets` and `shputs`. These
functions should not be used when using `sh_new_arena` and `sh_new_strdup`, as the
string found in the structure will always be used. You can do `strdup()` yourself on
keys. A simple string arena API is also provided that you can use as well.

The complete set of string hashmap functions is:

* `shlen` - The length of the hashmap array data for iterating the hashmap contents
* `shlenu` - As above, but unsigned
* `shget` - Get the value corresponding to a key, or return the default
* `shgets` - Get the value corresponding to a key, or return the default
* `shgeti` - Return the index into the array of a given key, or -1
* `shput` - Update an existing `key,value` pair, or add a new one if the key isn't present
* `shputs` - Update an existing structure, or add a new one if the key isn't present
* `shdel` - Delete a `key,value` pair if the key is present
* `shdefault` - Set the default value used by `hmget`
* `shdefaults` - Set the default struct used by `hmget`
* `sh_new_arena` - Overwrites pointer with a new hashmap using a string arena for string management
* `sh_new_strdup` - Overwrites pointer with a new hashmap using strdup for string management

In addition, the string arena has two functions:
* `stbds_stralloc` - Allocates a string in a string arena
* `stbds_strreset` - Frees all the strings in a string arena

##impl Implementation notes

All data structures use the trick of storing extra data "before" the pointed-to-array. In
the case of dynamic arrays this includes the length and capacity of the array. For
hash tables, a pointer is stored to a separately allocated hash table. The array
part of the hash table is actually valid from -1..`hmlen`, with -1 storing the default
value from `hmdefault`.

The code uses `size_t` as an unsigned type and `ptrdiff_t` as a signed type. These
are usually the same size as pointers (i.e. `uintptr_t`, but with compatibility with
older compilers), which are usually the same size as registers. However, there are
some compilation models in which pointers are only 32-bits on 64-bit architectures,
in which case these types gain an advantage for size overhead, but lose the ability
to compute hashes 64-bits-at-a-time.

String hashing is done with a simple but relatively good hash function which circular shifts
and adds characters, with a final mix.

Binary data hashing uses hard-coded fast-and-decent hash functions for 4- and 8-byte
data (8-byte is only on 64-bit platforms). All other data is hashed using a weakened form
of Siphash. If you define STBDS_SIPHASH_2_4 on 64-bit platforms, correct SipHash-2-4
will be used for all keys.

Hash tables are bucketed and cache-aligned for performance. However, because they use
open addressing with power-of-two table sizes, there will be performance spikes when
the table is resized.

The string arena begins by allocating blocks of size 512 bytes to minimize wastage,
but doubles these block sizes up to a size of 1MB to minimize the number of memory
allocations which must be performed.

-->

<h2>stb_ds.h</h2>


<a href="stb_ds.h">stb_ds.h</a> is a public-domain single-header-file C library
for type-safe manipulation
of two basic container types: dynamic arrays and hash tables. (The code also
works in C++, although if you do not need C/C++ interoperation you'd probably
want to use something template-based like <tt>vector&lt;></tt>.)
<p>
<h2>Arrays</h2>


Dynamic arrays are accessed using a pointer to the type of data you want in the array.
<p>
Here's some sample code for creating and manipulating a dynamic array:
<p>
<pre> int main(void)
 {
   int *array = NULL;
   arrput(array, 2);
   arrput(array, 3);
   arrput(array, 5);
   for (int i=0; i &lt; arrlen(array); ++i)
     printf("%d ", array[i]);
 }</pre>
<p>
<tt>arrput</tt> pushes the value onto the end of the dynamic array (like std::vector.push_back),
so the above code will print <tt>2 3 5</tt>.
<p>
Note that <b>these macros write to the <tt>array</tt> variable if the array has to be resized. This
means that if you pass a dynamic array into a function which increases its length, you
need to return the updated array from the function to its caller.</b> In other words, the
semantics are the same as any <tt>realloc()</tt>-ed pointer, and are unlike the semantics of
things like <tt>std::vector&lt;&gt;</tt>, where the core object is stable even if the array elements
aren't.
<p>
The following functions are defined:
<p>
<ul>
<li><tt>arrlen</tt> - the length of the dynamic array
<li><tt>arrlenu</tt> - the length of the dynamic array as an unsigned type
<li><tt>arrput</tt> - copy an object into the dynamic array
<li><tt>arrfree</tt> - free the entire array
<li><tt>arraddn</tt> - adds n uninitialized values to the dynamic array
<li><tt>arrsetlen</tt> - sets the length of the array (leaves new slots uninitialized)
<li><tt>arrlast</tt> - the last item in the array as an lvalue
<li><tt>arrins</tt> - inserts an item in the middle of the array
<li><tt>arrdel</tt> - deletes an item from the middle of the array, moving the following items over
<li><tt>arrswapdel</tt> - deletes an item from the middle of the array, replacing it with the formerly last item
<li><tt>arrcap</tt> - returns the internal capacity, the maximum length the array can be without reallocating it
<li><tt>arrsetcap</tt> - sets the internal capacity. it is not possible to shrink the capacity (currently)
</ul><p>
The name 'arrpush' is an alias for 'arrput' for compatibility with older stb libraries.
<p>
<h2>Hash map</h2>


Hash maps (i.e. hash tables) are created by defining a structure with a <tt>key</tt> field and a <tt>value</tt> field.
Here's some sample code for creating and manipulating a hash table:
<p>
<pre> int main(void)
 {
   float f;
   struct { float key; char value; } *hash = NULL;
   f=10.5; hmput(hash, f, 'h');
   f=20.4; hmput(hash, f, 'e');
   f=50.3; hmput(hash, f, 'l');
   f=40.6; hmput(hash, f, 'X');
   f=30.9; hmput(hash, f, 'o');</pre>
<p>
<pre>   f=40.6; printf("%c - ", hmget(hash, f));</pre>
<p>
<pre>   f=40.6; hmput(hash, f, 'l');
   for (int i=0; i &lt; hmlen(hash); ++i)
     printf("%c ", hash[i].value);
 }</pre>
<p>
<tt>hmput</tt> appends new entries to the end of the array, but updates existing ones, so the
above code will print <tt>X&nbsp;-&nbsp;h&nbsp;e&nbsp;l&nbsp;l&nbsp;o</tt>. Note that if there are deletions, the table
will become reordered (using swap-delete), so you typically should not make assumptions
about the order of items in the table; this is just convenient for the simple demonstration
above.
<p>
Note that on Visual Studio, the hash key provided to <tt>hmput</tt> must be an lvalue so the library
can take the address of it. In practice this is not normally a burden, but on GCC and Clang,
this limitation is eliminated, so you can write:
<p>
<pre> int main(void)
 {
   float f;
   struct { float key; char value; } *hash = NULL;
   hmput(hash, 10.5, 'h');
   hmput(hash, 20.4, 'e');
   hmput(hash, 50.3, 'l');
   hmput(hash, 40.6, 'X');
   hmput(hash, 30.9, 'o');</pre>
<p>
<pre>   printf("%c - ", hmget(hash, 40.6));</pre>
<p>
<pre>   hmput(hash, 40.6, 'l');
   for (int i=0; i &lt; hmlen(hash); ++i)
     printf("%c ", hash[i].value);
 }</pre>
<p>
You can specify a default value which is returned when you try to get a key that's not in the map.
The default value is all-bits-0.
<p>
<pre> int main(void)
 {
   int i,n;
   struct { int key; char value; } *hash = NULL;</pre>
<p>
<pre>   hmdefault(hash, 'l');
   i=0; hmput(hash, i, 'h');
   i=1; hmput(hash, i, 'e');
   i=4; hmput(hash, i, 'o');
   for (int i=0; i &lt;= 4; ++i)
     printf("%c ", hmget(hash,i));
 }</pre>
<p>
The above code prints <tt>h&nbsp;e&nbsp;l&nbsp;l&nbsp;o</tt>.
<p>
<h3>Keys are arbitrary</h3>


The key can be any type we want; two keys are considered equal if they are bit-identical, so
this may or may not be what you want for pointers, and is definitely not what you want for
strings. For example:
<p>
<pre> typedef struct { int x,y; } pair;
 
 int main(void)
 {
   int i;
   pair p;
   struct { pair key; char value; } *hash = NULL;
   p = (pair){2,3}; hmput(hash, p, 'h');
   p = (pair){7,4}; hmput(hash, p, 'e');
   p = (pair){1,1}; hmput(hash, p, 'l');
   p = (pair){5,5}; hmput(hash, p, 'x');
   p = (pair){3,5}; hmput(hash, p, 'o');</pre>
<p>
<pre>   p = (pair){5,5}; hmput(hash, p, 'l');
   for (int i=0; i &lt; hmlen(hash); ++i)
     printf("%c ", hash[i].value);
 }</pre>
<p>
The basic hashmap functions are:
<p>
<ul>
<li><tt>hmlen</tt> - The length of the hashmap array data for iterating the hashmap contents
<li><tt>hmlenu</tt> - As above, but unsigned
<li><tt>hmget</tt> - Get the value corresponding to a key, or return the default
<li><tt>hmgeti</tt> - Return the index into the array of a given key, or -1
<li><tt>hmput</tt> - Update an existing <tt>key,value</tt> pair, or add a new one if the key isn't present
<li><tt>hmdel</tt> - Delete a <tt>key,value</tt> pair if the key is present
<li><tt>hmdefault</tt> - Set the default value used by <tt>hmget</tt>
</ul><p>
<h3>Structures can have more fields</h3>


In addition, structures can contain multiple value fields. The only requirement is that there is a key field.
To do this, you must use structure values explicitly.
<p>
<pre> typedef struct { int key; float my_val; char *my_string; } my_struct;
 int main(void)
 {
   my_struct *hash = NULL;
   my_struct s;
   s = (my_struct) { 20, 5.0 , "hello " };
   hmputs(hash, s);
   s = (my_struct) { 40, 2.5 , "failure"};
   hmputs(hash, s);
   s = (my_struct) { 40, 1.1 , "world!" };
   hmputs(hash, s);
   s = (my_struct) { 0,0,0 };
   printf("%s", hmgets(hash, 20).my_string);
   printf("%s", hmgets(hash, 40).my_string);
 }</pre>
<p>
Because the structure value is copied into the hash table by hmputs, the above code prints <tt>hello world!</tt>.
<p>
The hashmap functions for manipulating structures without a <tt>value</tt> field are:
<p>
<ul>
<li><tt>hmlen</tt> - The length of the hashmap array data for iterating the hashmap contents
<li><tt>hmgets</tt> - Get the value corresponding to a key, or return the default
<li><tt>hmgeti</tt> - Return the index into the array of a given key, or -1
<li><tt>hmputs</tt> - Update an existing structure, or add a new one if the key isn't present
<li><tt>hmdel</tt> - Delete a structure if the key is present
<li><tt>hmdefaults</tt> - Set the default struct used by <tt>hmget</tt>
</ul><p>
<h3>String hashmaps</h3>


A separate set of functions are provided for when the key is a char* pointer.
In this case the keys do not need to be lvalues in Visual Studio.
<p>
<pre> int main(void)
 {
   struct { char *key; char value; } *hash = NULL;
   shput(hash, "bob"   , 'h');
   shput(hash, "sally" , 'e');
   shput(hash, "fred"  , 'l');
   shput(hash, "jen"   , 'x');
   shput(hash, "doug"  , 'o');</pre>
<p>
<pre>   char name[4] = "jen";
   shput(hash, name    , 'l');</pre>
<p>
<pre>   for (int i=0; i &lt; shlen(hash); ++i)
     printf("%c ", hash[i].value);
 }</pre>
<p>
Even though the original `"jen"` and the final <tt>shput()</tt> of <tt>name</tt> have different addresses,
the code above will print <tt>h&nbsp;e&nbsp;l&nbsp;l&nbsp;o</tt>.
<p>
Here is sample code to count duplicate lines in a file.
<p>
<pre> int main(int argc, char **argv)
 {
   struct { char *key; int value; } *hash = NULL;
   int i,n;
   char **dict = stb_stringfile(argv[1], &amp;n);
   for (i=0; i &lt; n; ++i) {
     int z = shget(hash, dict[i]); // default value is 0
     shput(hash, dict[i], z+1);
   }
   for (i=0; i &lt; shlen(hash); ++i)
     if (hash[i].value &gt; 1)
       printf("%4d %s\n", hash[i].value, hash[i].key);
 }</pre>
<p>
Note that by default shput stores whatever <tt>char*</tt> pointer was passed in.
Using a string stored on the stack as in the earlier "<tt>jen</tt>" code will not usually
work, since the string will be overwritten later. To address this, string hashmaps
provide two optional facilities for storing permanent strings.
<p>
You can have the internal copy of keys each be allocated separately with malloc:
<p>
<pre> struct { char *key; char *value } *hash;
 sh_new_strdup(hash);
 char name[4] = "bob";
 shput(hash, name, "hi there");
 name[0] = x;
 printf("%s", shget(hash, "bob").value);</pre>
<p>
Or you can have string keys be stored in an arena private to this hash table:
<p>
<pre> struct { char *key; char *value } *hash;
 sh_new_arena(hash);
 char name[4] = "bob";
 shput(hash, name, "hi there");
 name[0] = x;
 printf("%s", shget(hash, "bob").value);</pre>
<p>
Each of the above code fragments will print <tt>hi there</tt>.
<p>
Use <tt>sh_new_arena</tt> for hash tables if you never delete keys.
Using <tt>sh_new_strdup</tt> is slower, because the strings must be allocated and freed
on each <tt>shput</tt> and <tt>shdel</tt>, whereas <tt>sh_new_arena</tt> bundles multiple allocations
together to minimize memory allocations. However, if you use <tt>shdel</tt> with
<tt>sh_new_arena</tt>, the memory for the deleted string
will be wasted until you delete the entire table with <tt>shfree</tt>.
<p>
The string hashmap also provides structure functions <tt>shgets</tt> and <tt>shputs</tt>. These
functions should not be used when using <tt>sh_new_arena</tt> and <tt>sh_new_strdup</tt>, as the
string found in the structure will always be used. You can do <tt>strdup()</tt> yourself on
keys. A simple string arena API is also provided that you can use as well.
<p>
The complete set of string hashmap functions is:
<p>
<ul>
<li><tt>shlen</tt> - The length of the hashmap array data for iterating the hashmap contents
<li><tt>shlenu</tt> - As above, but unsigned
<li><tt>shget</tt> - Get the value corresponding to a key, or return the default
<li><tt>shgets</tt> - Get the value corresponding to a key, or return the default
<li><tt>shgeti</tt> - Return the index into the array of a given key, or -1
<li><tt>shput</tt> - Update an existing <tt>key,value</tt> pair, or add a new one if the key isn't present
<li><tt>shputs</tt> - Update an existing structure, or add a new one if the key isn't present
<li><tt>shdel</tt> - Delete a <tt>key,value</tt> pair if the key is present
<li><tt>shdefault</tt> - Set the default value used by <tt>hmget</tt>
<li><tt>shdefaults</tt> - Set the default struct used by <tt>hmget</tt>
<li><tt>sh_new_arena</tt> - Overwrites pointer with a new hashmap using a string arena for string management
<li><tt>sh_new_strdup</tt> - Overwrites pointer with a new hashmap using strdup for string management
</ul><p>
In addition, the string arena has two functions:
<ul>
<li><tt>stbds_stralloc</tt> - Allocates a string in a string arena
<li><tt>stbds_strreset</tt> - Frees all the strings in a string arena
</ul><p>
<h2><a name="impl"></a>Implementation notes</h2>


All data structures use the trick of storing extra data "before" the pointed-to-array. In
the case of dynamic arrays this includes the length and capacity of the array. For
hash tables, a pointer is stored to a separately allocated hash table. The array
part of the hash table is actually valid from -1..`hmlen`, with -1 storing the default
value from <tt>hmdefault</tt>.
<p>
The code uses <tt>size_t</tt> as an unsigned type and <tt>ptrdiff_t</tt> as a signed type. These
are usually the same size as pointers (i.e. <tt>uintptr_t</tt>, but with compatibility with
older compilers), which are usually the same size as registers. However, there are
some compilation models in which pointers are only 32-bits on 64-bit architectures,
in which case these types gain an advantage for size overhead, but lose the ability
to compute hashes 64-bits-at-a-time.
<p>
String hashing is done with a simple but relatively good hash function which circular shifts
and adds characters, with a final mix.
<p>
Binary data hashing uses hard-coded fast-and-decent hash functions for 4- and 8-byte
data (8-byte is only on 64-bit platforms). All other data is hashed using a weakened form
of Siphash. If you define STBDS_SIPHASH_2_4 on 64-bit platforms, correct SipHash-2-4
will be used for all keys.
<p>
Hash tables are bucketed and cache-aligned for performance. However, because they use
open addressing with power-of-two table sizes, there will be performance spikes when
the table is resized.
<p>
The string arena begins by allocating blocks of size 512 bytes to minimize wastage,
but doubles these block sizes up to a size of 1MB to minimize the number of memory
allocations which must be performed.
<p>

</body></html>
