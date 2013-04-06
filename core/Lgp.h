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
/*
 * This file may contains some code (especially the conflict part)
 * inspired from LGP/UnLGP tool written by Aali.
 * http://forums.qhimm.com/index.php?topic=8641.0
 */
#ifndef LGP_H
#define LGP_H

#include <QtCore>

struct LgpObserver
{
	LgpObserver() {}
	virtual bool observerWasCanceled() const=0;
	virtual void setObserverMaximum(unsigned int max)=0;
	virtual void setObserverValue(int value)=0;
};

class LgpHeaderEntry;
class LgpToc;

class LgpIterator
{
public:
	LgpIterator(LgpToc *toc, QFile *lgp);
	bool hasNext() const;
	bool hasPrevious() const;
	void next();
	void previous();
	void toBack();
	void toFront();
	QIODevice *file();
	QIODevice *modifiedFile();
	const QString &fileName() const;
	const QString &fileDir() const;
	QString filePath() const;
private:
	QHashIterator<quint16, LgpHeaderEntry *> it;
	QFile *_lgp;
};

class Lgp
{
public:
	enum LgpError {
		NoError,
		ReadError,
		WriteError,
		OpenError,
		AbortError,
		TimeOutError,
		RemoveError,
		RenameError,
		PositionError,
		ResizeError,
		PermissionsError,
		CopyError,
		InvalidError,
		FileNotFoundError
	};

	Lgp();
	Lgp(const QString &name);
	virtual ~Lgp();
	void clear();
	QStringList fileList();
	int fileCount();
	LgpIterator iterator();
	bool fileExists(const QString &filePath);
	QIODevice *file(const QString &filePath);
	QByteArray fileData(const QString &filePath);
	QIODevice *modifiedFile(const QString &filePath);
	QByteArray modifiedFileData(const QString &filePath);
	bool setFile(const QString &filePath, QIODevice *data);
	bool setFile(const QString &filePath, const QByteArray &data);
	bool addFile(const QString &filePath, QIODevice *data);
	bool addFile(const QString &filePath, const QByteArray &data);
	bool removeFile(const QString &filePath);
	bool renameFile(const QString &filePath, const QString &newFilePath);
	const QString &companyName();
	void setCompanyName(const QString &companyName);
	const QString &productName();
	void setProductName(const QString &productName);
	bool open();
	bool isOpen() const;
	void close();
	QString fileName() const;
	void setFileName(const QString &fileName);
	bool pack(const QString &destination=QString(), LgpObserver *observer=NULL);
	LgpError error() const;
	void unsetError();
	QString errorString() const;
private:
	bool openHeader();
	bool openCompanyName();
	bool openProductName();
	LgpHeaderEntry *headerEntry(const QString &filePath);
	void setError(LgpError error, const QString &errorString=QString());

	QString _companyName;
	LgpToc *_files;
	QString _productName;
	QFile _file;
	LgpError _error;
	QString _errorString;

};

#endif // LGP_H