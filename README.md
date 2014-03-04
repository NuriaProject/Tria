Tria
====

Tria is a code-generator for the NuriaProject Framework which allows users to
access classes, methods, enums and fields (incl annotations of each) at run-time.
Additionally, gathered data can be exported as JSON data.

Dependencies
------------

Build-time:
- LLVM/Clang (Compatible to 3.4)
- Qt5 (Core is enough)

Run-time:
- Qt5

Features
--------

For the C++ generator:
- Introspection of structs and classes
- Discovering and calling member and static methods
- Construction and destroying of types unknown at compile-time
- Automatic registration of types at load-time
- Automatic registration in the Qt meta system
- Automatic registration of conversion methods (For use with Nuria::Variant)
- Custom annotations with a value of arbitary type
- Not dependent on a base-type
- Ability to discover types at run-time based on inheritance or annotations (See Nuria::MetaObject)

For the JSON generator:
- Output of class data as JSON formatted data
- Exports classes, methods (members, statics and constructors), enums, fields and annotations
- For documentation of the format see src/jsongenerator.hpp
