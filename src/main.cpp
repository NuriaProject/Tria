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

#include <iostream>
#include <cstdlib>
#include <cstdio>

#include <QStringList>
#include <QString>
#include <QVector>
#include <QDebug>
#include <QFile>
#include <QDir>

#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Driver/Compilation.h>
#include <llvm/Support/CommandLine.h>
#include <clang/Lex/LexDiagnostic.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/Version.h>
#include <clang/Driver/Driver.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Driver/Tool.h>
#include <llvm/Support/Host.h>
#include <clang/Driver/Job.h>

#include "triaastconsumer.hpp"
#include "nuriagenerator.hpp"
#include "jsongenerator.hpp"
#include "definitions.hpp"

// Command-line arguments
namespace {
using namespace llvm;

// Options
cl::opt< std::string > argInputFile (cl::Positional, cl::desc ("<input file>"), cl::value_desc ("file"));
cl::list< std::string > argSearchPaths (cl::ConsumeAfter, cl::desc ("<additional search paths (For moc compat)>"));
cl::opt< std::string > argCxxOutputFile ("cxx-output", cl::ValueOptional, cl::init ("-"),
					 cl::desc ("C++ output file"), cl::value_desc ("cpp file"));
cl::opt< std::string > argJsonOutputFile ("json-output", cl::ValueOptional, cl::init ("-"),
					  cl::desc ("JSON output file"), cl::value_desc ("json file"));
cl::opt< bool > argJsonInsert ("insert-json", cl::ValueDisallowed,
			       cl::desc ("Insert JSON output data into the json output file rather than replacing it"));
cl::opt< bool > argInspectAll ("introspect-all", cl::ValueDisallowed,
			       cl::desc ("All types will be introspected as if they had a NURIA_INTROSPECT annotation. "
					 "Types with NURIA_SKIP will be ignored."));
cl::list< std::string > argInspectBases ("introspect-inheriting", cl::CommaSeparated,
					 cl::desc ("Introspect all types which inherit <type>."),
					 cl::value_desc ("type1,typeN,..."));
cl::list< std::string > argIncludeDirs ("I", cl::Prefix, cl::desc ("Additional search path"), cl::value_desc ("path"));
cl::list< std::string > argDefines ("D", cl::Prefix, cl::desc ("#define"), cl::value_desc ("name[=value]"));
cl::list< std::string > argUndefines ("U", cl::Prefix, cl::desc ("#undef"), cl::value_desc ("name"));

// Aliases
cl::alias aliasCxxOutputFile ("o", cl::Prefix, cl::desc ("Alias for -cxx-output"), cl::aliasopt (argCxxOutputFile));
cl::alias aliasJsonOutputFile ("j", cl::Prefix, cl::desc ("Alias for -json-output"), cl::aliasopt (argJsonOutputFile));
cl::alias aliasJsonInsert ("a", cl::desc ("Alias for -insert-json"), cl::aliasopt (argJsonInsert));
cl::alias aliasInspectBases ("B", cl::Prefix, cl::desc ("Alias for -introspect-inheriting"),
			     cl::aliasopt (argInspectBases));

}

// 
class TriaAction : public clang::ASTFrontendAction {
public:
	
	TriaAction (Definitions *definitions)
		: m_definitions (definitions)
	{
	}
	
protected:
	
	virtual clang::ASTConsumer *CreateASTConsumer (clang::CompilerInstance &ci,
						       llvm::StringRef fileName) override;
	
private:
	Definitions *m_definitions;
	
};

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

static bool openStdoutOrFile (QFile &device, const QString &path, QIODevice::OpenMode openMode) {
	if (path.isEmpty () || path == "-") {
		device.open (stdout, openMode);
	} else {
		device.setFileName (path);
		if (!device.open (openMode)) {
			qCritical("Failed to open file %s: %s",
				  qPrintable(path),
				  qPrintable(device.errorString ()));
			return false;
		}
		
	}
	
	return true;
	
}

