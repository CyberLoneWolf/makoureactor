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
#include "FontWidget.h"
#include "core/FF7Text.h"
#include "core/TexFile.h"
#include "Parameters.h"

FontWidget::FontWidget(QWidget *parent) :
	QWidget(parent), ff7Font(0)
{
	fontGrid = new FontGrid(16, 16, this);
	fontLetter = new FontLetter(this);
	fontPalette = new FontPalette(this);
	selectPal = new QComboBox(this);
	QStringList colors;
	colors << tr("Gris") << tr("Bleu") << tr("Rouge") << tr("Violet") << tr("Vert") << tr("Cyan") << tr("Jaune") << tr("Blanc");
	selectPal->addItems(colors);
	selectPal->setCurrentIndex(selectPal->count() - 1);

	selectTable = new QComboBox(this);

	fromImage1 = new QPushButton(tr("� partir d'une image..."), this);
	fromImage1->setVisible(false);//TODO
	fromImage2 = new QPushButton(tr("� partir d'une image..."), this);
	fromImage2->setVisible(false);//TODO
//	QPushButton *resetButton1 = new QPushButton(tr("Annuler les modifications"), this);//TODO
	textLetter = new QLineEdit(this);
	exportButton = new QPushButton(tr("Exporter..."), this);
	importButton = new QPushButton(tr("Importer..."), this);
	resetButton2 = new QPushButton(tr("Annuler les modifications"), this);
	resetButton2->setEnabled(false);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(fontGrid, 0, 0, 1, 2, Qt::AlignRight);
	layout->addWidget(fontLetter, 0, 2, 2, 2, Qt::AlignLeft);
	layout->addWidget(fromImage1, 1, 0, 1, 2, Qt::AlignLeft);
	layout->addWidget(selectPal, 2, 0, Qt::AlignRight);
	layout->addWidget(selectTable, 2, 1, Qt::AlignLeft);
	layout->addWidget(fontPalette, 2, 2, 1, 2, Qt::AlignLeft);
	layout->addWidget(textLetter, 3, 2);
	layout->addWidget(fromImage2, 3, 3, Qt::AlignRight);
//	layout->addWidget(resetButton1, 4, 0, 1, 2, Qt::AlignLeft);
	layout->addWidget(exportButton, 4, 0, Qt::AlignLeft);
	layout->addWidget(importButton, 4, 1, Qt::AlignLeft);
	layout->addWidget(resetButton2, 4, 2, 1, 2, Qt::AlignRight);
	layout->setRowStretch(5, 1);
	layout->setColumnStretch(3, 1);
	layout->setContentsMargins(QMargins());

	connect(selectPal, SIGNAL(currentIndexChanged(int)), SLOT(setColor(int)));
	connect(selectTable, SIGNAL(currentIndexChanged(int)), SLOT(setTable(int)));
	connect(fontGrid, SIGNAL(letterClicked(int)), SLOT(setLetter(int)));
	connect(fontLetter, SIGNAL(imageChanged(QRect)), fontGrid, SLOT(updateLetter(QRect)));
	connect(fontLetter, SIGNAL(imageChanged(QRect)), SLOT(setModified()));
//	connect(resetButton1, SIGNAL(clicked()), SLOT(reset()));
	connect(exportButton, SIGNAL(clicked()), SLOT(exportFont()));
	connect(importButton, SIGNAL(clicked()), SLOT(importFont()));
	connect(resetButton2, SIGNAL(clicked()), SLOT(resetLetter()));
	connect(fontPalette, SIGNAL(colorChanged(int)), fontLetter, SLOT(setPixelIndex(int)));
	connect(textLetter, SIGNAL(textEdited(QString)), SLOT(editLetter(QString)));
}

void FontWidget::clear()
{
	fontGrid->clear();
	fontLetter->clear();
	fontPalette->clear();
}

void FontWidget::setFF7Font(FF7Font *ff7Font)
{
	this->ff7Font = ff7Font;
	setWindowBinFile(ff7Font->windowBinFile());
	setReadOnly(ff7Font->isReadOnly());
}

void FontWidget::setWindowBinFile(WindowBinFile *windowBinFile)
{
	fontGrid->setWindowBinFile(windowBinFile);
	fontLetter->setWindowBinFile(windowBinFile);
	fontPalette->setWindowBinFile(windowBinFile);
	setLetter(0);

	if(selectTable->count() != windowBinFile->tableCount()) {
		selectTable->clear();
		for(int i=1 ; i<=windowBinFile->tableCount() ; ++i) {
			selectTable->addItem(tr("Table %1").arg(i));
		}
		selectTable->setEnabled(selectTable->count() > 1);
	}
}

void FontWidget::setReadOnly(bool ro)
{
	textLetter->setReadOnly(ro);
	fontLetter->setReadOnly(ro);
	fontPalette->setReadOnly(ro);
	fromImage1->setDisabled(ro);
	fromImage2->setDisabled(ro);
}

void FontWidget::setColor(int i)
{
	fontGrid->setColor((WindowBinFile::FontColor)i);
	fontLetter->setColor((WindowBinFile::FontColor)i);
	fontPalette->setCurrentPalette((WindowBinFile::FontColor)i);
}

void FontWidget::setTable(int i)
{
	fontGrid->setCurrentTable(i);
	fontLetter->setCurrentTable(i);
	setLetter(0);
}

