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

#ifndef DEFINITIONS_HPP
#define DEFINITIONS_HPP

#include "defs.hpp"

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
	StringSet declareTypes () const;
	
	/**
	 * Like declareTypes(), but removes all typedefs before returning
	 * the list. \sa addTypeDef
	 */
	StringSet declareTypesWithoutDuplicates () const;
	
	/** Tells the generator to generate a metatype declaration. */
	void declareType (const QString &type);
	
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
	StringSet m_declareTypes;
	StringSet m_avoidedTypes;
	StringMap m_typeDefs;
	QVector< ClassDef > m_classes;
	TimingNode *m_timing = nullptr;
	
};

#endif // DEFINITIONS_HPP
