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

#include <QString>

FileMapper::FileMapper(clang::tooling::ToolInvocation &tool)
        : m_tool (tool)
{
	
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
	QByteArray name = target.toLatin1 ();
	
	const char *rawContent = data.constData ();
	const char *rawName = name.constData ();
	
	this->m_buffers.append (name);
	this->m_buffers.append (data);
	
	llvm::StringRef nameRef (rawName, size_t (name.length ()));
	llvm::StringRef dataRef (rawContent, size_t (data.length ()));
	this->m_tool.mapVirtualFile (nameRef, dataRef);
	
}
