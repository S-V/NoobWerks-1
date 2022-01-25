# Base - foundation layer ( kernel, base framework ).

### It contains mostly basic math, OS abstraction layer.

It can be used as a base for pretty much any engine (of any game genre).

Dependencies: None.

Folders (modules):

Build - compile configuration.
Common - common platform-independent stuff.
System - low-level platform abstraction layer.
Debug - misc. helpers for debugging.
Types - basic types.
Memory - memory management.
Object - object model (class descriptors, object management, run-time type information, etc).
IO - input / output systems.
Job - job system.


Useful references:

Reflection in C++:
http://donw.io/post/reflection-cpp-2/


### TODO

- rename Tuple3::set() to Tuple3::make()

- NwDynamicString class, with '/' operator for concatenating paths.

- fixed dynamic array (to catch errors)
std::dynarray is a sequence container that encapsulates arrays with a size that is fixed at construction and does not change throughout the lifetime of the object.
http://www.enseignement.polytechnique.fr/informatique/INF478/docs/Cpp/en/cpp/container/dynarray.html

- Arrays::Add_NoResize() which checks capacity before and after and triggers an assert.

## References

### Optimization

Avoiding branches:
https://www.chessprogramming.org/Avoiding_Branches
