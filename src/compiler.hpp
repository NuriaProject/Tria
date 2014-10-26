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
