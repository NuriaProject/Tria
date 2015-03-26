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

#include "compiler.hpp"

#include <QString>

#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CodeGenOptions.h>
#include <clang/Frontend/TextDiagnostic.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <llvm/Option/ArgList.h>
#include <clang/Driver/Tool.h>
#include <llvm/Support/Host.h>
#include <clang/Driver/Job.h>

#include "filemapper.hpp"
#include "triaaction.hpp"

Compiler::Compiler (Definitions *definitions) {
	this->m_diagOpts = new clang::DiagnosticOptions;
	this->m_diagPrinter = new clang::TextDiagnosticPrinter (llvm::errs (), this->m_diagOpts);
	this->m_diag = new clang::DiagnosticsEngine (this->m_diagIds, this->m_diagOpts, this->m_diagPrinter, false);
	this->m_invocation = new clang::CompilerInvocation;
	this->m_action = new TriaAction (definitions);
	this->m_compiler = new clang::CompilerInstance;
	
	this->m_textDiag = new clang::TextDiagnostic (llvm::errs (), this->m_compiler->getLangOpts (),
	                                              this->m_diagOpts);
}

Compiler::~Compiler () {
	delete this->m_compiler;
	delete this->m_action;
//	delete this->m_invocation; // Owned by m_compiler
	delete this->m_diag;
	delete this->m_diagPrinter;
//	delete this->m_diagOpts; // Owned by m_diag
	delete this->m_textDiag;
}

static clang::driver::Driver *createDriver (clang::DiagnosticsEngine *diag, const char *applicationName) {
	using namespace clang::driver;
	
	std::string targetTriple = llvm::sys::getDefaultTargetTriple ();
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 4
	Driver *driver = new Driver (applicationName, targetTriple, "", *diag);
#else
	Driver *driver = new Driver (applicationName, targetTriple, *diag);
#endif
	driver->setTitle ("clang_based_tool");
	driver->setCheckInputsExist (false);
	
	return driver;
}

const llvm::opt::ArgStringList *Compiler::getCC1Arguments () const {
	using namespace clang::driver;
	
	const JobList &jobs = this->m_compilation->getJobs ();
	
	// TODO: Could we use jobs for proper multi-input file support?
	if (jobs.size () != 1 || !llvm::isa< Command > (*jobs.begin())) {
		llvm::SmallString< 256 > error_msg;
		llvm::raw_svector_ostream error_stream (error_msg);
		jobs.Print (error_stream, "; ", true);
		this->m_diag->Report (clang::diag::err_fe_expected_compiler_job) << error_stream.str ();
		return nullptr;
	}
	
	// The invoked job must be 'clang'
#if CLANG_VERSION_MINOR < 6
	const Command &cmd = *llvm::cast< Command > (*jobs.begin());
#else
	Command &cmd = llvm::cast< Command > (*jobs.begin());
#endif
	if (llvm::StringRef (cmd.getCreator ().getName ()) != "clang") {
		this->m_diag->Report (clang::diag::err_fe_expected_clang_command);
		return nullptr;
	}
	
	return &cmd.getArguments ();
}

bool Compiler::prepare (FileMapper *fileMapper, const std::vector< std::string > &arguments) {
	std::vector< const char * > argv;
	for (int i = 0, count = arguments.size (); i < count; i++) {
		argv.push_back (arguments[i].c_str ());
	}
	
	// Create instances
	const char *applicationName = argv[0];
	this->m_driver = createDriver (this->m_diag, applicationName);
	this->m_compilation = this->m_driver->BuildCompilation (llvm::makeArrayRef (argv));
	this->m_arguments = arguments;
	
	const llvm::opt::ArgStringList *cc1Args = getCC1Arguments ();
	if (!cc1Args) return false;
	
	// Create compiler instance
	clang::CompilerInvocation::CreateFromArgs (*this->m_invocation, cc1Args->data () + 1,
	                                           cc1Args->data () + cc1Args->size(), *this->m_diag);
	this->m_invocation->getFrontendOpts ().DisableFree = true;
	this->m_invocation->getCodeGenOpts ().DisableFree = true;
	
	// Map files
	fileMapper->applyMapping (this);
	return true;
}

bool Compiler::run () {
	clang::FileManager *fm = new clang::FileManager ({ "." });
	this->m_compiler->setInvocation (this->m_invocation);
	this->m_compiler->setFileManager (fm);
	this->m_compiler->createDiagnostics (this->m_diagPrinter, false);
	this->m_compiler->createSourceManager (*fm);
	
	bool success = this->m_compiler->ExecuteAction (*this->m_action);
	
	fm->clearStatCaches ();
	return success;
}

clang::DiagnosticOptions *Compiler::diagOpts () const {
	return this->m_diagOpts;
}

clang::TextDiagnosticPrinter *Compiler::diagPrinter () const {
	return this->m_diagPrinter;
}

clang::TextDiagnostic *Compiler::textDiag () const {
	return this->m_textDiag;
}

clang::DiagnosticsEngine *Compiler::diag () const {
	return this->m_diag;
}

clang::driver::Driver *Compiler::driver () const {
	return this->m_driver;
}

clang::driver::Compilation *Compiler::compilation () const {
	return this->m_compilation;
}

clang::CompilerInvocation *Compiler::invocation () const {
	return this->m_invocation;
}

clang::CompilerInstance *Compiler::compiler () const {
	return this->m_compiler;
}
