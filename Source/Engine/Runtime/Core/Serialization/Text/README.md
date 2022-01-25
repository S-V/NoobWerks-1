SON is a JSON-like format for describing structured data in a human-readable form.
SON = simple object notation.


(@todo: select another name?
STD - simple text data?
TDN - text data notation?
)


SON supports structured, hierarchical data definitions.
A SON value can be of one of the following types:
- A scalar value: a 64-bit integer, a double precision floating-point number, or string;
- A list - a sequence of values of any type;
- An array - an ordered sequence of values of any type which can be accessed by array index;
- An object - an unordered collection of key-value pairs.


This folder contains a non-streaming, non-extractive (in-place) document-centric (DOM-style) SON parser.

//---------------------------------------------------------
@todo: should be impose the requirement on arrays

//---------------------------------------------------------

'=' could be dropped from assignments,
but, IMHO, delimiters enhance readability.

//===========================================
//	Comments
//===========================================
Single-line comments begin with the character ';' and continue until terminated by a new line.
Multi-line comments begin and end with the character '|'.

//===========================================
//	Literal Values
//===========================================
The syntax and types of floating point and integer literal values
correspond to the int, long, float, and double literals of the C/C++ language.
Future refinements of SDSL may introduce a richer set of literal values (e.g., hex/octal literals).

//===========================================
//	References:
//===========================================
JSON, JSON5, YAML, TOML, Lua, Python

LSON: Lucid Serialized Object Notation
https://github.com/hollasch/LSON

SDL (Simple Declarative Language):
http://www.ikayzo.org/display/SDL/Home

libconfig

TXON – Text Object Notation:
http://www.hxa.name/txon/

MicroXML

ZML (Zurin's Markup Language):
http://www.gamedev.ru/flame/forum/?id=140660

SINX (SINX Is Not XML):
http://www.gamedev.ru/flame/forum/?id=140595
http://mikle33.narod.ru/sinx

STOB (markup language):
http://stobml.org/tiki-index.php
https://code.google.com/p/stob/
http://www.gamedev.ru/code/forum/?id=164940

PokiConfig:
http://www.gamedev.ru/flame/forum/?id=134697

Windows INI files

Boost.PropertyTree and its INFO format:
http://www.boost.org/doc/libs/1_43_0/doc/html/property_tree.html

Smart Game Format:
http://www.red-bean.com/sgf/

Java application properties:
http://en.wikipedia.org/wiki/.properties

//===========================================
//	Implementations:
//===========================================
jsmn
js0n
rapidjson
sajson
ultraJSON
jansson
gason
VTD-XML

//===========================================
//	Useful:
//===========================================
Thoughts on simplified serialization formats:
http://ryanuber.com/04-11-2013/thoughts-on-serialization.html
