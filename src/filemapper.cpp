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
