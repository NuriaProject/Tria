/* Copyright (c) 2014-2015, The Nuria Project
 * The NuriaProject Framework is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 * 
 * The NuriaProject Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with The NuriaProject Framework.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEFINITIONS_HPP
#define DEFINITIONS_HPP

#include "defs.hpp"
#undef bool

#include <QStringList>
#include <functional>

#include <QMap>
#include <QSet>

typedef QMap< QString, QString > StringMap;
typedef QSet< QString > StringSet;
class TriaAction;
class TimingNode;
class QIODevice;

class Definitions {
public:
	Definitions (const QStringList &sourceFiles);
	~Definitions ();
	
	/** Returns the list of source files. */
	QStringList sourceFiles () const;
	
	/** Adds \a theClass. */
	void addClassDefinition (const ClassDef &theClass);
	
	/** Returns all class definitions */
	QVector< ClassDef > classDefintions () const;
	
	/**
	 * Returns the types which should be declared.
	 * Avoided types over-rule this list. This list in turn over-rules
	 * "declareTypes".
	 */
	StringSet declaredTypes () const;
	
	/** Registers \a type as already Q_DECLARE_METATYPE'd. */
	void addDeclaredType (const QString &type);
	
	/** Returns \c true if \a type has already been declared. */
	bool isTypeDeclared (const QString &type);
	
	/** Returns the list of to-be-declared types. */
	QMap< QString, bool > declareTypes () const;
	
	/** Tells the generator to generate a metatype declaration. */
	void declareType (const QString &type, bool isFullyDeclared);
	
	/** Takes \a type out of the to-be-declared list. */
	void undeclareType (const QString &type);
	
	/**
	 * Adds a typedef from \a desugared to \a typeDef.
	 */
	void addTypeDef (const QString &desugared, const QString &typeDef);
	
	/**
	 * Returns a list of types which should be avoided. Mainly, these
	 * types don't offer value-semantics.
	 * 
	 * \note Please note, that T is not T* - The latter is always fine.
	 */
	StringSet avoidedTypes () const;
	
	/** Returns all typedefs. */
	StringMap typedefs () const;
	
	/** Returns \c true if \a type should be avoided. */
	bool isTypeAvoided (const QString &type);
	
	/** Will avoid \a type when generating code. */
	void avoidType (const QString &type);
	
	/** */
	TimingNode *timing () const;
	void parsingComplete ();
	
private:
	friend class TriaAction;
	
	void setTimingNode (TimingNode *node);
	
	void cleanUpClassDef (ClassDef &def);
	
	// 
	QStringList m_fileNames;
	StringSet m_declaredTypes;
	QMap< QString, bool > m_declareTypes;
	StringSet m_avoidedTypes;
	StringMap m_typeDefs;
	QVector< ClassDef > m_classes;
	TimingNode *m_timing = nullptr;
	
};

#endif // DEFINITIONS_HPP
