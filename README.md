Tria
====

Tria is a code-generator for the NuriaProject Framework which allows users to
access classes, methods, enums and fields (incl annotations of each) at run-time.
Additionally, gathered data can be exported as JSON data.

It's also possible to use Tria as code-generator by writing Lua scripts.
(See below)

Dependencies
------------

Build-time:
- LLVM/Clang (Compatible to 3.4)
- Qt5 (Core is enough)

Run-time:
- Qt5

Additionally, starting with Clang3.5, libClang depends on:
- pthread
- zlib
- libcurses

Features
--------

### For the C++ generator:
- Introspection of structs and classes
- Discovering and calling member and static methods
- Construction and destroying of types unknown at compile-time
- Automatic registration of types at load-time
- Automatic registration in the Qt meta system
- Automatic registration of conversion methods (For use with QVariant::convert)
- Custom annotations with a value of arbitary type
- Not dependent on a base-type
- Ability to discover types at run-time based on inheritance or annotations (See Nuria::MetaObject)

#### For the JSON generator:
- Output of class data as JSON formatted data
- Exports classes, methods (members, statics and constructors), enums, fields and annotations
- For documentation of the format see [Tria JSON format](https://github.com/NuriaProject/Framework/wiki/Tria-JSON-format)

### For the Lua generator:
- Ability to write custom generators
- See the wiki for more details: [Tria Lua API](https://github.com/NuriaProject/Framework/wiki/Tria-Lua-API)

As a side-note, both the C++/Nuria and the JSON generator are implemented using
the Lua generator!

License
-------

Tria is licensed under the LGPLv3. See COPYING.LESSER for more information.