/* This file is part of the NuriaProject Framework. Copyright 2012 NuriaProject
 * The NuriaProject Framework is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 * The NuriaProject Framework is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with the library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <nuria/metaobject.hpp>

#include <QtTest/QtTest>
#include <QObject>
#include <QDate>

// Header processed by Tria
#include "tst_header.hpp"

class TriaTest : public QObject {
	Q_OBJECT
private slots:
	
	void checkThatTypesAreRegistered ();
	void getAllTypes ();
	void getTypesByAnnotation ();
	void getTypesByBase ();
	
	void verifyTypeA ();
	void verifyTypeB ();
	void verifyTypeD ();
	void fieldsAreSorted ();
	
	void verifyTemplateAndTypedefs ();
	
	void methodsAreSorted ();
	void enumsAreSorted ();
	void enumElementsAreSorted ();
	
	void checkEnumElements ();
	void checkEnumKeyToValue ();
	void checkEnumValueToKey ();
	
	void checkNonExistanceOfIgnoredElements ();
	void constPointerMethodsAreIgnored ();
	
	void checkClassAnnotations ();
	void checkMethodAnnotations ();
	void checkEnumAnnotations ();
	void checkFieldAnnotations ();
	void verifyMultipleAnnotations ();
	
	void checkOverloadedMethod ();
	void verifyMethodsMetaData ();
	void callMemberMethod ();
	void callStaticMethod ();
	void callOverloadedMethod ();
	void createAndDestroyInstance ();
	
	void verifyFieldType ();
	void verifyFieldAccess ();
	void readWriteRawField ();
	void readWriteRequireField ();
	void readWriteUnguarded ();
	void readWriteGuarded ();
	void readWriteReadOnly ();
	
	void testCtorConversion ();
	void testFromMethodConversion ();
	void testToMethodConversion ();
	void testOperatorConversion ();
	
	void callGuardedCtor ();
	void callGuardedStaticVoid ();
	void callGuardedStaticInt ();
	void callGuardedMemberVoid ();
	void callGuardedMemberInt ();
	
	void writeGuardedFieldWithMethod ();
	void writeGuardedFieldWithMember ();
	
};

// 
void TriaTest::checkThatTypesAreRegistered () {
	using namespace Nuria;
	
	QVERIFY (MetaObject::byName ("Test::A"));
	QVERIFY (MetaObject::byName ("Test::B"));
	QVERIFY (!MetaObject::byName ("Test::Ignored"));
	
}

void TriaTest::getAllTypes () {
	using namespace Nuria;
	
	MetaObjectMap expected { { "Test::A", MetaObject::byName ("Test::A") },
				 { "Test::B", MetaObject::byName ("Test::B") },
				 { "Test::C", MetaObject::byName ("Test::C") },
				 { "Test::D", MetaObject::byName ("Test::D") },
				 { "Test::E", MetaObject::byName ("Test::E") } };
	MetaObjectMap result = MetaObject::allTypes ();
	
	QCOMPARE(result, expected);
	
}

void TriaTest::getTypesByAnnotation () {
	using namespace Nuria;
	
	MetaObjectMap expected { { "Test::A", MetaObject::byName ("Test::A") } };
	MetaObjectMap result = MetaObject::typesWithAnnotation ("DoesItWork");
	
	QCOMPARE(result, expected);
	
}

void TriaTest::getTypesByBase () {
	using namespace Nuria;
	
	MetaObjectMap expected { { "Test::B", MetaObject::byName ("Test::B") } };
	MetaObjectMap result = MetaObject::typesInheriting ("Test::A");
	
	QCOMPARE(result, expected);
	
}

void TriaTest::verifyTypeA () {
	using namespace Nuria;
	
	MetaObject *a = MetaObject::byName ("Test::A");
	QVERIFY(a);
	QCOMPARE(a->metaTypeId (), qMetaTypeId< Test::A > ());
	QCOMPARE(a->pointerMetaTypeId (), qMetaTypeId< Test::A * > ());
	QCOMPARE(a->parents ().length (), 0);
	QCOMPARE(a->annotationCount (), 1);
	QCOMPARE(a->methodCount (), 5);
	QCOMPARE(a->fieldCount (), 5);
	QCOMPARE(a->enumCount (), 3);
	
	QVERIFY(a->method ({ "" }).isValid ());
	QVERIFY(a->method ({ "", "Test::A" }).isValid ());
	QVERIFY(a->method ({ "callMe" }).isValid ());
	QVERIFY(a->method ({ "callMe", "int" }).isValid ());
	QVERIFY(a->method ({ "callMeStatic" }).isValid ());
	
}

void TriaTest::verifyTypeB () {
	using namespace Nuria;
	
	MetaObject *b = MetaObject::byName ("Test::B");
	QVERIFY(b);
	QCOMPARE(b->metaTypeId (), qMetaTypeId< Test::B > ());
	QCOMPARE(b->pointerMetaTypeId (), qMetaTypeId< Test::B * > ());
	QCOMPARE(b->parents ().length (), 1);
	QCOMPARE(b->annotationCount (), 0);
	QCOMPARE(b->methodCount (), 10);
	QCOMPARE(b->fieldCount (), 4);
	QCOMPARE(b->enumCount (), 0);
	
	QVERIFY(b->method ({ "" }).isValid ());
	QVERIFY(b->method ({ "", "Test::B" }).isValid ());
	QVERIFY(b->method ({ "", "QString" }).isValid ());
	QVERIFY(b->method ({ "sum" }).isValid ());
	QVERIFY(b->method ({ "sum", "int" }).isValid ());
	QVERIFY(b->method ({ "sum", "int", "int" }).isValid ());
	QVERIFY(b->method ({ "sum", "int", "int", "int" }).isValid ());
	QVERIFY(b->method ({ "fromInt", "int" }).isValid ());
	QVERIFY(b->method ({ "toByteArray" }).isValid ());
	QVERIFY(b->method ({ "toString" }).isValid ());
	
}

void TriaTest::verifyTypeD () {
	using namespace Nuria;
	
	MetaObject *d = MetaObject::byName ("Test::D");
	QVERIFY(d);
	QCOMPARE(d->metaTypeId (), qMetaTypeId< Test::D > ());
	QCOMPARE(d->pointerMetaTypeId (), qMetaTypeId< Test::D * > ());
	QCOMPARE(d->parents ().length (), 0);
	QCOMPARE(d->annotationCount (), 0);
	QCOMPARE(d->methodCount (), 3);
	QCOMPARE(d->fieldCount (), 0);
	QCOMPARE(d->enumCount (), 1);
	
	QVERIFY(d->method ({ "" }).isValid ());
	QVERIFY(d->method ({ "", "Test::D" }).isValid ());
	QVERIFY(d->method ({ "intList", "Test::D::MyFlags" }).isValid ());
	
}

void TriaTest::fieldsAreSorted () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::B");
	
	QStringList expected { "a", "b", "c", "value" };
	QStringList result;
	for (int i = 0; i < meta->fieldCount (); i++) {
		result << meta->field (i).name ();
	}
	
	// 
	QCOMPARE(result, expected);
	
}

void TriaTest::verifyTemplateAndTypedefs () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::D");
	Nuria::MetaMethod method = meta->method ({ "intList", "Test::D::MyFlags" });
	
	auto argTypes = method.argumentTypes ();
	
	QCOMPARE(method.returnType ().data (), "QList<int>");
	QCOMPARE(argTypes.length (), 1);
	QCOMPARE(argTypes.first ().data (), "Test::D::MyFlags");
	
}

void TriaTest::methodsAreSorted () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	QStringList expected { "", "", "callMe", "callMe", "callMeStatic" };
	QStringList result;
	for (int i = 0; i < meta->methodCount (); i++) {
		result << meta->method (i).name ();
	}
	
	// 
	QCOMPARE(result, expected);
	
}

void TriaTest::enumsAreSorted () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	QStringList expected { "First", "Numbers", "Second" };
	QStringList result;
	for (int i = 0; i < meta->enumCount (); i++) {
		result << meta->enumAt (i).name ();
	}
	
	// 
	QCOMPARE(result, expected);
	
}

void TriaTest::enumElementsAreSorted () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaEnum theEnum = meta->enumByName ("Numbers");
	
	QStringList expected { "Hundred", "One", "Ten", "Two" };
	QStringList result;
	for (int i = 0; i < theEnum.elementCount (); i++) {
		result << theEnum.key (i);
	}
	
	// 
	QVERIFY(theEnum.isValid ());
	QCOMPARE(result, expected);
	
}

void TriaTest::checkEnumElements () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaEnum theEnum = meta->enumByName ("Numbers");
	
	QMap< QByteArray, int > expected { { "One", 1 }, { "Two", 2 }, { "Ten", 10 }, { "Hundred", 100 } };
	QMap< QByteArray, int > result;
	for (int i = 0; i < theEnum.elementCount (); i++) {
		result.insert (theEnum.key (i), theEnum.value (i));
	}
	
	// 
	QVERIFY(theEnum.isValid ());
	QCOMPARE(result, expected);
	
}

void TriaTest::checkEnumKeyToValue () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaEnum theEnum = meta->enumByName ("Numbers");
	
	QCOMPARE(theEnum.keyToValue ("One"), 1);
	QCOMPARE(theEnum.keyToValue ("Two"), 2);
	QCOMPARE(theEnum.keyToValue ("Ten"), 10);
	QCOMPARE(theEnum.keyToValue ("Hundred"), 100);
}

void TriaTest::checkEnumValueToKey () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaEnum theEnum = meta->enumByName ("Numbers");
	
	QCOMPARE(theEnum.valueToKey (1), QByteArray ("One"));
	QCOMPARE(theEnum.valueToKey (2), QByteArray ("Two"));
	QCOMPARE(theEnum.valueToKey (10), QByteArray ("Ten"));
	QCOMPARE(theEnum.valueToKey (100), QByteArray ("Hundred"));
}

void TriaTest::checkNonExistanceOfIgnoredElements () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	QVERIFY(!meta->fieldByName ("ignoredField").isValid ());
	QVERIFY(!meta->enumByName ("ignoredEnum").isValid ());
	QVERIFY(!meta->method ({ "ignoredMethod" }).isValid ());
	QVERIFY(!meta->method ({ "ignoredStatic" }).isValid ());
	QVERIFY(!meta->method ({ "", "int" }).isValid ()); // Constructor
	
	// Skipped conversion operator
	QVERIFY(!QMetaType::hasRegisteredConverterFunction (qMetaTypeId< Test::A > (), qMetaTypeId< QPoint > ()));
	
}

void TriaTest::constPointerMethodsAreIgnored () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::B");
	QVERIFY(!meta->method ({ "valueAsPtr" }).isValid ());
	
}

void TriaTest::checkOverloadedMethod () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::B");
	
	int lower = meta->methodLowerBound ("sum");
	int upper = meta->methodUpperBound ("sum");
	QCOMPARE(upper - lower + 1, 4);
	
	// Sorted by argument count in ascending manner.
	for (int i = 0; lower <= upper; i++, lower++) {
		QCOMPARE(meta->method (lower).argumentTypes ().length (), i);
	}
	
}

template< typename ... Args >
static inline QVector< QByteArray > getList (Args ... args) {
	return QVector< QByteArray > { args ... };
}

void TriaTest::verifyMethodsMetaData () {
	using namespace Nuria;
	MetaObject *meta = Nuria::MetaObject::byName ("Test::B");
	
	// Implicitly verifies MetaMethod::argumentTypes().
	MetaMethod ctorDefault = meta->method({ "" });
	MetaMethod ctorString = meta->method({ "", "QString" });
	MetaMethod sumZero = meta->method({ "sum" });
	MetaMethod sumOne = meta->method({ "sum", "int" });
	MetaMethod sumTwo = meta->method({ "sum", "int", "int" });
	MetaMethod sumThree = meta->method({ "sum", "int", "int", "int" });
	MetaMethod fromInt = meta->method({ "fromInt", "int" });
	MetaMethod toByteArray = meta->method({ "toByteArray" });
	MetaMethod toString = meta->method({ "toString" });
	
	// 
	QVERIFY(!ctorDefault.annotationCount ());
	QVERIFY(!ctorString.annotationCount ());
	QVERIFY(!sumZero.annotationCount ());
	QVERIFY(!sumOne.annotationCount ());
	QVERIFY(!sumTwo.annotationCount ());
	QVERIFY(!sumThree.annotationCount ());
	QVERIFY(!fromInt.annotationCount ());
	QVERIFY(!toByteArray.annotationCount ());
	QVERIFY(!toString.annotationCount ());
	
	// 
	QCOMPARE(ctorDefault.type (), MetaMethod::Constructor);
	QCOMPARE(ctorString.type (), MetaMethod::Constructor);
	QCOMPARE(sumZero.type (), MetaMethod::Method);
	QCOMPARE(sumOne.type (), MetaMethod::Method);
	QCOMPARE(sumTwo.type (), MetaMethod::Method);
	QCOMPARE(sumThree.type (), MetaMethod::Method);
	QCOMPARE(fromInt.type (), MetaMethod::Static);
	QCOMPARE(toByteArray.type (), MetaMethod::Method);
	QCOMPARE(toString.type (), MetaMethod::Method);
	
	// 
	QCOMPARE(ctorDefault.returnType (), QByteArray ("Test::B"));
	QCOMPARE(ctorString.returnType (), QByteArray ("Test::B"));
	QCOMPARE(sumZero.returnType (), QByteArray ("int"));
	QCOMPARE(sumOne.returnType (), QByteArray ("int"));
	QCOMPARE(sumTwo.returnType (), QByteArray ("int"));
	QCOMPARE(sumThree.returnType (), QByteArray ("int"));
	QCOMPARE(fromInt.returnType (), QByteArray ("Test::B"));
	QCOMPARE(toByteArray.returnType (), QByteArray ("QByteArray"));
	QCOMPARE(toString.returnType (), QByteArray ("QString"));
	
	// 
	QCOMPARE(ctorDefault.argumentNames (), getList ());
	QCOMPARE(ctorString.argumentNames (), getList ("s"));
	QCOMPARE(sumZero.argumentNames (), getList ());
	QCOMPARE(sumOne.argumentNames (), getList ("a"));
	QCOMPARE(sumTwo.argumentNames (), getList ("a", "b"));
	QCOMPARE(sumThree.argumentNames (), getList ("a", "b", "c"));
	QCOMPARE(fromInt.argumentNames (), getList ("i"));
	QCOMPARE(toByteArray.argumentNames (), getList ());
	QCOMPARE(toString.argumentNames (), getList ());
	
}

template< typename T >
static QVariantMap allAnnotations (T &from) {
	QVariantMap map;
	for (int i = 0; i < from.annotationCount (); i++) {
		Nuria::MetaAnnotation a = from.annotation (i);
		map.insert (a.name (), a.value ());
	}
	
	return map;
}

void TriaTest::checkClassAnnotations () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	QVariantMap expect { { "DoesItWork", 1 + 2 } };
	QCOMPARE(allAnnotations (*meta), expect);
	
}

void TriaTest::checkMethodAnnotations () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaMethod ctor = meta->method ({ "" });
	Nuria::MetaMethod member = meta->method ({ "callMe" });
	Nuria::MetaMethod memberTwo = meta->method ({ "callMe", "int" });
	Nuria::MetaMethod staticMethod = meta->method ({ "callMeStatic" });
	
	QVariantMap expectCtor { { "IsACtor", true } };
	QVERIFY(ctor.isValid ());
	QCOMPARE(allAnnotations (ctor), expectCtor);
	
	QVariantMap expectMember { { "IsAMember", true } };
	QVERIFY(member.isValid ());
	QCOMPARE(allAnnotations (member), expectMember);
	
	QVariantMap expectMemberTwo;
	QVERIFY(memberTwo.isValid ());
	QCOMPARE(allAnnotations (memberTwo), expectMemberTwo);
	
	QVariantMap expectStatic { { "IsAStatic", true } };
	QVERIFY(staticMethod.isValid ());
	QCOMPARE(allAnnotations (staticMethod), expectStatic);
	
}

void TriaTest::checkEnumAnnotations () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	Nuria::MetaEnum first = meta->enumByName ("First");
	Nuria::MetaEnum second = meta->enumByName ("Second");
	Nuria::MetaEnum numbers = meta->enumByName ("Numbers");
	
	QVariantMap expectEmpty;
	QVariantMap expectNumbers { { "First", 0 }, { "Second", 1 } };
	
	QCOMPARE(allAnnotations (first), expectEmpty);
	QCOMPARE(allAnnotations (second), expectEmpty);
	QCOMPARE(allAnnotations (numbers), expectNumbers);
	
}

void TriaTest::checkFieldAnnotations () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField field = meta->fieldByName ("rawField");
	
	QVariantMap expect { { "IsAField", true } };
	QCOMPARE(allAnnotations (field), expect);
	
}

void TriaTest::verifyMultipleAnnotations () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::C");
	
	// Note: The annotation{Lower,Upper}Bound implementation is shared
	// across all Meta* classes.
	
	int lower = meta->annotationLowerBound ("Foo");
	int upper = meta->annotationUpperBound ("Foo");
	
	QCOMPARE(meta->annotationCount (), 5);
	QCOMPARE(lower, 1);
	QCOMPARE(upper, 3);
	QCOMPARE(meta->annotation (1).name ().data (), "Foo");
	QCOMPARE(meta->annotation (2).name ().data (), "Foo");
	QCOMPARE(meta->annotation (3).name ().data (), "Foo");
	QCOMPARE(meta->annotation (1).value ().toInt (), 1);
	QCOMPARE(meta->annotation (2).value ().toInt (), 2);
	QCOMPARE(meta->annotation (3).value ().toInt (), 3);
	
}

void TriaTest::callMemberMethod () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	// Supress debug output
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	Test::A a;
	
	QTest::ignoreMessage (QtDebugMsg, "member");
	meta->method ({ "callMe" }).callback (&a) ();
	
	QTest::ignoreMessage (QtDebugMsg, "a=5");
	meta->method ({ "callMe", "int" }).callback (&a) (5);
	
}

void TriaTest::callStaticMethod () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	QTest::ignoreMessage (QtDebugMsg, "static");
	meta->method ({ "callMeStatic" }).callback () ();
	
}

void TriaTest::callOverloadedMethod () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::B");
	
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	Test::B b;
	
	QCOMPARE(meta->method ({ "sum" }).callback (&b) ().toInt (), 5 + 4 + 3);
	QCOMPARE(meta->method ({ "sum", "int" }).callback (&b) (7).toInt (), 7 + 4 + 3);
	QCOMPARE(meta->method ({ "sum", "int", "int" }).callback (&b) (7, 9).toInt (), 7 + 9 + 3);
	QCOMPARE(meta->method ({ "sum", "int", "int", "int" }).callback (&b) (7, 9, 11).toInt (), 7 + 9 + 11);
	
}

void TriaTest::createAndDestroyInstance () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	
	// 
	Nuria::MetaMethod ctor = meta->method ({ "" });
	
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	Test::A *a = ctor.callback () ().value< Test::A * > ();
	
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	meta->destroyInstance (a);
	
}

void TriaTest::verifyFieldType () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField rawField = meta->fieldByName ("rawField");
	Nuria::MetaField requireField = meta->fieldByName ("requireField");
	Nuria::MetaField unguarded = meta->fieldByName ("unguarded");
	Nuria::MetaField guarded = meta->fieldByName ("guarded");
	Nuria::MetaField readOnly = meta->fieldByName ("readOnly");
	
	QCOMPARE(rawField.typeName (), QByteArray ("int"));
	QCOMPARE(requireField.typeName (), QByteArray ("int"));
	QCOMPARE(unguarded.typeName (), QByteArray ("int"));
	QCOMPARE(guarded.typeName (), QByteArray ("int"));
	QCOMPARE(readOnly.typeName (), QByteArray ("int"));
}

void TriaTest::verifyFieldAccess () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField rawField = meta->fieldByName ("rawField");
	Nuria::MetaField requireField = meta->fieldByName ("requireField");
	Nuria::MetaField unguarded = meta->fieldByName ("unguarded");
	Nuria::MetaField guarded = meta->fieldByName ("guarded");
	Nuria::MetaField readOnly = meta->fieldByName ("readOnly");
	Nuria::MetaField nonExistant = meta->fieldByName ("Tria");
	
	QCOMPARE(rawField.access (), Nuria::MetaField::ReadWrite);
	QCOMPARE(requireField.access (), Nuria::MetaField::ReadWrite);
	QCOMPARE(unguarded.access (), Nuria::MetaField::ReadWrite);
	QCOMPARE(guarded.access (), Nuria::MetaField::ReadWrite);
	QCOMPARE(readOnly.access (), Nuria::MetaField::ReadOnly);
	QCOMPARE(nonExistant.access (), Nuria::MetaField::NoAccess);
}

void TriaTest::readWriteRawField () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField field = meta->fieldByName ("rawField");
	
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	Test::A a;
	
	a.rawField = 0;
	QVERIFY(field.write (&a, 123));
	QCOMPARE(a.rawField, 123);
	QCOMPARE(field.read (&a).toInt (), 123);
}

void TriaTest::readWriteRequireField () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField field = meta->fieldByName ("requireField");
	
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	Test::A a;
	
	a.requireField = 0;
	QVERIFY(field.write (&a, 10));
	QCOMPARE(a.requireField, 10);
	QCOMPARE(field.read (&a).toInt (), 10);
	
	QVERIFY(!field.write (&a, 20));
	QCOMPARE(a.requireField, 10);
	QCOMPARE(field.read (&a).toInt (), 10);
	
}

void TriaTest::readWriteUnguarded () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField field = meta->fieldByName ("unguarded");
	
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	Test::A a;
	
	a.setUnguarded (0);
	QVERIFY(field.write (&a, 123));
	QCOMPARE(a.unguarded (), 123);
	QCOMPARE(field.read (&a).toInt (), 123);
	
}

void TriaTest::readWriteGuarded () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField field = meta->fieldByName ("guarded");
	
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	Test::A a;
	
	a.setGuarded (0);
	QVERIFY(field.write (&a, 1));
	QCOMPARE(a.guarded (), 1);
	QCOMPARE(field.read (&a).toInt (), 1);
	
	QVERIFY(!field.write (&a, -1));
	QCOMPARE(a.guarded (), 1);
	QCOMPARE(field.read (&a).toInt (), 1);
	
}


void TriaTest::readWriteReadOnly () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::A");
	Nuria::MetaField field = meta->fieldByName ("readOnly");
	
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	Test::A a;
	
	QVERIFY(!field.write (&a, 1));
	QCOMPARE(field.read (&a).toInt (), 0xC0FFEE);
	
	QVERIFY(!field.write (&a, 0xC0FFEE));
	QCOMPARE(field.read (&a).toInt (), 0xC0FFEE);
	
}

void TriaTest::testCtorConversion () {
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	
	QVariant result = QString ("42");
	QVERIFY(result.convert (qMetaTypeId< Test::B > ()));
	QVERIFY(result.userType () == qMetaTypeId< Test::B > ());
	QCOMPARE(result.value< Test::B > ().value, 42);
	
}

void TriaTest::testFromMethodConversion () {
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	
	QVariant result (42);
	QVERIFY(result.convert (qMetaTypeId< Test::B > ()));
	QVERIFY(result.userType () == qMetaTypeId< Test::B > ());
	QCOMPARE(result.value< Test::B > ().value, 42);
}

void TriaTest::testToMethodConversion () {
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	
	Test::B b;
	b.value = 1337;
	
	QVariant result = QVariant::fromValue (b);
	QVERIFY(result.convert (QMetaType::QString));
	QVERIFY(result.userType () == qMetaTypeId< QString > ());
	QCOMPARE(result.toString ().toInt (), 1337);
	
}

void TriaTest::testOperatorConversion () {
	QTest::ignoreMessage (QtDebugMsg, "ctor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	QTest::ignoreMessage (QtDebugMsg, "dtor");
	
	Test::A a;
	a.rawField = Test::A::Hundred;
	
	QVariant result = QVariant::fromValue (a);
	QVERIFY(result.convert (qMetaTypeId< Test::A::Numbers > ()));
	QVERIFY(result.userType () == qMetaTypeId< Test::A::Numbers > ());
	QCOMPARE(result.value< Test::A::Numbers > (), Test::A::Hundred);
}

void TriaTest::callGuardedCtor () {
	using namespace Nuria;
	MetaObject *meta = MetaObject::byName ("Test::C");
	MetaMethod ctor = meta->method ({ "", "QString" });
	QVERIFY(ctor.isValid ());
	
	Callback guarded = ctor.callback ();
	Callback unguarded = ctor.unsafeCallback ();
	Callback test = ctor.testCallback ();
	
	QVERIFY(guarded.isValid ());
	QVERIFY(unguarded.isValid ());
	QVERIFY(test.isValid ());
	
	QTest::ignoreMessage (QtDebugMsg, "Foo");
	QTest::ignoreMessage (QtDebugMsg, "C");
	QCOMPARE(guarded (QString ("C")).isValid (), false);
	QCOMPARE(guarded (QString ("Foo")).isValid (), true);
	QCOMPARE(unguarded (QString ("C")).isValid (), true);
	QCOMPARE(test (QString ("Foo")).toBool (), true);
	QCOMPARE(test (QString ("C")).toBool (), false);
	
}

void TriaTest::callGuardedStaticVoid () {
	using namespace Nuria;
	MetaObject *meta = MetaObject::byName ("Test::C");
	MetaMethod staticVoid = meta->method ({ "staticVoid", "int" });
	QVERIFY(staticVoid.isValid ());
	
	Callback guarded = staticVoid.callback ();
	Callback unguarded = staticVoid.unsafeCallback ();
	Callback test = staticVoid.testCallback ();
	
	QVERIFY(guarded.isValid ());
	QVERIFY(unguarded.isValid ());
	QVERIFY(test.isValid ());
	QCOMPARE(test (1).toBool (), true);
	QCOMPARE(test (0).toBool (), false);
	
	QTest::ignoreMessage (QtDebugMsg, "staticVoid=1");
	QTest::ignoreMessage (QtDebugMsg, "staticVoid=2");
	guarded (0);
	guarded (1);
	unguarded (2);
	
}

void TriaTest::callGuardedStaticInt () {
	using namespace Nuria;
	MetaObject *meta = MetaObject::byName ("Test::C");
	MetaMethod staticInt = meta->method ({ "staticInt", "int", "int" });
	QVERIFY(staticInt.isValid ());
	
	Callback guarded = staticInt.callback ();
	Callback unguarded = staticInt.unsafeCallback ();
	Callback test = staticInt.testCallback ();
	
	QVERIFY(guarded.isValid ());
	QVERIFY(unguarded.isValid ());
	QVERIFY(test.isValid ());
	QCOMPARE(test (1, 2).toBool (), true);
	QCOMPARE(test (1, 1).toBool (), false);
	
	QCOMPARE(guarded (1, 1).isValid (), false);
	QCOMPARE(guarded (2, 3).toInt (), 2 + 3);
	QCOMPARE(unguarded (1, 1).toInt (), 1 + 1);
	
}

void TriaTest::callGuardedMemberVoid () {
	using namespace Nuria;
	MetaObject *meta = MetaObject::byName ("Test::C");
	MetaMethod memberVoid = meta->method ({ "memberVoid", "int" });
	QVERIFY(memberVoid.isValid ());
	
	QTest::ignoreMessage (QtDebugMsg, "");
	Test::C c ("");
	c.member = 5;
	
	Callback guarded = memberVoid.callback (&c);
	Callback unguarded = memberVoid.unsafeCallback (&c);
	Callback test = memberVoid.testCallback (&c);
	
	QVERIFY(guarded.isValid ());
	QVERIFY(unguarded.isValid ());
	QVERIFY(test.isValid ());
	QCOMPARE(test (4).toBool (), true);
	QCOMPARE(test (c.member).toBool (), false);
	
	QTest::ignoreMessage (QtDebugMsg, "memberVoid=1");
	QTest::ignoreMessage (QtDebugMsg, "memberVoid=2");
	guarded (c.member);
	guarded (1);
	unguarded (2);
	
}

void TriaTest::callGuardedMemberInt () {
	using namespace Nuria;
	MetaObject *meta = MetaObject::byName ("Test::C");
	MetaMethod memberInt = meta->method ({ "memberInt", "int", "int" });
	QVERIFY(memberInt.isValid ());
	
	QTest::ignoreMessage (QtDebugMsg, "");
	Test::C c ("");
	c.member = 5;
	
	Callback guarded = memberInt.callback (&c);
	Callback unguarded = memberInt.unsafeCallback (&c);
	Callback test = memberInt.testCallback (&c);
	
	QVERIFY(guarded.isValid ());
	QVERIFY(unguarded.isValid ());
	QVERIFY(test.isValid ());
	
	QTest::ignoreMessage (QtDebugMsg, "checker=1,2");
	QTest::ignoreMessage (QtDebugMsg, "checker=2,1");
	
	QCOMPARE(test (1, 2).toBool (), true);
	QCOMPARE(test (2, 1).toBool (), false);
	
	QTest::ignoreMessage (QtDebugMsg, "checker=1,1");
	QTest::ignoreMessage (QtDebugMsg, "checker=2,3");
	QCOMPARE(guarded (1, 1).isValid (), false);
	QCOMPARE(guarded (2, 3).toInt (), 2 + 3);
	QCOMPARE(unguarded (1, 1).toInt (), 1 + 1);
}

void TriaTest::writeGuardedFieldWithMethod () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::C");
	Nuria::MetaField withCall = meta->fieldByName ("withCall");
	QVERIFY(withCall.isValid ());
	
	QTest::ignoreMessage (QtDebugMsg, "");
	Test::C c ("");
	c.withCall = 1;
	
	QTest::ignoreMessage (QtDebugMsg, "checker=5,5");
	QTest::ignoreMessage (QtDebugMsg, "checker=4,5");
	QVERIFY(!withCall.write (&c, 5));
	QCOMPARE(c.withCall, 1);
	QVERIFY(withCall.write (&c, 4));
	QCOMPARE(c.withCall, 4);
}

void TriaTest::writeGuardedFieldWithMember () {
	Nuria::MetaObject *meta = Nuria::MetaObject::byName ("Test::C");
	Nuria::MetaField withField = meta->fieldByName ("withField");
	QVERIFY(withField.isValid ());
	
	QTest::ignoreMessage (QtDebugMsg, "");
	Test::C c ("");
	c.member = 5;
	c.withField = 1;
	
	QVERIFY(!withField.write (&c, c.member));
	QCOMPARE(c.withField, 1);
	QVERIFY(withField.write (&c, 4));
	QCOMPARE(c.withField, 4);
	
}

QTEST_MAIN(TriaTest)
#include "tst_tria.moc"