static QVector< QByteArray > mapVirtualFiles (clang::tooling::ToolInvocation &tool,
                                              const QDir &directory, const QString &prefix) {
	QVector< QByteArray > buffers;
	
	QStringList builtinHeaders = directory.entryList (QDir::Files);
	QStringList dirs = directory.entryList (QDir::Dirs);
	
	for (const QString &cur : dirs) {
		QDir d = directory;
		d.cd (cur);
		buffers += mapVirtualFiles (tool, d, prefix + cur + "/");
	}
	
	for (QString cur : builtinHeaders) {
		QFile file (directory.filePath (cur));
		file.open (QIODevice::ReadOnly);
		
		QByteArray name = prefix.toLatin1 () + cur.toLatin1 ();
		const char *rawName = name.constData ();
		
		QByteArray content = file.readAll ();
		const char *rawContent = content.constData ();
		
		buffers << name << content;
		
		llvm::StringRef nameRef (rawName, size_t (name.length ()));
		llvm::StringRef dataRef (rawContent, size_t (content.length ()));
		tool.mapVirtualFile (nameRef, dataRef);
	}
	
	return buffers;
}

static void prefixedAppend (std::vector< std::string > &arguments, llvm::cl::list< std::string > &input,
			    const std::string &prefix) {
	for (const std::string &cur : input) {
		arguments.push_back (prefix + cur);
	}
	
}

static void passThroughClangOptions (std::vector< std::string > &arguments) {
	prefixedAppend (arguments, argDefines, "-D");
	prefixedAppend (arguments, argUndefines, "-U");
	prefixedAppend (arguments, argIncludeDirs, "-I");
	prefixedAppend (arguments, argSearchPaths, "-I");
	
}

int main (int argc, const char **argv) {
	std::vector< std::string > arguments;
	
	// Prepare Clang arguments
	arguments.push_back (argv[0]);
	arguments.push_back ("-x");
	arguments.push_back ("c++");
	arguments.push_back ("-fPIE");
	arguments.push_back ("-DTRIA_RUN");
	arguments.push_back ("-std=c++11");
	arguments.push_back ("-fsyntax-only");
	
	// Parse arguments
	const char *helpTitle = "Tria by the NuriaProject, built on " __DATE__ " " __TIME__;
	llvm::cl::ParseCommandLineOptions (argc, argv, helpTitle);
	
	// Inject absolute path to the clang headers on linux.
	// Should we bundle those too? Would add another 1.5MiB ..
#ifdef Q_OS_LINUX
	arguments.push_back ("-isystem");
	arguments.push_back (LLVM_PREFIX "/lib/clang/" CLANG_VERSION_STRING "/include");
#else
	// For other OSes
	arguments.push_back ("-I/builtins");
#endif
	
	// Append user-supplied arguments
	passThroughClangOptions (arguments);
	arguments.push_back (argInputFile);
	
	// 
	clang::FileManager *fm = new clang::FileManager ({ "." });
	
	// Create tool instance
	Definitions definitions (QString::fromStdString (argInputFile));
	TriaAction *triaAction = new TriaAction (&definitions);
	clang::tooling::ToolInvocation tool (arguments, triaAction, fm);
	
	// Map shipped built-in headers
	auto fileBuffers = mapVirtualFiles (tool, QDir (":/headers/"), QStringLiteral ("/builtins/"));
	
	// Run it
	if (!tool.run()) {
		return 1;
	}
	
	// Generate code
	bool jsonOutput = (argJsonOutputFile.getPosition () > 0);
	bool cxxOutput = (argCxxOutputFile.getPosition () > 0);
	
	// C++ code generator
	if (!jsonOutput || (cxxOutput && jsonOutput)) {
		QString path = QString::fromStdString (argCxxOutputFile);
		NuriaGenerator generator (&definitions);
		QFile device;
		
		if (!openStdoutOrFile (device, path, QIODevice::WriteOnly) ||
		    !generator.generate (&device)) {
			return 2;
		}
		
	}
	
	// JSON generator
	if (jsonOutput) {
		QString path = QString::fromStdString (argJsonOutputFile);
		JsonGenerator generator (&definitions);
		QFile device;
		
		if (!openStdoutOrFile (device, path, QIODevice::ReadWrite) ||
		    !generator.generate (&device, argJsonInsert)) {
			return 3;
		}
		
	}
	
	// Avoid "fileHandles is unused" warning
	fileBuffers.clear ();
	
	// 
	return 0;
}
