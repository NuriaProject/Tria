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
#include <vector>

#include <QStringList>
#include <QString>
#include <QVector>
#include <QDebug>
#include <QFile>
#include <QTime>
#include <QDir>

#include <llvm/Support/CommandLine.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Basic/Version.h>

#include "luagenerator.hpp"
#include "definitions.hpp"
#include "filemapper.hpp"
#include "triaaction.hpp"

// Command-line arguments
namespace {
using namespace llvm;

// Options
cl::list< std::string > argInputFiles (cl::Positional, cl::desc ("<input file(s)>"), cl::value_desc ("file"));
cl::opt< std::string > argCxxOutputFile ("cxx-output", cl::ValueOptional, cl::init ("-"),
					 cl::desc ("C++ output file"), cl::value_desc ("cpp file"));
cl::opt< std::string > argJsonOutputFile ("json-output", cl::ValueOptional, cl::init ("-"),
					  cl::desc ("JSON output file"), cl::value_desc ("json file"));
cl::list< std::string > argLuaGenerators ("lua-generator", cl::desc ("Lua generator script"),
                                          cl::value_desc ("script:outfile[:arguments]"));
cl::opt< bool > argLuaShell ("shell", cl::ValueDisallowed,
                             cl::desc ("Opens a Lua shell on stdin/out in the Lua generator environment"));
cl::opt< bool > argTimes ("times", cl::ValueDisallowed,
                          cl::desc ("Writes the times each pass takes to stdout"));
cl::list< std::string > argIncludeDirs ("I", cl::Prefix, cl::desc ("Additional search path"), cl::value_desc ("path"));
cl::list< std::string > argDefines ("D", cl::Prefix, cl::desc ("#define"), cl::value_desc ("name[=value]"));
cl::list< std::string > argUndefines ("U", cl::Prefix, cl::desc ("#undef"), cl::value_desc ("name"));

// Aliases
cl::alias aliasCxxOutputFile ("o", cl::Prefix, cl::desc ("Alias for -cxx-output"), cl::aliasopt (argCxxOutputFile));
cl::alias aliasJsonOutputFile ("j", cl::Prefix, cl::desc ("Alias for -json-output"), cl::aliasopt (argJsonOutputFile));

}

static void prefixedAppend (std::vector< std::string > &arguments, llvm::cl::list< std::string > &input,
			    const std::string &prefix) {
	for (const std::string &cur : input) {
		arguments.push_back (prefix + cur);
	}
	
}

static void initClangArguments (const char *progName, std::vector< std::string > &arguments) {
	arguments.push_back (progName);
	arguments.push_back ("-x");
	arguments.push_back ("c++");
	arguments.push_back ("-fPIE");
	arguments.push_back ("-DTRIA_RUN");
	arguments.push_back ("-std=c++11");
	arguments.push_back ("-fsyntax-only");
	
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
	prefixedAppend (arguments, argDefines, "-D");
	prefixedAppend (arguments, argUndefines, "-U");
	prefixedAppend (arguments, argIncludeDirs, "-I");
	
}

static void printTimes (int total, const std::vector< std::pair< std::string, int > > &times, Definitions &defs) {
	if (!argTimes) {
		return;
	}
	
	// 
	printf ("Times taken:\n");
	int prev = 0;
	for (const std::pair< std::string, int > &stage : times) {
		int cur = stage.second - prev;
		prev = stage.second;
		
		printf ("  %3ims %4.1f%% %s\n", cur,
		        float (cur) / float (total) * 100.f, stage.first.c_str ());
	}
	
	// 
	printf ("  %3ims  100%% total\n", total);
	
	// 
	if (defs.timing ()) {
		printf ("Verbose parsing times: (Files #included multiple times are not shown)\n");
		defs.timing ()->sort ();
		defs.timing ()->print (1);
	}
	
}

static QVector< GenConf > generatorsFromArguments () {
	QVector< GenConf > generators;
	bool jsonOutput = (argJsonOutputFile.getPosition () > 0);
	bool cxxOutput = (argCxxOutputFile.getPosition () > 0);
	
	// C++ code generator
	if (cxxOutput) {
		QString path = QString::fromStdString (argCxxOutputFile);
		generators.append ({ QStringLiteral(":/lua/nuria.lua"), path, QString () });
	}
	
	// JSON generator
	if (jsonOutput) {
		QString path = QString::fromStdString (argJsonOutputFile);
		generators.append ({ QStringLiteral(":/lua/json.lua"), path, QString () });
	}
	
	// Lua shell
	if (argLuaShell) {
		generators.append ({ QStringLiteral("SHELL"), QStringLiteral("-") , QString () });
	}
	
	// Custom Lua generators
	for (const std::string &config : argLuaGenerators) {
		GenConf genConf;
		if (!LuaGenerator::parseConfig (config, genConf)) {
			exit (4);
		}
		
		generators.append (genConf);
		
	}
	
	// 
	return generators;
}

static std::string addInputFiles (QByteArray &fakeHeader) {
	if (argInputFiles.getNumOccurrences () == 1) {
		return *std::begin (argInputFiles);
	}
	
	// Multiple files
	for (const std::string &cur : argInputFiles) {
		fakeHeader.append ("#include \"");
		fakeHeader.append (cur.c_str (), cur.length ());
		fakeHeader.append ("\"\n");
	}
	
	// Map virtual fake header including those files
	return std::string ("nuriaVirtualHeader.hpp");
}

static QStringList sourceFileList () {
	QStringList list;
	for (const std::string &cur : argInputFiles) {
		list.append (QString::fromStdString (cur));
	}
	
	return list;
}

int main (int argc, const char **argv) {
	std::vector< std::pair< std::string, int > > times;
	std::vector< std::string > arguments;
	QByteArray fakeHeader;
	
	QTime timeTotal;
	timeTotal.start ();
	
	// Parse arguments
	const char *helpTitle = "Tria by the NuriaProject, built on " __DATE__ " " __TIME__;
	llvm::cl::ParseCommandLineOptions (argc, argv, helpTitle);
	initClangArguments (argv[0], arguments);
	std::string inputFile = addInputFiles (fakeHeader);
	arguments.push_back (inputFile);
	
	// 
	QVector< GenConf > generators = generatorsFromArguments ();
	clang::FileManager *fm = new clang::FileManager ({ "." });
	
	// Create tool instance
	Definitions definitions (sourceFileList ());
	TriaAction *triaAction = new TriaAction (&definitions);
	clang::tooling::ToolInvocation tool (arguments, triaAction, fm);
	FileMapper mapper (tool);
	
	// Map shipped built-in headers
	mapper.mapRecursive (QDir (":/headers/"), QStringLiteral("/builtins/"));
	if (!fakeHeader.isEmpty ()) {
		mapper.mapByteArray (fakeHeader, QString::fromStdString (inputFile));
	}
	
	times.emplace_back ("init", timeTotal.elapsed ());
	
	// Run it
	if (!tool.run()) {
		return 1;
	}
	
	// Generate code
	definitions.parsingComplete ();
	times.emplace_back ("parse", timeTotal.elapsed ());
	
	// Run generators
	LuaGenerator luaGenerator (&definitions);
	for (int i = 0; i < generators.length (); i++) {
		const GenConf &conf = generators.at (i);
		if (!luaGenerator.generate (conf)) {
			return 5;
		}
		
		times.emplace_back (conf.luaScript.toStdString (), timeTotal.elapsed ());
	}
	
	// 
	printTimes (timeTotal.elapsed (), times, definitions);
	return 0;
}
