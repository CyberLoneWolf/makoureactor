/****************************************************************************
 ** Makou Reactor Final Fantasy VII Field Script Editor
 ** Copyright (C) 2009-2012 Arzel Jérôme <myst6re@gmail.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "PCFieldFile.h"

PCFieldFile::PCFieldFile() :
	LzsSectionFile()
{
}

bool PCFieldFile::openHeader()
{
	if (!io()->seek(6)) {
		qWarning() << "PCFieldFile::openHeader cannot seek" << io()->errorString();
		return false;
	}

	if (io()->read(reinterpret_cast<char *>(_sectionPositions), sectionCount() * 4) != sectionCount() * 4) {
		qWarning() << "PCFieldFile::openHeader file too short:" << io()->errorString();
		return false;
	}

	if (LzsSectionFile::sectionPos(0) != 42) {
		qWarning() << "PCFieldFile::openHeader first position must be 42:" << _sectionPositions[0];
		return false;
	}

	return true;
}

int PCFieldFile::setSectionData(int pos, int oldSize,
								const QByteArray &section,
								QByteArray &out)
{
	int newSize = section.size();

	out.replace(pos, 4, reinterpret_cast<char *>(&newSize), 4);
	out.replace(pos + 4, oldSize, section);

	return newSize - oldSize;
}

bool PCFieldFile::writePositions(QByteArray &data)
{
	const int size = PC_FIELD_FILE_SECTION_COUNT * 4;

	data.replace(6, size, reinterpret_cast<char *>(_sectionPositions), size);

	return true;
}

quint32 PCFieldFile::sectionPos(quint8 id) const
{
	return LzsSectionFile::sectionPos(id) + 4;
}

quint32 PCFieldFile::sectionSize(quint8 id, bool &eof) const
{
	quint32 size = LzsSectionFile::sectionSize(id, eof);
	if (!eof) {
		return size - 4;
	}
	return size;
}
