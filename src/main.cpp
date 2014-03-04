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

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Tool.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Lex/LexDiagnostic.h>

#include <clang/Driver/Job.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <llvm/Support/Host.h>

#include "triaastconsumer.hpp"
#include "nuriagenerator.hpp"
#include "jsongenerator.hpp"
#include "definitions.hpp"

enum class OutputGenerators {
	DefaultGenerator = 0x0,
	CPlusPlusGenerator = 0x1,
	JsonGenerator = 0x2
};

struct Options {
	int generators = int (OutputGenerators::DefaultGenerator);
	bool jsonAppend = false;
	bool preprocessOnly = false;
	QString sourceFile;
	QString cxxOutput = QStringLiteral ("-");
	QString jsonOutput = QStringLiteral ("-");
};

static void showVersion () {
	::printf ("Tria by the NuriaProject, built on " __DATE__ " " __TIME__ "\n");
}

static void showHelp () {
	showVersion ();
	::printf ("Usage: tria [options] <header file> -- <include searchpaths>\n"
		  "  -o<file>             Write output to <file> (Defaults to stdout)\n"
		  "  -j<file>             Write JSON output to <file>\n"
		  "  -a                   Insert class definitions into JSON file, keeping other data\n"
		  "  -I<dir>              Add <dir> to the search path for #includes\n"
		  "  -E                   Only runs the pre-processor of clang\n"
		  "  -D<macro>[=<value>]  Defines <macro> with a optional <value>\n"
		  "  -U<macro>            Undefine <macro>\n"
		  "  -- <path(s)>         Everything after -- is treated like a -I<Path>\n"
		  "                       This option only exists to work-around QMakes inabilities.\n"
		  "Note: When only -o or -j is given, only C++ code or JSON data will be generated.\n"
		  "      Setting both will generate both types at once. When neither are given, the\n"
		  "      default behaviour is to generate C++ code and output it on stdout.\n");
}

static void parseArguments (int argc, const char **argv, std::vector< std::string > &clangArgs,
			    Options &options) {
	bool nextArgNotInput = false;
	bool hasInput = false;
	
	for (int i = 1; i < argc; i++) {
		const char *cur = argv[i];
		
		if (*cur == '-') {
			nextArgNotInput = false;
			switch (cur[1]) {
			case 'h':
			case '?':
				showHelp ();
				::exit (0);
			case 'v':
				showVersion ();
				::exit (0);
			case 'a':
				options.jsonAppend = true;
				break;
			case 'o':
				options.generators |= int (OutputGenerators::CPlusPlusGenerator);
				
				if (cur[2]) {
					options.cxxOutput = cur + 2;
				} else if (i + 1 < argc) {
					i++;
					options.cxxOutput = argv[i];
				}
				
				break;
			case 'j':
				options.generators |= int (OutputGenerators::JsonGenerator);
				
				if (cur[2]) {
					options.cxxOutput = cur + 2;
				} else if (i + 1 < argc) {
					i++;
					options.jsonOutput = argv[i];
				}
				
				break;
			case 'E':
				options.preprocessOnly = true;
				break;
			case 'f':
			case 'D':
			case 'I':
			case 'U':
			case 'W':
			case 'X':
				clangArgs.push_back (cur);
				if (!cur[2]) {
					nextArgNotInput = true;
				}
				
				break;
			case '-':
				for (i++; i < argc; i++) {
					clangArgs.push_back (std::string ("-I") + argv[i]);
				}
				
				break;
			default:
				std::cerr << "tria: Invalid argument '" << cur << "'" << std::endl;
				::exit (1);
			}
			
		} else {
			if (!nextArgNotInput) {
				if (hasInput) {
					std::cerr << "error: Only one input file may be specified" << std::endl;
					::exit (2);
				}
				
				hasInput = true;
			}
			
			clangArgs.push_back (cur);
			options.sourceFile = cur;
		}
		
	}
	
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
	ci.getPreprocessor().SetSuppressIncludeNotFoundError (true);
	ci.getLangOpts().DelayedTemplateParsing = true;
	
	// Enable everything for code compatibility
	ci.getLangOpts().MicrosoftExt = true;
	ci.getLangOpts().DollarIdents = true;
#if CLANG_VERSION_MAJOR != 3 || CLANG_VERSION_MINOR > 2
	ci.getLangOpts().CPlusPlus11 = true;
#else
	CI.getLangOpts().CPlusPlus0x = true;
#endif
	ci.getLangOpts().CPlusPlus1y = true;
	ci.getLangOpts().GNUMode = true;
	
	return new TriaASTConsumer (ci, fileName, this->m_definitions);
}

static clang::FrontendAction *createAction (bool preprocessOnly, Definitions *generator = nullptr) {
	if (preprocessOnly) {
		return new clang::PrintPreprocessedAction;
	}
	
	// 
	return new TriaAction (generator);
}

static bool openStdoutOrFile (QFile &device, const QString &path) {
	if (path == QLatin1String ("-")) {
		device.open (stdout, QIODevice::WriteOnly);
	} else {
		device.setFileName (path);
		if (!device.open (QIODevice::WriteOnly)) {
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

int main (int argc, const char **argv) {
	std::vector< std::string > arguments;
	
	// Prepare Clang arguments
	arguments.push_back (argv[0]);
	arguments.push_back ("-x");
	arguments.push_back ("c++");
	arguments.push_back ("-fPIE");
	arguments.push_back ("-DTRIA_RUN");
	arguments.push_back ("-std=c++11");
	
	// Parse arguments
	Options options;
	parseArguments (argc, argv, arguments, options);
	
	// Always include built-in headers
	arguments.push_back ("-I/builtins");
	
	// 
	clang::FileManager *fm = new clang::FileManager ({"."});
	if (options.preprocessOnly) {
		arguments.push_back ("-P");
	} else {
		// Don't compile anything, Clang itself will only validate the syntax.
		arguments.push_back ("-fsyntax-only");
	}
	
	// Create tool instance
	Definitions definitions (options.sourceFile);
	clang::tooling::ToolInvocation tool (arguments, createAction (options.preprocessOnly, &definitions), fm);
	
	// Map shipped built-in headers
	auto fileBuffers = mapVirtualFiles (tool, QDir (":/headers/"), QStringLiteral ("/builtins/"));
	
	// Run it
	if (!tool.run()) {
		return 1;
	}
	
	// Generate code. C++ code generator
	if (options.generators == int (OutputGenerators::DefaultGenerator) ||
	    options.generators & int (OutputGenerators::CPlusPlusGenerator)) {
		NuriaGenerator generator (&definitions);
		QFile device;
		
		if (!openStdoutOrFile (device, options.cxxOutput) ||
		    !generator.generate (&device)) {
			return 2;
		}
		
	}
	
	// JSON generator
	if (options.generators & int (OutputGenerators::JsonGenerator)) {
		JsonGenerator generator (&definitions);
		QFile device;
		
		if (!openStdoutOrFile (device, options.jsonOutput) ||
		    !generator.generate (&device, options.jsonAppend)) {
			return 3;
		}
		
	}
	
	// Avoid "fileHandles is unused" warning
	fileBuffers.clear ();
	
	// 
	return 0;
}
