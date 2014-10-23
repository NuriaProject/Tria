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

#ifndef TRIAACTION_HPP
#define TRIAACTION_HPP

#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <QByteArray>
#include <QVector>

class Definitions;

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

class TriaAction : public clang::ASTFrontendAction {
public:
	
	TriaAction (Definitions *definitions);
	
protected:
	
	virtual clang::ASTConsumer *CreateASTConsumer (clang::CompilerInstance &ci,
						       llvm::StringRef fileName) override;
	
private:
	Definitions *m_definitions;
	
};

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
