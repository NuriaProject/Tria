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

#include "triaaction.hpp"

#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/CommandLine.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/Version.h>

#include "triaastconsumer.hpp"
#include "definitions.hpp"

#include <QString>
#include <QDebug>
#include <QTime>

#ifdef Q_OS_UNIX
#include <sys/time.h>
#endif

namespace {
using namespace llvm;

cl::opt< bool > argInspectAll ("introspect-all", cl::ValueDisallowed,
			       cl::desc ("All types will be introspected as if they had a NURIA_INTROSPECT annotation. "
					 "Types with NURIA_SKIP will be ignored."));
cl::list< std::string > argInspectBases ("introspect-inheriting", cl::CommaSeparated,
					 cl::desc ("Introspect all types which inherit <type>."),
					 cl::value_desc ("type1,typeN,..."));
cl::opt< bool > argVerboseTimes ("verbose-times", cl::ValueDisallowed,
                                 cl::desc ("Print detailed timings of all processed input files"));
cl::opt< std::string > argGlobalClass ("global-class", cl::value_desc ("class name"),
                                       cl::desc ("Fake class to put globals (methods, enums) into"));

// Aliases
cl::alias aliasInspectBases ("B", cl::Prefix, cl::desc ("Alias for -introspect-inheriting"),
			     cl::aliasopt (argInspectBases));

}

TriaAction::TriaAction (Definitions *definitions)
        : m_definitions (definitions)
{ }

static QByteArray sourceFileName (const QStringList &files) {
	if (files.length () == 1) {
		return files.first ().toLatin1 ();
	}
	
	// 
	return QByteArrayLiteral("Source files");
}

#if CLANG_VERSION_MINOR < 6
#define UNIQUE_COMPAT(Type, Name, Value) Type *Name = Value
#define MOVE_COMPAT(X) (X)
#else
#define UNIQUE_COMPAT(Type, Name, Value) std::unique_ptr< Type > Name (Value)
#define MOVE_COMPAT(X) std::move(X)
#endif

TriaAction::CreateAstConsumerResultType TriaAction::CreateASTConsumer (clang::CompilerInstance &ci,
                                                                       llvm::StringRef fileName) {
	ci.getFrontendOpts().SkipFunctionBodies = true;
	ci.getPreprocessor().enableIncrementalProcessing (true);
	ci.getLangOpts().DelayedTemplateParsing = true;
	
	// Enable everything for code compatibility
	ci.getLangOpts().MicrosoftExt = true;
	ci.getLangOpts().DollarIdents = true;
	ci.getLangOpts().CPlusPlus11 = true;
	ci.getLangOpts().GNUMode = true;
	
#if CLANG_VERSION_MINOR < 6
	ci.getLangOpts().CPlusPlus1y = true;
#else
	ci.getLangOpts().CPlusPlus14 = true;
#endif
	
	if (argVerboseTimes) {
		UNIQUE_COMPAT(PreprocessorHooks, hook, new PreprocessorHooks (ci));
		hook->timing ()->name = sourceFileName (this->m_definitions->sourceFiles ());
		this->m_definitions->setTimingNode (hook->timing ());
		ci.getPreprocessor ().addPPCallbacks (MOVE_COMPAT(hook));
	}
	
	// 
	QStringList whichInherit;
	for (const std::string &cur : argInspectBases) {
		whichInherit.append (QString::fromStdString (cur));
	}
	
	// 
	TriaASTConsumer *consumer = new TriaASTConsumer (ci, fileName, whichInherit, argInspectAll,
	                                                 argGlobalClass, this->m_definitions);
	
#if CLANG_VERSION_MINOR < 6
	return consumer;
#else
	return std::unique_ptr< clang::ASTConsumer > (consumer);
#endif
}

static inline qint64 nowUsec () {
#ifdef Q_OS_UNIX
	timeval tv;
	::gettimeofday (&tv, nullptr);
	
	return qint64 (tv.tv_sec * 1000000) + tv.tv_usec;
#else
	return qint64 (QTime::currentTime ().msecsSinceStartOfDay ()) * 1000;
#endif
}

PreprocessorHooks::PreprocessorHooks (clang::CompilerInstance &ci)
        : m_compiler (ci), m_root (new TimingNode (QByteArray ())), m_current (m_root)
{
	
	this->m_root->startTime = nowUsec ();
	
}

PreprocessorHooks::~PreprocessorHooks () {
	// 
}

