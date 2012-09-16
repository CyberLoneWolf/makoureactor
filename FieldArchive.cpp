/****************************************************************************
 ** Makou Reactor Final Fantasy VII Field Script Editor
 ** Copyright (C) 2009-2012 Arzel J�r�me <myst6re@gmail.com>
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
#include "FieldArchive.h"
#include "FieldPS.h"
#include "FieldPC.h"
#include "GZIP.h"
#include "LZS.h"
#include "Data.h"

FieldArchive::FieldArchive()
	: fic(NULL), dir(NULL), iso(NULL), isDat(false)
{
}

FieldArchive::FieldArchive(const QString &path, bool isDirectory)
	: fic(NULL), dir(NULL), iso(NULL), isDat(false)
{
	if(isDirectory) {
		dir = new QDir(path);
	}
	else {
		QString ext = path.mid(path.lastIndexOf('.')+1).toLower();

		if(ext == "iso" || ext == "bin") {
			iso = new IsoArchive(path);
			if(!iso->open(QIODevice::ReadOnly)) {
				delete iso;
				iso = NULL;
			}
		}
		else {
			isDat = ext == "dat";
			fic = new QLockedFile(path);
		}
	}
	//	fileWatcher.addPath(path);
	//	connect(&fileWatcher, SIGNAL(fileChanged(QString)), this, SIGNAL(fileChanged(QString)));
	//	connect(&fileWatcher, SIGNAL(directoryChanged(QString)), this, SIGNAL(directoryChanged(QString)));
}

FieldArchive::~FieldArchive()
{
	foreach(Field *field, fileList)	delete field;
	foreach(TutFile *tut, tuts)		delete tut;
	if(fic!=NULL)	delete fic;
	if(dir!=NULL)	delete dir;
	if(iso!=NULL)	delete iso;
}

QString FieldArchive::path() const
{
	if(dir!=NULL)
		return dir->path();
	if(fic!=NULL)
		return fic->fileName();
	if(iso!=NULL)
		return iso->fileName();

	return QString();
}

QString FieldArchive::name() const
{
	if(fic!=NULL)
		return fic->fileName().mid(fic->fileName().lastIndexOf("/")+1);
	if(iso!=NULL)
		return iso->fileName().mid(iso->fileName().lastIndexOf("/")+1);

	return QString();
}

QString FieldArchive::chemin() const
{
	if(dir!=NULL)
		return dir->path() + "/";
	if(fic!=NULL)
		return fic->fileName().left(fic->fileName().lastIndexOf("/")+1);
	if(iso!=NULL)
		return iso->fileName().left(iso->fileName().lastIndexOf("/")+1);

	return QString();
}

int FieldArchive::size() const
{
	return fileList.size();
}

bool FieldArchive::isDatFile() const
{
	return fic != NULL && isDat;
}

bool FieldArchive::isDirectory() const
{
	return dir != NULL;
}

bool FieldArchive::isLgp() const
{
	return fic != NULL && !isDat;
}

bool FieldArchive::isIso() const
{
	return iso != NULL;
}

bool FieldArchive::openField(Field *field, bool dontOptimize)
{
	if(!field->isOpen()) {
		return field->open(dontOptimize);
	}
	return true;
}

Field *FieldArchive::field(quint32 id, bool open, bool dontOptimize)
{
	Field *field = fileList.value(id, NULL);
	if(field!=NULL && open && !openField(field, dontOptimize)) {
		return NULL;
	}
	if(field!=NULL) {
		Data::currentTextes = field->scriptsAndTexts()->texts();
	}
	return field;
}

Field *FieldArchive::field(const QString &name, bool open)
{
	int index;
	if((index = findField(name)) != -1) {
		return field(index, open);
	}
	return NULL;
}

QByteArray FieldArchive::fieldDataCache;
QByteArray FieldArchive::mimDataCache;
QByteArray FieldArchive::modelDataCache;
Field *FieldArchive::fieldCache=0;
Field *FieldArchive::mimCache=0;
Field *FieldArchive::modelCache=0;

QByteArray FieldArchive::getLgpData(int position)
{
	if(fic==NULL)	return QByteArray();
	quint32 fileSize;

	if(!fic->isOpen() && !fic->open(QIODevice::ReadOnly))		return QByteArray();
	if(!fic->seek(position+20))					return QByteArray();
	if(fic->read((char *)&fileSize, 4) != 4)	return QByteArray();

	QByteArray data = fic->read(fileSize);

//	fic->close();

	return data;
}

QByteArray FieldArchive::getFieldData(Field *field, bool unlzs)
{
	quint32 lzsSize;
	QByteArray data;

	// use data from the cache
	if(unlzs && fieldDataIsCached(field)) {
//		qDebug() << "FieldArchive use field data from cache" << field->getName();
		return fieldDataCache;
	}/* else {
		qDebug() << "FieldArchive don't use field data from cache" << field->getName() << unlzs;
	}*/

	if(isDatFile()) {
		if(!fic->isOpen() && !fic->open(QIODevice::ReadOnly))		return QByteArray();
		fic->reset();
		data = fic->readAll();
		fic->close();

		if(data.size() < 4)		return QByteArray();

		memcpy(&lzsSize, data.constData(), 4);

		if((quint32)data.size() != lzsSize+4)	return QByteArray();

		data = unlzs ? LZS::decompress(data.mid(4)) : data;
	} else if(isLgp()) {
		data = getLgpData(((FieldPC *)field)->getPosition());

		if(data.size() < 4)		return QByteArray();

		memcpy(&lzsSize, data.constData(), 4);
		if((quint32)data.size() != lzsSize + 4)				return QByteArray();

		data = unlzs ? LZS::decompress(data.mid(4)) : data;
	} else if(isIso() || isDirectory()) {
		data = getFileData(field->getName().toUpper()+".DAT", unlzs);
	}

	if(unlzs) { // put decompressed data in the cache
		fieldCache = field;
		fieldDataCache = data;
	}
	return data;
}

