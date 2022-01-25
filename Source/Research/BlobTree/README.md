An Implicit Surfaces / Signed Distance Fields (SDF) library
================

Description
TBD

## Extension to manipulate Blobtrees (called Implicit Surfaces or SDF, Signed Distance Fields)

A library to manage implicit surfaces using a Blobtree.

### Main concepts ###
This library implements a blobtree representation.
A blobtree is a tree of implicit primitives generating a scalar field. Classic fields include distance fields, but other implicit functions can also be used.

### Usage ###

TBD

````
Code Snippet
````

#### Documentation ####
TBD

## Future Improvements ##

### Optimization ###

   TBD

### Signed Distance Fields ###

#### Nodes ####

   Signed Distance Fields support is minimalist for now. Should be added : union, intersection, difference, transformation (rotation, translation, scale...).

#### Materials ####

   For now only SDFRootNode can handle a material. Primitive nodes at least should define materials settings and evaluation.

### Architecture and Code ###

#### Automated regression tests ####

Tests should be added for automatic comparison of expected values. This is a required step before optimizing primitive computations.
Also, concerning analytical gradient, the same requirement applies : for all primtives analytical results should be compared with numerical one to check it is correct.

#### Deprecation and refactoring ####

TBD

### Architecture and Code ###



## References ##

### Papers ###

HyperFun
http://hyperfun.org/wiki/doku.php?id=hyperfun:main
https://hyperfun.org/frep/main
http://paulbourke.net/dataformats/hyperfun/HF_oper.html

Implementing the Blob Tree
https://web.uvic.ca/~etcwilde/papers/ImplementingTheBlobTree.pdf

Unifying Procedural Graphics (MSci Individual Project Report) [2009]
https://www.doc.ic.ac.uk/teaching/distinguished-projects/2009/d.birch.pdf



SDF Bounding Volumes - 2019
https://www.iquilezles.org/www/articles/sdfbounding/sdfbounding.htm
https://twitter.com/iquilezles/status/1166082636088922112?lang=en


Interval Arithmetic and Recursive Subdivision for Implicit Functions and Constructive Solid Geometry [1992]

https://locklessinc.com/articles/interval_arithmetic/

Fast SDF evaluation on CPU (using cache and SIMD optimizations):

https://software.intel.com/sites/default/files/m/5/0/1/Parsip_ExtendedAbstract.pdf

http://www.pouryashirazian.com/storage/publication/slides/2014_phd_oralexam.pdf



Additive manufacturing file format
https://en.wikipedia.org/wiki/Additive_manufacturing_file_format



### Implementations ###

Blobtree.js an Implicit Surfaces / Signed Distance Fields - SDF library for three.js
https://github.com/dualbox/three-js-blobtree