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

#include "filemapper.hpp"

#include <clang/Frontend/CompilerInvocation.h>
#include <QString>

#include <llvm/Support/MemoryBuffer.h>
#include "compiler.hpp"

FileMapper::FileMapper () {
	
}

void FileMapper::mapRecursive (const QDir &directory, const QString &prefix) {
	QStringList files = directory.entryList (QDir::Files);
	QStringList dirs = directory.entryList (QDir::Dirs);
	
	for (const QString &cur : dirs) {
		QDir d = directory;
		d.cd (cur);
		mapRecursive (d, prefix + cur + QLatin1String("/"));
	}
	
	for (const QString &cur : files) {
		mapFile (directory.filePath (cur), prefix + cur);
	}
	
}

void FileMapper::mapFile (const QString &path, const QString &target) {
	QFile file (path);
	file.open (QIODevice::ReadOnly);
	
	mapByteArray (file.readAll (), target);
}

void FileMapper::mapByteArray (const QByteArray &data, const QString &target) {
	this->m_files.insert (target.toLatin1 (), data);
}

void FileMapper::applyMapping (Compiler *compiler) {
	clang::CompilerInvocation *invocation = compiler->invocation ();
	
	for (auto it = this->m_files.begin (), end = this->m_files.end (); it != end; ++it) {
		llvm::StringRef name (it.key ().constData (), it.key ().length ());
		llvm::StringRef data (it->constData (), it->length ());
		
		llvm::MemoryBuffer *input = llvm::MemoryBuffer::getMemBuffer (data);
		invocation->getPreprocessorOpts ().addRemappedFile (name, input);
	}
	
}