QByteArray FieldArchive::getMimData(Field *field, bool unlzs)
{
	// use data from the cache
	if(unlzs && mimDataIsCached(field)) {
		return mimDataCache;
	}

	QByteArray data = getFileData(field->getName().toUpper()+".MIM", unlzs);

	if(unlzs) { // put decompressed data in the cache
		mimCache = field;
		mimDataCache = data;
	}
	return data;
}

QByteArray FieldArchive::getModelData(Field *field, bool unlzs)
{
	// use data from the cache
	if(unlzs && modelDataIsCached(field)) {
		return modelDataCache;
	}

	QByteArray data = getFileData(field->getName().toUpper()+".BSX", unlzs);

	if(unlzs) { // put decompressed data in the cache
		modelCache = field;
		modelDataCache = data;
	}
	return data;
}

QByteArray FieldArchive::getFileData(const QString &fileName, bool unlzs)
{
	quint32 lzsSize;
	QByteArray data;

	if(isLgp()) {
		return QByteArray();
	} else if(isIso()) {
		data = iso->file(isoFieldDirectory->file(fileName.toUpper()));

		if(data.size() < 4)		return QByteArray();

		memcpy(&lzsSize, data.constData(), 4);

		if((quint32)data.size() != lzsSize+4)	return QByteArray();

		return unlzs ? LZS::decompress(data.mid(4)) : data;
	} else if(isDatFile() || isDirectory()) {
		QFile f(chemin()+fileName.toUpper());
		if(!f.open(QIODevice::ReadOnly))	return QByteArray();
		data = f.readAll();
		f.close();

		if(data.size() < 4)		return QByteArray();

		memcpy(&lzsSize, data.constData(), 4);

		if((quint32)data.size() != lzsSize+4)	return QByteArray();

		return unlzs ? LZS::decompress(data.mid(4)) : data;
	} else {
		return QByteArray();
	}
}

TutFile *FieldArchive::getTut(const QString &name)
{
	if(!isLgp()) return NULL;

	QMapIterator<QString, int> i(tutPos);
	while(i.hasNext()) {
		i.next();
		if(name.startsWith(i.key(), Qt::CaseInsensitive)) {
			int pos = i.value();
			if(tuts.contains(i.key())) {
				return tuts.value(i.key());
			} else {
				QByteArray data = getLgpData(pos);
				if(!data.isEmpty()) {
					TutFile *tutFile = new TutFile(data, true);
					tuts.insert(i.key(), tutFile);
					return tutFile;
				}
			}
		}
	}

	return NULL;
}

bool FieldArchive::fieldDataIsCached(Field *field) const
{
	return fieldCache && fieldCache == field;
}

