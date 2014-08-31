/* Copyright (c) 2014, The Nuria Project
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *    1. The origin of this software must not be misrepresented; you must not
 *       claim that you wrote the original software. If you use this software
 *       in a product, an acknowledgment in the product documentation would be
 *       appreciated but is not required.
 *    2. Altered source versions must be plainly marked as such, and must not be
 *       misrepresented as being the original software.
 *    3. This notice may not be removed or altered from any source
 *       distribution.
 */

#ifndef TST_HEADER_HPP
#define TST_HEADER_HPP

#include <nuria/essentials.hpp>
#include <QObject>
#include <QString>
#include <QPoint>

#include "tst_ignored.hpp"

// Namespaces are allowed by using the qualified name of types.
namespace Test {

// Forward declarations don't bring tria down.
class A;
struct B;

// We can find A by looking for classes with the "DoesItWork" annotation.
class NURIA_INTROSPECT NURIA_ANNOTATE(DoesItWork, 1 + 2) A {
	
	// Private symbols are ignored.
	int ignoredField;
	enum IgnoredEnum { IgnoredElement };
	void ignoredMethod () {}
	static void ignoredStatic () {}
	A (int a) { (void)a; }
	
	int m_unguarded;
	int m_guarded;
public:
	
	// Enums are sorted ..
	enum First {};
	enum Second {};
	
	// Test multiple annotations, test key <-> value
	enum
	NURIA_ANNOTATE(First, 0) NURIA_ANNOTATE(Second, 1)
	Numbers {
		One = 1,
		Two,
		Ten = 10,
		Hundred = 100
	};
	
	// 
	NURIA_ANNOTATE(IsACtor, true)
	A () { qDebug("ctor"); }
	~A () { qDebug("dtor"); }
	
	// We're using Nuria::Callback, thus we can do a simple test here.
	// If Nuria::Callback fails its own test will notice.
	
	NURIA_ANNOTATE(IsAStatic, true)
	static void callMeStatic () { qDebug("static"); }
	
	NURIA_ANNOTATE(IsAMember, true)
	void callMe () { qDebug("member"); }
	
	void callMe (int a) { qDebug("a=%i", a); }
	
	// Conversion operator
	operator Numbers () { return (Numbers)rawField; }
	
	NURIA_SKIP
	operator QPoint () { return QPoint(0, 0); }
	
	// Will be ignored because return type or soem argument doesn't have value semantics.
	QObject foo ();
	void foo (QObject a);
	
	// Raw fields
	NURIA_ANNOTATE(IsAField, true)
	int rawField;
	
	NURIA_ANNOTATE(IsAFieldToo, true)
	NURIA_REQUIRE(requireField >= 10 && requireField < 20)
	int requireField;
	
	// Fields with accessors
	NURIA_READ(unguarded)
	int unguarded () { return this->m_unguarded; }
	
	NURIA_WRITE(unguarded)
	void setUnguarded (int b) { this->m_unguarded = b; }
	
	NURIA_READ(guarded)
	int guarded () { return this->m_guarded; }
	
	NURIA_WRITE(guarded)
	NURIA_REQUIRE(b > 0)
	void setGuarded (int b) { this->m_guarded = b; }
	
	// Read-only field
	NURIA_READ(readOnly)
	int readOnlyValue () const { return 0xC0FFEE; }
	
};

// B inherits A. We'll be able to find B through this.
struct NURIA_INTROSPECT B : public A {
	int value;
	
	// All elements are stored sorted
	int c, a, b;
	
	// Needed to enable value-semantics.
	B () : value (-1) { }
	
	// 
	int sum (int a = 5, int b = 4, int c = 3)
	{ return a + b + c; }
	
	// methods with const* as result type are ignored
	const int *valueAsPtr () const { return &value; }
	
	// Test conversion detection
	B (const QString &s) { value = s.toInt (); }
	static B fromInt (int i) { B b; b.value = i; return b; }
	
	// to* member-converters, const vs non-const
	QByteArray toByteArray () { return QByteArray::number (value); }
	QString toString () const { return QString::number (value); }
	
};

// Class testing complex NURIA_REQUIREs on methods and fields
struct
NURIA_INTROSPECT
NURIA_ANNOTATE(A, "A")
NURIA_ANNOTATE(Foo, 1)
NURIA_ANNOTATE(Foo, 2)
NURIA_ANNOTATE(Foo, 3)
NURIA_ANNOTATE(Z, "Z")
C {
	int member;
	bool checker (int a, int b) {
		qDebug("checker=%i,%i", a, b);
		return (a < b);
	}
	
	NURIA_REQUIRE(name != "C")
	C (QString name) { qDebug("%s", qPrintable(name)); }
	
	NURIA_REQUIRE(foo != 0)
	static void staticVoid (int foo) { qDebug("staticVoid=%i", foo); }
	
	NURIA_REQUIRE(a != b)
	static int staticInt (int a, int b) { return a + b; }
	
	NURIA_REQUIRE(foo != member)
	void memberVoid (int foo) { qDebug("memberVoid=%i", foo); }
	
	NURIA_REQUIRE(checker(a, b))
	int memberInt (int a, int b) { return a + b; }
	
	NURIA_REQUIRE(checker (withCall, 5))
	int withCall;
	
	NURIA_REQUIRE(withField != member)
	int withField;
	
};

// Structure testing typedefs and templates
struct NURIA_INTROSPECT D {
	
	// typedef QFlags< MyFlag > MyFlags
	enum MyFlag { One = 1, Two = 2, Four = 4 };
	Q_DECLARE_FLAGS(MyFlags, MyFlag)
	
	// Operators are ignored for now.
	D &operator= (const D &other) { return *this; }
	
	// 
	QList< int > intList (MyFlags flags) { return QList< int > (); }
	
};

// Parent class is not introspected, only sub-classes are.
struct NoValueSemantics {
	NoValueSemantics &operator= (const NoValueSemantics &) = delete;
};

// Does not have value-semantics because of inheriting NoValueSemantics.
struct NURIA_INTROSPECT E : NoValueSemantics {
	int a;
};

// This struct will NOT be made available.
struct Ignored {};

}

// Tria won't declare already Q_DECLARE_METATYPE'd types.
Q_DECLARE_METATYPE(Test::A::Numbers)
Q_DECLARE_METATYPE(Test::A*)
Q_DECLARE_METATYPE(Test::A)
Q_DECLARE_METATYPE(Test::B*)
Q_DECLARE_METATYPE(Test::B)
Q_DECLARE_METATYPE(Test::D*)
Q_DECLARE_METATYPE(Test::D)
Q_DECLARE_METATYPE(Test::D::MyFlags)

#endif // TST_HEADER_HPP
