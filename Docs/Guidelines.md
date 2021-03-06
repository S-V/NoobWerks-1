##	Coding Style Guidelines

###	Naming Conventions

In general, code should be as self documenting as possible. All names should be
composed of complete and descriptive words; words should not be shortened or
abbreviated except in rare and obvious (to the average developer) cases.


###	Coding style

Both structs and classes should using camel casing and begin with a capital letter.

Structs - C-style, POD-types.
Classes - types with virtual member functions and/or nontrivial ctors/dtors.

names of types start with a capital letter, often prefixed with "mx".




Structs that exist only in memory may end in *_m, structs that are stored on disk - *_d, structs that are stored in lookup tables - *_t.
e.g.
struct TextureHeader_d
{
	UINT32		magic;	//!< FourCC code
...
};



global functions are sometimes prefixed with "mx".

Names of classes and functions start with a capital letter
(but some low-level utility functions may start with a small letter (e.g. mxVec3::dot3, pxShapeMgr::_updateAabbs()))

renderer classes (public interfaces) start with "rx",
physics - "px"

F_... is used for static file scoped functions (e.g. F_DefaultAlloc).
global variables start with the letter 'g' for  (e.g. gUseHavok)

Names of recursive functions have suffix "_r", e.g. "PartitionBSPNodeWithPlane_r" or "recursiveReloadChangedFiles"
or "_R", e.g. DrawPortals_R (esp. if used in the renderer).

internal identifiers are prefixed with "_" (e.g. _internalTick(), _create_rigid_body_impl() ).


usually types declared with 'class' should be allocated from heap (it's true when they include MX_DECLARE_HEAP_OBJECT),
structs are usually C-style POD (Plain Old Data) types without virtual functions and nontrivial copy constructors...


== Pointers:

void Foobar( const Foo& p );	// p is not modified inside the function
void Foobar( const Foo* p );	// p is not modified inside the function, but better use a reference

void Foobar( int *a, int *b );	// function returns a and b

void Foobar( int * a, int * b );	// function uses passed values of a and b and modifies them

void Foobar( int & a );	// function uses value of a and modifies it

// function returns a, but it can be confusing - you may use a pointer (pass &a),
// but pointers can point at null while reference are almost always valid - decide for yourself!
void Foobar( int &a );

###	 General recommendations

Minimize code bloat!

Use (const char*, UINT length) in function arguments instead of (const String& str) - inlining of string ctor causes excessive bloat.
Besides, (const String& str) forces users of your code know about String.


### References

OMG: Our Machinery Guidebook
https://ourmachinery.com/files/guidebook.md.html

Google C++ Style Guide
https://google.github.io/styleguide/cppguide.html