void FontWidget::setLetter(int i)
{
	fontLetter->setLetter(i);
	fontGrid->setLetter(i);

	QByteArray ba;

	if(fontGrid->currentTable() >= 1 && fontGrid->currentTable() <= 5) {
		ba.append(char(0xF9 + fontGrid->currentTable()));
	}

	if(fontGrid->currentTable() <= 5) {
		if(ff7Font) {
//			textLetter->setText(FF7Text(ba.append((char)i), ff7Font->tables()));//TODO: ff7Font
		} else {
			textLetter->setText(FF7Text(ba.append((char)i)).text(false));//TODO: jp
		}
	}
	resetButton2->setEnabled(false);
}

void FontWidget::editLetter(const QString &letter)
{
	if(ff7Font) {
		ff7Font->setChar(fontGrid->currentTable(), fontGrid->currentLetter(), letter);
	}
}

void FontWidget::exportFont()
{
	WindowBinFile *windowBinFile = fontGrid->windowBinFile();

	QString binF, tdwF, txtF, pngF, jpgF, bmpF;
	QStringList filter;
	if(windowBinFile) {
		filter.append(binF = tr("Fichier police FF7 (*.bin)"));
		filter.append(tdwF = tr("Fichier police FF8 (*.tdw)"));
		filter.append(pngF = tr("Fichier image PNG (*.png)"));
		filter.append(jpgF = tr("Fichier image JPG (*.jpg)"));
		filter.append(bmpF = tr("Fichier image BMP (*.bmp)"));
	}

	if(ff7Font)
		filter.append(txtF = tr("Fichier traduction %1 (*.txt)").arg(PROG_NAME));
	QString selectedFilter;

	QString path = QFileDialog::getSaveFileName(this, tr("Exporter police de caract�re"), "sysfnt", filter.join(";;"), &selectedFilter);
	if(path.isNull())		return;

	if(selectedFilter == binF) {
		/*QByteArray data;
		//TODO
		if(windowBinFile->save(data)) {
			QFile f(path);
			if(f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
				f.write(data);
				f.close();
			} else {
				QMessageBox::warning(this, tr("Erreur"), tr("Erreur d'ouverture du fichier. (%1)").arg(f.errorString()));
			}
		} else {
			QMessageBox::warning(this, tr("Erreur"), tr("Erreur lors de l'enregistrement."));
		}*/
	} else if(selectedFilter == tdwF) {
		/*QByteArray data;
		//TODO
		if(windowBinFile->save(data)) {
			QFile f(path);
			if(f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
				f.write(data);
				f.close();
			} else {
				QMessageBox::warning(this, tr("Erreur"), tr("Erreur d'ouverture du fichier. (%1)").arg(f.errorString()));
			}
		} else {
			QMessageBox::warning(this, tr("Erreur"), tr("Erreur lors de l'enregistrement."));
		}*/
	} else if(selectedFilter == txtF) {
		QString data = ff7Font->saveTxt();
		QFile f(path);
		if(f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			f.write(data.toUtf8());
			f.close();
		} else {
			QMessageBox::warning(this, tr("Erreur"), tr("Erreur d'ouverture du fichier. (%1)").arg(f.errorString()));
		}
	} else if(selectedFilter == pngF ||
			  selectedFilter == jpgF ||
			  selectedFilter == bmpF) {
		char *format;
		if(selectedFilter == pngF) {
			format = (char *)"PNG";
		} else if(selectedFilter == jpgF) {
			format = (char *)"JPG";
		} else {
			format = (char *)"BMP";
		}
		windowBinFile->image((WindowBinFile::FontColor)selectPal->currentIndex()).save(path, format);
	}
}

void FontWidget::importFont()
{
	WindowBinFile *windowBinFile = fontGrid->windowBinFile();

	QString tdwF;
	QStringList filter;
	if(windowBinFile) {
		filter.append(tdwF = tr("Fichier police FF8 (*.tdw)"));
	}
	QString selectedFilter;

	QString path = QFileDialog::getOpenFileName(this, tr("Importer police de caract�re"), "sysfnt", filter.join(";;"), &selectedFilter);
	if(path.isNull())		return;

	if(selectedFilter == tdwF) {
		QFile f(path);
		if(f.open(QIODevice::ReadOnly)) {
			WindowBinFile newWindowBinFile;
			if(newWindowBinFile.open(f.readAll())) {
				newWindowBinFile.setModified(true);
				*windowBinFile = newWindowBinFile;
				setWindowBinFile(windowBinFile);// update
				emit letterEdited();
			} else {
				QMessageBox::warning(this, tr("Erreur"), tr("Fichier invalide"));
			}
			f.close();
		} else {
			QMessageBox::warning(this, tr("Erreur"), tr("Erreur d'ouverture du fichier. (%1)").arg(f.errorString()));
		}
	}
}

void FontWidget::reset()
{
//	fontGrid->reset();//TODO
	fontLetter->update();
}

void FontWidget::resetLetter()
{
	fontLetter->reset();
	fontGrid->update();
	resetButton2->setEnabled(false);
}

void FontWidget::setModified()
{
	emit letterEdited();
	resetButton2->setEnabled(true);
}

void FontWidget::focusInEvent(QFocusEvent *)
{
	fontGrid->setFocus();
}