bool FieldArchive::mimDataIsCached(Field *field) const
{
	return mimCache && mimCache == field;
}

bool FieldArchive::modelDataIsCached(Field *field) const
{
	return modelCache && modelCache == field;
}

void FieldArchive::clearCachedData()
{
	fieldCache = 0;
	mimCache = 0;
	modelCache = 0;
	fieldDataCache.clear();
	mimDataCache.clear();
	modelDataCache.clear();
}

bool FieldArchive::isAllOpened()
{
	foreach(Field *f, fileList) {
		if(!f->isOpen())	return false;
	}
	return true;
}

QList<FF7Var> FieldArchive::searchAllVars()
{
	QList<FF7Var> vars;
	int size = fileList.size();

	for(int i=0 ; i<size ; ++i) {
		QCoreApplication::processEvents();
		Field *field = this->field(i);
		if(field != NULL) {
			field->scriptsAndTexts()->searchAllVars(vars);
		}
	}

	return vars;
}

void FieldArchive::searchAll()
{
	int size = fileList.size();

	QSet<quint8> unknown1Values;

	for(int i=0 ; i<size ; ++i) {
		Field *field = this->field(i);
		if(field != NULL) {
			Section1File *section1 = field->scriptsAndTexts();
			if(section1->isOpen()) {
//				foreach(GrpScript *grpScript, section1->grpScripts()) {
//					foreach(Script *script, grpScript->getScripts()) {
//						foreach(Opcode *opcode, script->getOpcodes()) {
//							if(opcode->id() == Opcode::AKAO) {
//								OpcodeAKAO *akao = (OpcodeAKAO *)opcode;
//								unknown1Values.insert(akao->opcode);
//							}
//							if(opcode->id() == Opcode::AKAO2) {
//								OpcodeAKAO2 *akao = (OpcodeAKAO2 *)opcode;
//								unknown1Values.insert(akao->opcode);
//							}
//						}
//					}
//				}
				bool jp = true;
				foreach(FF7Text *text, *section1->texts()) {
					if(text->getData() != FF7Text(text->getText(jp), jp).getData()) {
						qDebug() << "error" << text->getText(jp) << FF7Text(text->getText(jp), jp).getText(jp);

					}
				}
			}
//			TutFile *tut = field->tutosAndSounds();
//			if(tut->isOpen()) {
//				qDebug() << field->getName();
//				for(int akaoID=0 ; akaoID<tut->size() ; ++akaoID) {
//					if(!tut->isTut(akaoID)) {
//						if(tut->akaoID(akaoID) >= 100) {
//							qDebug() << "error" << akaoID << tut->akaoID(akaoID);
//						}
//					}
//				}
//			}

//			InfFile *inf = field->getInf();
//			if(inf->isOpen()) {
//				inf->test();
//			}
		}
	}

	QList<quint8> l = unknown1Values.toList();
	qSort(l);
	foreach(quint8 u1, l) {
		qDebug() << u1;
	}
}

bool FieldArchive::searchIterators(QMap<QString, int>::const_iterator &i, QMap<QString, int>::const_iterator &end, int fieldID, Sorting sorting) const
{
	if(fieldID >= fileList.size())		return false;

	switch(sorting) {
	case SortByName:
		i = fieldsSortByName.constFind(fieldsSortByName.key(fieldID), fieldID);
		end = fieldsSortByName.constEnd();
		if(i==end) {
			i = fieldsSortByName.constBegin();
		}
		return true;
	case SortByMapId:
		i = fieldsSortByMapId.constFind(fieldsSortByMapId.key(fieldID), fieldID);
		end = fieldsSortByMapId.constEnd();
		if(i==end) {
			i = fieldsSortByMapId.constBegin();
		}
		return true;
	}
	return true;
}

