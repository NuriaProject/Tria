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

#ifndef TRIAACTION_HPP
#define TRIAACTION_HPP

#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Basic/Version.h>
#include <QByteArray>
#include <QVector>

class Definitions;

// 
class TimingNode {
public:
        enum { Skipped = -1 };
        
        // 
        TimingNode (const QByteArray &n, TimingNode *p = nullptr);
        ~TimingNode ();
	
	void print (int indent = 0) const;
        void stop ();
	void sort ();
	
        // 
        qint64 time = 0;
        qint64 startTime = 0;
        qint64 initTime = 0;
        qint64 totalTime = Skipped;
        
        QByteArray name;
        
        QVector< TimingNode * > children;
        TimingNode *parent;
	
private:
	
	void printImpl (int indent, const std::vector< char > &depth, float impact, bool last) const;
};

// 
class TriaAction : public clang::ASTFrontendAction {
public:
	
	TriaAction (Definitions *definitions);
	
protected:
	
#if CLANG_VERSION_MINOR < 6
	typedef clang::ASTConsumer *CreateAstConsumerResultType;
#else
	typedef std::unique_ptr< clang::ASTConsumer > CreateAstConsumerResultType;
#endif
	
	virtual CreateAstConsumerResultType CreateASTConsumer (clang::CompilerInstance &ci,
	                                                       llvm::StringRef fileName) override;
	
private:
	Definitions *m_definitions;
	
};

// 
class PreprocessorHooks : public clang::PPCallbacks {
public:
	PreprocessorHooks (clang::CompilerInstance &ci);
	~PreprocessorHooks () override;
	
	TimingNode *timing () const;
	
	void FileChanged (clang::SourceLocation loc, FileChangeReason reason,
	                  clang::SrcMgr::CharacteristicKind fileType, clang::FileID prevFID) override;
	
	void FileSkipped (const clang::FileEntry &, const clang::Token &, clang::SrcMgr::CharacteristicKind);
	
	void InclusionDirective (clang::SourceLocation, const clang::Token &, clang::StringRef, bool,
	                         clang::CharSourceRange, const clang::FileEntry *file, clang::StringRef,
	                         clang::StringRef, const clang::Module *) override;
	
public:
	void goUp (bool skipped);
	
	clang::CompilerInstance &m_compiler;
	
	// 
	QByteArray m_curFile;
	TimingNode *m_root;
	TimingNode *m_current;
	
};

#endif // TRIAACTION_HPP
