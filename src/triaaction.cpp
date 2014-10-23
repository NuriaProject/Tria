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

clang::ASTConsumer *TriaAction::CreateASTConsumer (clang::CompilerInstance &ci, llvm::StringRef fileName) {
	ci.getFrontendOpts().SkipFunctionBodies = true;
	ci.getPreprocessor().enableIncrementalProcessing (true);
	ci.getLangOpts().DelayedTemplateParsing = true;
	
	// Enable everything for code compatibility
	ci.getLangOpts().MicrosoftExt = true;
	ci.getLangOpts().DollarIdents = true;
	ci.getLangOpts().CPlusPlus11 = true;
	ci.getLangOpts().CPlusPlus1y = true;
	ci.getLangOpts().GNUMode = true;
	
	if (argVerboseTimes) {
		PreprocessorHooks *hook = new PreprocessorHooks (ci);
		hook->timing ()->name = sourceFileName (this->m_definitions->sourceFiles ());
		this->m_definitions->setTimingNode (hook->timing ());
		ci.getPreprocessor ().addPPCallbacks (hook);
	}
	
	// 
	QStringList whichInherit;
	for (const std::string &cur : argInspectBases) {
		whichInherit.append (QString::fromStdString (cur));
	}
	
	// 
	return new TriaASTConsumer (ci, fileName, whichInherit, argInspectAll, this->m_definitions);
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