bool FieldArchive::searchOpcode(int opcode, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, end;
	if(!searchIterators(i, end, fieldID, sorting))	return false;

	for( ; i != end ; ++i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchOpcode(opcode, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 0;
	}
	return false;
}

bool FieldArchive::searchVar(quint8 bank, quint8 adress, int value, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, end;
	if(!searchIterators(i, end, fieldID, sorting))	return false;

	for( ; i != end ; ++i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchVar(bank, adress, value, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 0;
	}
	return false;
}

bool FieldArchive::searchExec(quint8 group, quint8 script, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, end;
	if(!searchIterators(i, end, fieldID, sorting))	return false;

	for( ; i != end ; ++i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchExec(group, script, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 0;
	}
	return false;
}

bool FieldArchive::searchMapJump(quint16 _field, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, end;
	if(!searchIterators(i, end, fieldID, sorting))	return false;

	for( ; i != end ; ++i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchMapJump(_field, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 0;
	}
	return false;
}

bool FieldArchive::searchTextInScripts(const QRegExp &text, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, end;
	if(!searchIterators(i, end, fieldID, sorting))	return false;

	for( ; i != end ; ++i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchTextInScripts(text, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 0;
	}
	return false;
}

bool FieldArchive::searchText(const QRegExp &text, int &fieldID, int &textID, int &from, int &size, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, end;
	if(!searchIterators(i, end, fieldID, sorting))	return false;

	for( ; i != end ; ++i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchText(text, textID, from, size))
			return true;
		textID = from = 0;
	}
	return false;
}

bool FieldArchive::searchIteratorsP(QMap<QString, int>::const_iterator &i, QMap<QString, int>::const_iterator &begin, int fieldID, Sorting sorting) const
{
	if(fieldID < 0)		return false;

	switch(sorting) {
	case SortByName:
		begin = fieldsSortByName.constBegin();
		i = fieldsSortByName.constFind(fieldsSortByName.key(fieldID), fieldID);
		if(i==fieldsSortByName.constEnd()) {
			--i;
		}
		return true;
	case SortByMapId:
		begin = fieldsSortByMapId.constBegin();
		i = fieldsSortByMapId.constFind(fieldsSortByMapId.key(fieldID), fieldID);
		if(i==fieldsSortByMapId.constEnd()) {
			--i;
		}
		return true;
	}
	return true;
}

bool FieldArchive::searchOpcodeP(int opcode, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, begin;
	if(!searchIteratorsP(i, begin, fieldID, sorting))	return false;

	for( ; i != begin-1 ; --i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchOpcodeP(opcode, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 2147483647;
	}

	return false;
}

bool FieldArchive::searchVarP(quint8 bank, quint8 adress, int value, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, begin;
	if(!searchIteratorsP(i, begin, fieldID, sorting))	return false;

	for( ; i != begin-1 ; --i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchVarP(bank, adress, value, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 2147483647;
	}
	return false;
}

bool FieldArchive::searchExecP(quint8 group, quint8 script, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, begin;
	if(!searchIteratorsP(i, begin, fieldID, sorting))	return false;

	for( ; i != begin-1 ; --i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchExecP(group, script, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 2147483647;
	}
	return false;
}

bool FieldArchive::searchMapJumpP(quint16 _field, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, begin;
	if(!searchIteratorsP(i, begin, fieldID, sorting))	return false;

	for( ; i != begin-1 ; --i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchMapJumpP(_field, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 2147483647;
	}
	return false;
}

bool FieldArchive::searchTextInScriptsP(const QRegExp &text, int &fieldID, int &groupID, int &scriptID, int &opcodeID, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, begin;
	if(!searchIteratorsP(i, begin, fieldID, sorting))	return false;

	for( ; i != begin-1 ; --i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchTextInScriptsP(text, groupID, scriptID, opcodeID))
			return true;
		groupID = scriptID = opcodeID = 2147483647;
	}
	return false;
}

bool FieldArchive::searchTextP(const QRegExp &text, int &fieldID, int &textID, int &from, int &size, Sorting sorting)
{
	QMap<QString, int>::const_iterator i, begin;
	if(!searchIteratorsP(i, begin, fieldID, sorting))	return false;

	for( ; i != begin-1 ; --i)
	{
		QCoreApplication::processEvents();
		Field *f = field(fieldID = i.value());
		if(f!=NULL && f->scriptsAndTexts()->searchTextP(text, textID, from, size))
			return true;
		textID = from = 2147483647;
	}
	return false;
}

void FieldArchive::close()
{
	if(fic!=NULL)	fic->close();
}

void FieldArchive::addDAT(const QString &name, QList<QTreeWidgetItem *> &items)
{
	Field *field = new FieldPS(name.toLower(), this);

	QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << name.toLower() << QString());
	item->setData(0, Qt::UserRole, fileList.size());
	items.append(item);
	fileList.append(field);
}

quint8 FieldArchive::open(QList<QTreeWidgetItem *> &items)
{
	foreach(Field *field, fileList)	delete field;
	foreach(TutFile *tut, tuts)		delete tut;
	fileList.clear();
	tuts.clear();
	tutPos.clear();
	fieldsSortByName.clear();
	fieldsSortByMapId.clear();
	Data::field_names.clear();

	if(isDirectory())
	{
		QStringList list;
		list.append("*.DAT");
		list = dir->entryList(list, QDir::Files | QDir::NoSymLinks);

		emit nbFilesChanged(list.size());

		//		QTime t;t.start();

		for(int i=0 ; i<list.size() ; ++i) {
			QCoreApplication::processEvents();
			emit progress(i);

			QString path = dir->filePath(list.at(i));
			QString name = path.mid(path.lastIndexOf('/')+1);
			addDAT(name.left(name.lastIndexOf('.')), items);
		}

		//		qDebug("Ouverture : %d ms", t.elapsed());
	} else if(isDatFile()) {
		QString path = fic->fileName();
		QString name = path.mid(path.lastIndexOf('/')+1);
		addDAT(name.left(name.lastIndexOf('.')), items);
	} else if(isIso()) {
		isoFieldDirectory = iso->rootDirectory()->directory("FIELD");
		if(isoFieldDirectory == NULL) {
			return 4;
		}

		emit nbFilesChanged(isoFieldDirectory->filesAndDirectories().size());

		//		QTime t;t.start();

		int i=0;
		foreach(IsoFile *file, isoFieldDirectory->files()) {
			QCoreApplication::processEvents();
			emit progress(i++);

			if(file->name().endsWith(".DAT") && !file->name().startsWith("WM")) {
				QString name = file->name().mid(file->name().lastIndexOf('/')+1);
				addDAT(name.left(name.lastIndexOf('.')), items);
			}
		}
		//		qDebug("Ouverture : %d ms", t.elapsed());
	} else if(isLgp()) {
		if(!fic->isOpen() && !fic->open(QIODevice::ReadOnly))	return 1;

		quint32 nbFiles;

		fic->seek(12);
		fic->read((char *)&nbFiles,4);

		if(nbFiles==0 || fic->size()<16+27*nbFiles)	return 2;

		quint32 filePos;
		QList<quint32> listPos;

		QByteArray toc = fic->read(27 * nbFiles);
		const char *tocData = toc.constData();

		quint32 i;
		for(i=0 ; i<nbFiles ; ++i)
		{
			QString name(QByteArray(&tocData[27 * i], 20));
			if(!name.contains(".")) {
				memcpy(&filePos, &tocData[27 * i + 20], 4);
				listPos.append(filePos);
			} else if(name.endsWith(".tut", Qt::CaseInsensitive)) {
				memcpy(&filePos, &tocData[27 * i + 20], 4);
				name.chop(4);// strip ".tut"
				tutPos.insert(name, filePos);
			}
		}
		if(listPos.isEmpty())	return 3;
		qSort(listPos);

		emit nbFilesChanged(listPos.last());

		quint32 fileSize, lzsSize, freq = listPos.size()>50 ? listPos.size()/50 : 1;

		//		QTime t;t.start();

		Field *field;
		i = 0;
		foreach(const quint32 &pos, listPos)
		{
			if(i%freq==0) {
				QCoreApplication::processEvents();
				emit progress(pos);
			}

			if(!fic->seek(pos))	break;
			QByteArray data = fic->read(24);
			QString name(data.left(20));
			memcpy(&fileSize, data.constData()+20, 4);

			if(name.compare("maplist", Qt::CaseInsensitive) == 0)	Data::openMaplist(fic->read(fileSize));
			else
			{
				fic->read((char *)&lzsSize, 4);
				if(fileSize != lzsSize+4)	continue;

				field = new FieldPC(pos, name, this);

				QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << name << QString());
				item->setData(0, Qt::UserRole, fileList.size());
				fileList.append(field);
				items.append(item);
				++i;
			}
		}
		//		qDebug("Ouverture : %d ms", t.elapsed());
		// qDebug() << FICHIER::size << " o";
		// qDebug() << "nbGS : " << GrpScript::COUNT;
		// qDebug() << "nbS : " << Script::COUNT;
		// qDebug() << "nbC : " << Opcode::COUNT;
		// qDebug("-------------------------------------------------");
	}
	if(items.isEmpty())	return 3;

	if(Data::field_names.isEmpty()) {
		Data::openMaplist(isLgp());
	}

	int index;
	foreach(QTreeWidgetItem *item, items) {
		QString name = item->text(0);
		int id = item->data(0, Qt::UserRole).toInt();

		if((index = Data::field_names.indexOf(name)) != -1) {
			item->setText(1, QString("%1").arg(index,3));
		} else {
			item->setText(1, "~");
		}
		fieldsSortByName.insert(name, id);
		fieldsSortByMapId.insert(item->text(1), id);
	}

	return 0;
}

qint32 FieldArchive::findField(const QString &name) const
{
	qint32 i=0;
	foreach(Field *field, fileList) {
		if(field->getName().compare(name, Qt::CaseInsensitive) == 0)	return i;
		++i;
	}
	return -1;
}

void FieldArchive::setSaved()
{
	foreach(Field *field, fileList) {
		field->setSaved();
	}
	foreach(TutFile *tut, tuts) {
		tut->setModified(false);
	}
}

quint8 FieldArchive::save(QString path)
{
	quint32 nbFiles, pos, taille, oldtaille, fileSize;
	qint32 fieldID;
	bool saveAs;

	clearCachedData(); // Important: the file data will change

	if(isDirectory() || isDatFile())
	{
		nbFiles = fileList.size();
		emit nbFilesChanged(nbFiles);

		for(quint32 fieldID=0 ; fieldID<nbFiles ; ++fieldID)
		{
			Field *field = fileList.at(fieldID);
			if(field->isOpen() && field->isModified())
			{
				QTemporaryFile tempFic;
				if(!tempFic.open())		return 2;
				QFile DATfic(isDatFile() ? fic->fileName() : dir->filePath(field->getName()+".DAT"));
				if(!DATfic.open(QIODevice::ReadOnly)) return 1;
				DATfic.read((char *)&fileSize,4);
				tempFic.write(field->save(DATfic.read(fileSize), true));
				if(!DATfic.remove()) return 4;
				tempFic.copy(DATfic.fileName());
			}
			emit progress(fieldID);
		}

		setSaved();

		return 0;
	}
	else if(isLgp())
	{
		QMap<quint32, quint32> positions1, positions2;
		QMap<Field *, quint32> newPositions;
		QMap<QString, quint32> newPositionsTut;
		QByteArray fileName, newFile;
		QString tutName;

		if(path.isNull())
			path = fic->fileName();

		// Replace if the new path is the same as the old
		saveAs = QFileInfo(path) != QFileInfo(*fic);

		// QFile fic(this->path());
		if(!fic->isOpen() && !fic->open(QIODevice::ReadOnly))	return 1;
		QFile tempFic(path % ".makoutemp");
		if(!tempFic.open(QIODevice::WriteOnly))		return 2;

		fic->seek(12);
		fic->read((char *)&nbFiles, 4);

		if(nbFiles>1000 || nbFiles==0) 	return 5;

		emit nbFilesChanged(nbFiles+21);

		//QTime t;t.start();
		//Parcourir la table des mati�re
		QByteArray toc = fic->read(27 * nbFiles);
		const char *tocData = toc.constData();

		for(quint32 i=0 ; i<nbFiles ; ++i)
		{
			memcpy(&pos, &tocData[27 * i + 20], 4);
			positions1.insert(pos, i);
		}

		QMap<quint32, quint32>::const_iterator i = positions1.constBegin();
		const quint32 positionFirstFile = i.key();

		// qDebug() << i.key() << (27*nbFiles + 16);//CRC size

		fic->reset();
		tempFic.write(fic->read(positionFirstFile));

//		qDebug("Lister les positions des fichiers : %d ms", t.elapsed());
//		t.restart();

		quint16 avancement = 0;
		while(i != positions1.constEnd())
		{
			QCoreApplication::processEvents();

			if(!fic->seek(i.key()))	return 3;

			//Listage positions 2
			pos = tempFic.pos();
			positions2.insert(i.value(), pos);

			fieldID = findField(QString(fileName = fic->read(20)));
			if(fileName.size() != 20 || fic->read((char *)&oldtaille, 4) != 4) 	return 3;
			tempFic.write(fileName);// File Name
			// qDebug() << QString(fileName);

			if(fieldID != -1)
			{
				Field *field = fileList.at(fieldID);
				// qDebug() << "[FIELD]" << fieldID << field->getName();

				newPositions.insert(field, pos);

				//v�rifier si on a pas une nouvelle version du fichier
				if(field->isOpen() && field->isModified())
				{
//					qDebug() << field->getName() << "======== modified";
					//R�cup�rer l'ancien fichier
					if(fic->read((char *)&fileSize, 4) != 4)	return 3;
					if(oldtaille != fileSize+4)					return 3;
					//Cr�er le nouveau fichier � partir de l'ancien
					newFile = field->save(fic->read(fileSize), true);

					//Refaire les tailles
					taille = newFile.size();

					//�crire le nouveau fichier dans le fichier temporaire
					tempFic.write((char *)&taille, 4);//nouvelle taille
					tempFic.write(newFile);//nouveau fichier
				}
				else
				{
//					qDebug() << "unmodified";
					tempFic.write((char *)&oldtaille, 4);//Taille
					tempFic.write(fic->read(oldtaille));//Fichier
				}
			}
			else if(QString(fileName).endsWith(".tut", Qt::CaseInsensitive) && tuts.contains(tutName = fileName.left(fileName.lastIndexOf('.'))))
			{
				TutFile *tut = tuts.value(tutName);

//				qDebug() << "[TUT]" << tutName;

				newPositionsTut.insert(tutName, pos);

				if(tut->isModified())
				{
//					qDebug() << "======== modified";
					QByteArray tocTut, tutData;
					tutData = tut->save(tocTut);
					tocTut.append(tutData);
					taille = tocTut.size();
					tempFic.write((char *)&taille, 4);//Taille
					tempFic.write(tocTut);//Fichier
				}
				else
				{
//					qDebug() << "unmodified";
					tempFic.write((char *)&oldtaille, 4);//Taille
					tempFic.write(fic->read(oldtaille));//Fichier
				}
			}
			else
			{
//				qDebug() << "[NOTHING] unmodified" << QString(fileName);
				tempFic.write((char *)&oldtaille, 4);//Taille
				tempFic.write(fic->read(oldtaille));//Fichier
			}
//			qDebug("Ecrire un fichier : %d ms", t.elapsed());
//			t.restart();
			emit progress(avancement++);
			++i;
		}

//		tempFic.write(fic->readAll());
		tempFic.write("FINAL FANTASY7", 14);

//		qDebug("Ecrire les fichiers : %d ms", t.elapsed());
//		t.restart();

		//fabrication de la nouvelle table des mati�res
		for(quint32 i=0 ; i<nbFiles ; ++i)
		{
			tempFic.seek(16 + i*27 + 20);
			pos = positions2.value(i);
			tempFic.write((char *)&pos, 4);
		}

//		qDebug("Ecrire la table des mati�res : %d ms", t.elapsed());
//		t.restart();

		emit progress(nbFiles+7);

		// Remove or close the old file
		if(!saveAs) {
			if(!fic->remove())	return 4;
		} else {
			fic->close();
			fic->setFileName(path);
		}

		emit progress(nbFiles+14);
		//t.restart();

		// Move the temporary file
		if(QFile::exists(path) && !QFile::remove(path))		return 4;
		if(!tempFic.rename(path))			return 4;

		//qDebug("Copy the temporary file: %d ms", t.elapsed());

		fic->open(QIODevice::ReadOnly);// reopen archive

		// t.restart();
		// Update fieldPC positions
		QMapIterator<Field *, quint32> fieldIt(newPositions);
		while(fieldIt.hasNext()) {
			fieldIt.next();
			((FieldPC *)fieldIt.key())->setPosition(fieldIt.value());
		}
		// Update tut positions
		QMapIterator<QString, quint32> tutIt(newPositionsTut);
		while(tutIt.hasNext()) {
			tutIt.next();
			tutPos.insert(tutIt.key(), tutIt.value());
		}
		emit progress(nbFiles+21);

		// Clear "isModified" state
		setSaved();

		// qDebug("Ecrire le nouvel Lgp : %d ms", t.elapsed());
		return 0;
	}
	else if(isIso())
	{
		if(path.isNull())
			path = iso->fileName();

		saveAs = QFileInfo(path) != QFileInfo(*iso);

		IsoArchive isoTemp(path%".makoutemp");
		if(!isoTemp.open(QIODevice::ReadWrite | QIODevice::Truncate))		return 2;

		emit nbFilesChanged(100);

		// FIELD/*.DAT

		bool archiveModified = false;

		foreach(Field *field, fileList) {
			if(field->isOpen() && field->isModified()) {
				IsoFile *isoField = isoFieldDirectory->file(field->getName().toUpper() + ".DAT");
				if(isoField == NULL) {
					continue;
				}

				isoField->setData(field->save(iso->file(isoField).mid(4), true));
				archiveModified = true;
			}
		}

		if(!archiveModified) {
			return 3;
		}

		// FIELD/FIELD.BIN

		IsoFile *isoFieldBin = isoFieldDirectory->file("FIELD.BIN");
		if(isoFieldBin == NULL) {
			return 3;
		}

		QByteArray data = updateFieldBin(iso->file(isoFieldBin), isoFieldDirectory);
		if(data.isEmpty()) {
			return 3;
		}
		isoFieldBin->setData(data);

//		qDebug() << "start";

		if(!iso->pack(&isoTemp, this, isoFieldDirectory))
			return 3;

		// End

		// Remove or close the old file
		if(!saveAs) {
			if(!iso->remove())	return 4;
		} else {
			iso->close();
			iso->setFileName(path);
		}
		// Move the temporary file
		if(QFile::exists(path) && !QFile::remove(path))		return 4;
		isoTemp.rename(path);

		if(!iso->open(QIODevice::ReadOnly))		return 4;

		// Clear "isModified" state
		iso->applyModifications(isoFieldDirectory);
		setSaved();

		return 0;
	}

	return 6;
}

QByteArray FieldArchive::updateFieldBin(const QByteArray &data, IsoDirectory *fieldDirectory)
{
	QByteArray header = data.left(8), ungzip;
	quint32 oldSectorStart, newSectorStart, oldSize, newSize;
	QMap<QByteArray, QByteArray> changementsFieldBin;
	QMap<QByteArray, QString> fichiers;//debug

	foreach(IsoDirectory *fileOrDir, fieldDirectory->directories()) {
		if(fileOrDir->isModified() && !fileOrDir->name().endsWith(".X") && fileOrDir->name()!="FIELD.BIN") {
			oldSectorStart = fileOrDir->location();
			oldSize = fileOrDir->size();
			newSectorStart = fileOrDir->newLocation();
			newSize = fileOrDir->newSize();

			QByteArray tempBA = QByteArray((char *)&oldSectorStart, 4).append((char *)&oldSize, 4);
			changementsFieldBin.insert(tempBA, QByteArray((char *)&newSectorStart, 4).append((char *)&newSize, 4));
			fichiers.insert(tempBA, fileOrDir->name());
		}
	}

	/**
	 *	HEADER :
	 *	 4 : ungzipped size
	 *	 4 : ungzipped size - 51588
	 */
	// d�compression gzip
	quint32 decSize;
	memcpy(&decSize, data.constData(), 4);
	ungzip = GZIP::decompress(data.mid(8), decSize);
	if(ungzip.isEmpty())	return QByteArray();

//	qDebug() << "header field.bin" << header.toHex() << QString::number(data.size()-8,16) << decSize << ungzip.size();

	// mise � jour

	int indicativePosition=0x30000, count;
	QByteArray copy = ungzip;
	QMapIterator<QByteArray, QByteArray> i(changementsFieldBin);
	while(i.hasNext()) {
		i.next();
		count = copy.count(i.key());
		if(count == 0) {
			qWarning() << "Error not found!" << fichiers.value(i.key());
			return QByteArray();
		} else if(count == 1) {
			indicativePosition = copy.indexOf(i.key());
			ungzip.replace(i.key(), i.value());
		} else {
			qWarning() << "error multiple occurrences 1" << fichiers.value(i.key());
			if(copy.mid(indicativePosition).count(i.key())==1) {
				ungzip.replace(copy.indexOf(i.key(), indicativePosition), 8, i.value());
			} else {
				qWarning() << "error multiple occurrences 2" << fichiers.value(i.key());
				return QByteArray();
			}
		}
	}

	copy = GZIP::compress(ungzip);

	if(copy.isEmpty())	return QByteArray();

	return header.append(copy);
}
