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
#include <clang/Basic/Version.h>

#include "triaastconsumer.hpp"
#include "definitions.hpp"

#include <QString>
#include <QDebug>

namespace {
using namespace llvm;

cl::opt< bool > argInspectAll ("introspect-all", cl::ValueDisallowed,
			       cl::desc ("All types will be introspected as if they had a NURIA_INTROSPECT annotation. "
					 "Types with NURIA_SKIP will be ignored."));
cl::list< std::string > argInspectBases ("introspect-inheriting", cl::CommaSeparated,
					 cl::desc ("Introspect all types which inherit <type>."),
					 cl::value_desc ("type1,typeN,..."));

// Aliases
cl::alias aliasInspectBases ("B", cl::Prefix, cl::desc ("Alias for -introspect-inheriting"),
			     cl::aliasopt (argInspectBases));

}

TriaAction::TriaAction (Definitions *definitions)
        : m_definitions (definitions)
{ }


clang::ASTConsumer *TriaAction::CreateASTConsumer (clang::CompilerInstance &ci, llvm::StringRef fileName) {
	
	ci.getFrontendOpts().SkipFunctionBodies = true;
	ci.getPreprocessor().enableIncrementalProcessing (true);
	ci.getLangOpts().DelayedTemplateParsing = true;
	
	// Enable everything for code compatibility
	ci.getLangOpts().MicrosoftExt = true;
	ci.getLangOpts().DollarIdents = true;
#if CLANG_VERSION_MAJOR != 3 || CLANG_VERSION_MINOR > 2
	ci.getLangOpts().CPlusPlus11 = true;
#else
	ci.getLangOpts().CPlusPlus0x = true;
#endif
	ci.getLangOpts().CPlusPlus1y = true;
	ci.getLangOpts().GNUMode = true;
	
	// 
	QStringList whichInherit;
	for (const std::string &cur : argInspectBases) {
		whichInherit.append (QString::fromStdString (cur));
	}
	
	// 
	return new TriaASTConsumer (ci, fileName, whichInherit, argInspectAll, this->m_definitions);
}