TimingNode *PreprocessorHooks::timing () const {
	return this->m_root;
}

void PreprocessorHooks::FileChanged (clang::SourceLocation loc, clang::PPCallbacks::FileChangeReason reason,
                                     clang::SrcMgr::CharacteristicKind fileType, clang::FileID prevFID) {
	qint64 now = nowUsec ();
	
	if (reason == EnterFile) { // Stop parents' time
		TimingNode *p = this->m_current->parent;
		if (p) {
			p->time += now - p->startTime;
		}
		
	} else if (reason == ExitFile) { // Leaving
		goUp (false);
	}
	
	// 
	this->m_current->startTime = now;
	
}

void PreprocessorHooks::FileSkipped (const clang::FileEntry &, const clang::Token &,
                                     clang::SrcMgr::CharacteristicKind) {
	goUp (true);
}

void PreprocessorHooks::InclusionDirective (clang::SourceLocation, const clang::Token &, clang::StringRef, bool,
                                            clang::CharSourceRange, const clang::FileEntry *file, clang::StringRef,
                                            clang::StringRef, const clang::Module *) {
	
	this->m_curFile = QByteArray (file->getName ());
	
	TimingNode *node = new TimingNode (this->m_curFile, this->m_current);
	
	this->m_current->children.append (node);
	this->m_current = node;
}

void PreprocessorHooks::goUp (bool skipped) {
	if (!skipped) {
		this->m_current->stop ();
	}
	
	// 
	if (this->m_current->parent) { // Start parents' time again
		this->m_current = this->m_current->parent;
		this->m_current->startTime = nowUsec ();
		
		if (skipped) {
			delete this->m_current->children.takeLast ();
		}
		
	}
	
}

TimingNode::TimingNode (const QByteArray &n, TimingNode *p)
        : name (n), parent (p)
{
	
	this->initTime = nowUsec ();
	
}

TimingNode::~TimingNode () {
	qDeleteAll (children);
}

void TimingNode::print (int indent) const {
	std::vector< char > depth;
	printImpl (indent, depth, 100.f, true);
}

void TimingNode::printImpl (int indent, const std::vector< char > &depth, float impact, bool last) const {
	enum { NameLengthMax = 40 };
	enum { PadTo = 95 };
	
	// 
	QByteArray prefix (indent, ' ');
	QByteArray name = this->name;
	
	// Shorten name
	if (name.length () > NameLengthMax + 3) {
		name = name.mid (name.length () - NameLengthMax);
		name.prepend ("...");
	}
	
	// Write space indention
	printf ("%s", prefix.constData ());
	
	// Write tree structure
	for (int i = 1; i < depth.size (); i++) {
		if (depth.at (i) == ' ') printf ("  ");
		else printf ("\xE2\x94\x82 "); // |
	}
	
	if (last) {
		printf ("\xE2\x94\x94"); // |_
	} else if (this->parent) {
		printf ("\xE2\x94\x9C"); // |-
	}
	
	// Write name
	int padLen = PadTo - ((depth.size () + 1) * 2 + 5 + name.length ());
	if (impact > 99.9f) padLen--; // Account for "100%"
	QByteArray pad (padLen, ' ');
	
	const char *rawName = name.constData ();
	const char *rawPad = pad.constData ();
	printf ("\xE2\x94\x80%02.0f%% %s%s[%3ims %3ims]\n", impact, rawName, rawPad,
	        this->time / 1000, this->totalTime / 1000);
	
	// Root?
	if (!this->parent) {
		indent += 2;
	}
	
	// Print children
	auto newDepth = depth;
	newDepth.push_back (last ? ' ' : '|');
	
	float childTime = std::max (1LL, this->totalTime - this->time);
	for (int i = 0, count = this->children.length (); i < count; i++) {
		TimingNode *c = this->children.at (i);
		float localImpact = float (c->totalTime) / childTime * 100.0f;
		c->printImpl (indent, newDepth, localImpact, (i + 1 >= count));
	}
	
}

void TimingNode::stop () {
	this->totalTime = nowUsec () - this->initTime;
}

static bool timeComp (TimingNode *left, TimingNode *right) {
	return (left->totalTime > right->totalTime); // Reversed.
}

void TimingNode::sort () {
	std::sort (this->children.begin (), this->children.end (), timeComp);
	
	for (int i = 0; i < this->children.length (); i++) {
		this->children.at (i)->sort ();
	}
	
}
