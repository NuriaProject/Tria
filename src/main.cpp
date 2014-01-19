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
#include <QFile>

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
#include "generator.hpp"

struct Options {
  bool preprocessOnly = false;
  QString sourceFile;
  QString output = QStringLiteral ("-");
};

static void showVersion () {
	::printf ("Tria by the NuriaProject\n");
}

static void showHelp () {
	showVersion ();
	::printf ("Usage: tria [options] <header file>\n"
		  "  -o<file>             Write output to <file> (Defaults to stdout)\n"
		  "  -I<dir>              Add <dir> to the search path for #includes\n"
		  "  -E                   Only runs the pre-processor of clang\n"
		  "  -D<macro>[=<value>]  Defines <macro> with a optional <value>\n"
		  "  -U<macro>            Undefine <macro>\n");
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
			case 'o':
				if (cur[2]) {
					options.output = cur + 2;
				} else if (i + 1 < argc) {
					i++;
					options.output = argv[i];
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
	
	TriaAction (Generator *generator)
		: m_generator (generator)
	{
	}
	
        virtual bool hasCodeCompletionSupport () const
        { return true; }
	
protected:
	
	virtual clang::ASTConsumer *CreateASTConsumer (clang::CompilerInstance &ci,
						       llvm::StringRef fileName) override;
	
private:
	Generator *m_generator;
	
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
	
	return new TriaASTConsumer (ci, fileName, this->m_generator);
}

static clang::FrontendAction *createAction (bool preprocessOnly, Generator *generator = nullptr) {
	if (preprocessOnly) {
		return new clang::PrintPreprocessedAction;
	}
	
	// 
	return new TriaAction (generator);
}

static bool generateCode (Generator *generator, const QString &output) {
	QFile device;
	
	if (output == QLatin1String ("-")) {
		device.open (stdout, QIODevice::WriteOnly);
	} else {
		device.setFileName (output);
		if (!device.open (QIODevice::WriteOnly)) {
			qCritical("Failed to open file %s: %s",
				  qPrintable(output),
				  qPrintable(device.errorString ()));
			return false;
		}
		
	}
	
	// 
	return generator->generate (&device);
	
}

int main (int argc, const char **argv) {
	std::vector< std::string > arguments;
	
	// Prepare Clang arguments
	arguments.push_back (argv[0]);
	arguments.push_back ("-x");
	arguments.push_back ("c++");
	arguments.push_back ("-fPIE");
	arguments.push_back ("-DTRIA_RUN");
	
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
	
	// Create tool and run it.
	Generator generator (options.sourceFile);
	clang::tooling::ToolInvocation tool (arguments, createAction (options.preprocessOnly, &generator), fm);
	
	if (!tool.run()) {
		return 1;
	}
	
	// Generate code
	if (!generateCode (&generator, options.output)) {
		return 2;
	}
	
	// 
	return 0;
}
