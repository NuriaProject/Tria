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

#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <clang/Basic/DiagnosticOptions.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <llvm/Option/Option.h>
#undef bool

namespace clang {
class TextDiagnosticPrinter;
class CompilerInvocation;
class CompilerInstance;
class TextDiagnostic;

namespace driver {
class Compilation;
class Driver;
}

}

class Definitions;
class FileMapper;
class TriaAction;
class Compiler {
public:
	
	Compiler (Definitions *definitions);
	
	~Compiler ();
	
	bool prepare (FileMapper *fileMapper, const std::vector< std::string > &arguments);
	bool run ();
	
	// 
	clang::DiagnosticOptions *diagOpts () const;
	clang::TextDiagnosticPrinter *diagPrinter () const;
	clang::TextDiagnostic *textDiag () const;
	clang::DiagnosticsEngine *diag () const;
	clang::driver::Driver *driver () const;
	clang::driver::Compilation *compilation () const;
	clang::CompilerInvocation *invocation () const;
	clang::CompilerInstance *compiler () const;
	
private:
	
	const llvm::opt::ArgStringList *getCC1Arguments () const;
	
	std::vector< std::string > m_arguments;
	clang::DiagnosticOptions *m_diagOpts;
	clang::TextDiagnosticPrinter *m_diagPrinter;
	clang::TextDiagnostic *m_textDiag = nullptr;
	clang::DiagnosticsEngine *m_diag;
	llvm::IntrusiveRefCntPtr< clang::DiagnosticIDs > m_diagIds;
	clang::driver::Driver *m_driver = nullptr;
	clang::driver::Compilation *m_compilation = nullptr;
	clang::CompilerInvocation *m_invocation;
	clang::CompilerInstance *m_compiler;
	TriaAction *m_action;
	
};

#endif // COMPILER_HPP
