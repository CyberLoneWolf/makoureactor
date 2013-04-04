#include "BackgroundFilePS.h"
#include "Palette.h"

BackgroundFilePS::BackgroundFilePS() :
	BackgroundFile()
{
}

QPixmap BackgroundFilePS::openBackground(const QByteArray &datDataDec, const QByteArray &mimDataDec, const QHash<quint8, quint8> &paramActifs, const qint16 *z, const bool *layers)
{
	/*--- OUVERTURE DU MIM ---*/
	const char *constMimData = mimDataDec.constData();
	quint32 mimDataSize = mimDataDec.size();
	MIM headerPal, headerImg, headerEffect;
	quint32 i;

	if(mimDataSize < 12)	return QPixmap();

	memcpy(&headerPal, constMimData, 12);

	if(mimDataSize < (quint32)12+headerPal.h*512)	return QPixmap();

	QList<Palette> palettes;
	for(i=0 ; i<headerPal.h ; ++i)
		palettes.append(Palette(mimDataDec.mid(12+i*512,512)));

	if(mimDataSize < headerPal.size + 12)	return QPixmap();

	memcpy(&headerImg, constMimData + headerPal.size, 12);

	headerImg.w *= 2;

	if(headerPal.size+headerImg.size+12 < mimDataSize)
	{
		memcpy(&headerEffect, constMimData + headerPal.size+headerImg.size, 12);
		headerEffect.w *= 2;
	}
	else
	{
		headerEffect.size = 4;
		headerEffect.w = 0;
		headerEffect.x = 0;
	}

	/*--- OUVERTURE DU DAT ---*/
	const char *constDatData = datDataDec.constData();
	quint32 datDataSize = datDataDec.size();
	quint32 debut1, debut2, debut3, debut4;

	/* // MODEL SECTION
	quint32 debutSection7, start2 = debutSection7 - start + 28;
	quint16 size7, nb_model7;

	memcpy(&debutSection7, constDatData + 24, 4);
	memcpy(&size7, constDatData + start2, 2);
	memcpy(&nb_model7, constDatData + start2+2, 2);
	qDebug() << QString("Size = %1 | nbModels = %2").arg(size7).arg(nb_model7);
	for(i=0 ; i<nb_model7 ; ++i) {
		qDebug() << QString("??? = %1 | ??? = %2 | ??? = %3 | nbAnims = %4").arg((quint8)datData.at(start2+4+i*8)).arg((quint8)datData.at(start2+5+i*8)).arg((quint8)datData.at(start2+6+i*8)).arg((quint8)datData.at(start2+7+i*8));
		qDebug() << QString("??? = %1 | ??? = %2 | ??? = %3 | ??? = %4").arg((quint8)datData.at(start2+8+i*8)).arg((quint8)datData.at(start2+9+i*8)).arg((quint8)datData.at(start2+10+i*8)).arg((quint8)datData.at(start2+11+i*8));
	}
	fdebug.write(datData.mid(start2));
	fdebug.close(); */

	if(datDataSize < 16)	return QPixmap();

	memcpy(&debut1, constDatData, 4);
	memcpy(&debut2, constDatData + 4, 4);
	memcpy(&debut3, constDatData + 8, 4);
	memcpy(&debut4, constDatData + 12, 4);

	quint16 tilePos=0, tileCount=0;
	QList<quint32> nbTilesTex, nbTilesLayer;
	quint8 layerID=0;
	qint16 type;
	i = 16;
	while(i<debut1)
	{
		if(datDataSize < i+2)	return QPixmap();

		memcpy(&type, constDatData + i, 2);

		if(type==0x7FFF)
		{
			nbTilesLayer.append(tilePos+tileCount);
			++layerID;
		}
		else
		{
			if(type==0x7FFE)
			{
				memcpy(&tilePos, constDatData + i-4, 2);
				memcpy(&tileCount, constDatData + i-2, 2);

				nbTilesTex.append(tilePos+tileCount);
			}
			else {
				if(datDataSize < i+6)	return QPixmap();

				memcpy(&tilePos, constDatData + i+2, 2);
				memcpy(&tileCount, constDatData + i+4, 2);
			}
			i+=4;
		}
		i+=2;
	}

	QList<layer2Tile> tiles2;
	QMultiMap<qint16, Tile> tiles;
	layer1Tile tile1;
	layer2Tile tile2;
	layer3Tile tile4;
	paramTile tile3;
	Tile tile;
	quint16 texID=0, largeurMax=0, largeurMin=0, hauteurMax=0, hauteurMin=0;
	quint32 size, tileID=0;

	size = (debut3-debut2)/2;

	if(datDataSize < debut2+size*2)	return QPixmap();

	for(i=0 ; i<size ; ++i) {
		memcpy(&tile2, constDatData + debut2+i*2, 2);
		tiles2.append(tile2);
	}
	if(tiles2.isEmpty()) {
		return QPixmap();
	}
	tile2 = tiles2.first();

	size = (debut2-debut1)/8;

	if(datDataSize < debut1+size*8)	return QPixmap();

	for(i=0 ; i<size ; ++i)
	{
		memcpy(&tile1, constDatData + debut1+i*8, 8);
		if(qAbs(tile1.cibleX) < 1000 && qAbs(tile1.cibleY) < 1000) {
			if(tile1.cibleX >= 0 && tile1.cibleX > largeurMax)
				largeurMax = tile1.cibleX;
			else if(tile1.cibleX < 0 && -tile1.cibleX > largeurMin)
				largeurMin = -tile1.cibleX;
			if(tile1.cibleY >= 0 && tile1.cibleY > hauteurMax)
				hauteurMax = tile1.cibleY;
			else if(tile1.cibleY < 0 && -tile1.cibleY > hauteurMin)
				hauteurMin = -tile1.cibleY;

			tile.cibleX = tile1.cibleX;
			tile.cibleY = tile1.cibleY;
			tile.srcX = tile1.srcX;
			tile.srcY = tile1.srcY;
			tile.paletteID = tile1.palID;

			if(texID+1<tiles2.size() && texID+1<nbTilesTex.size() && tileID>=nbTilesTex.at(texID)) {
				++texID;
				tile2 = tiles2.at(texID);
			}

			tile.textureID = tile2.page_x;
			tile.textureID2 = tile2.page_y;
			tile.deph = tile2.deph;
			tile.typeTrans = tile2.typeTrans;

			tile.param = tile.state = tile.blending = 0;
			tile.ID = 4095;

			tile.size = 16;

			if(layers==NULL || layers[0])
				tiles.insert(4096-tile.ID, tile);
		}
		++tileID;
	}

	size = (debut4-debut3)/14;

	if(datDataSize < debut3+size*14)	return QPixmap();

	for(i=0 ; i<size ; ++i)
	{
		memcpy(&tile1, constDatData + debut3+i*14, 8);
		if(qAbs(tile1.cibleX) < 1000 || qAbs(tile1.cibleY) < 1000) {
			if(tile1.cibleX >= 0 && tile1.cibleX > largeurMax)
				largeurMax = tile1.cibleX;
			else if(tile1.cibleX < 0 && -tile1.cibleX > largeurMin)
				largeurMin = -tile1.cibleX;
			if(tile1.cibleY >= 0 && tile1.cibleY > hauteurMax)
				hauteurMax = tile1.cibleY;
			else if(tile1.cibleY < 0 && -tile1.cibleY > hauteurMin)
				hauteurMin = -tile1.cibleY;

			tile.cibleX = tile1.cibleX;
			tile.cibleY = tile1.cibleY;
			tile.srcX = tile1.srcX;
			tile.srcY = tile1.srcY;
			tile.paletteID = tile1.palID;

			memcpy(&tile2, constDatData + debut3+i*14+8, 2);
			memcpy(&tile3, constDatData + debut3+i*14+10, 4);

			tile.param = tile3.param;
			tile.state = tile3.state;

			if(tile.state==0 || paramActifs.value(tile.param, 0)&tile.state) {
				tile.textureID = tile2.page_x;
				tile.textureID2 = tile2.page_y;
				tile.deph = tile2.deph;
				tile.typeTrans = tile2.typeTrans;

				tile.blending = tile3.blending;
				tile.ID = tile3.group;

				tile.size = 16;

				if(layers==NULL || layers[1])
					tiles.insert(4096-tile.ID, tile);
			}
		}
		++tileID;
	}

	layerID = 2;

	size = (datDataSize-debut4)/10;

	if(datDataSize < debut4+size*10)	return QPixmap();

	for(i=0 ; i<size ; ++i)
	{
		memcpy(&tile1, constDatData + debut4+i*10, 8);
		if(qAbs(tile1.cibleX) < 1000 || qAbs(tile1.cibleY) < 1000) {
			if(tile1.cibleX >= 0 && tile1.cibleX+16 > largeurMax)
				largeurMax = tile1.cibleX+16;
			else if(tile1.cibleX < 0 && -tile1.cibleX > largeurMin)
				largeurMin = -tile1.cibleX;
			if(tile1.cibleY >= 0 && tile1.cibleY+16 > hauteurMax)
				hauteurMax = tile1.cibleY+16;
			else if(tile1.cibleY < 0 && -tile1.cibleY > hauteurMin)
				hauteurMin = -tile1.cibleY;

			tile.cibleX = tile1.cibleX;
			tile.cibleY = tile1.cibleY;
			tile.srcX = tile1.srcX;
			tile.srcY = tile1.srcY;
			tile.paletteID = tile1.palID;

			memcpy(&tile4, constDatData + debut4+i*10+8, 2);

			tile.param = tile4.param;
			tile.state = tile4.state;

			if(texID+1<tiles2.size() && texID+1<nbTilesTex.size() && tileID>=nbTilesTex.at(texID)) {
				++texID;
				tile2 = tiles2.at(texID);
			}

			if(layerID+1<nbTilesLayer.size() && tileID>=nbTilesLayer.at(layerID)) {
				++layerID;
			}

			if(tile.state==0 || paramActifs.value(tile.param, 0)&tile.state) {
				tile.blending = tile4.blending;

				tile.textureID = tile2.page_x;
				tile.textureID2 = tile2.page_y;
				tile.deph = tile2.deph;
				tile.typeTrans = tile2.typeTrans;

				tile.ID = layerID==2 ? 4096 : 0;
				tile.size = 32;

				if(layers==NULL || layerID>3 || layers[layerID])
					tiles.insert(4096-(z[layerID!=2]!=-1 ? z[layerID!=2] : tile.ID), tile);
			}
		}
		++tileID;
	}

	int imageWidth = largeurMin + largeurMax + 16;
	QImage image(imageWidth, hauteurMin + hauteurMax + 16, QImage::Format_ARGB32);
	QRgb *pixels = (QRgb *)image.bits();
	quint32 origin, top;
	quint16 width, deuxOctets, baseX;
	quint8 index, right;

	image.fill(0xFF000000);

	foreach(const Tile &tile, tiles)
	{
		width = tile.textureID2 ? headerEffect.w : headerImg.w;
		texID = tile.textureID - (tile.textureID2 ? headerEffect.x/64 : headerImg.x/64);
		origin = headerPal.size + 12 + (tile.textureID2 ? headerImg.size : 0) + tile.srcY*width + tile.srcX*tile.deph + texID*128;
		right = 0;
		top = (hauteurMin + tile.cibleY) * imageWidth;
		baseX = largeurMin + tile.cibleX;

		if(tile.deph == 2)
		{

			for(quint16 j=0 ; j<width*tile.size ; j+=2)
			{
				memcpy(&deuxOctets, constMimData + origin+j, 2);
				if(deuxOctets!=0)
					pixels[baseX + right + top] = PsColor::fromPsColor(deuxOctets);

				if(++right==tile.size)
				{
					right = 0;
					j += width-tile.size*2;
					top += imageWidth;
				}
			}
		}
		else if(tile.deph == 1)
		{
			const Palette &palette = palettes.at(tile.paletteID);

			for(quint16 j=0 ; j<width*tile.size ; ++j)
			{
				index = (quint8)mimDataDec.at(origin+j);

				if(!palette.isZero.at(index)) {
					if(tile.blending) {
						pixels[baseX + right + top] = blendColor(tile.typeTrans, pixels[baseX + right + top], palette.couleurs.at(index));
					} else {
						pixels[baseX + right + top] = palette.couleurs.at(index);
					}
				}

				if(++right==tile.size)
				{
					right = 0;
					j += width-tile.size;
					top += imageWidth;
				}
			}
		}
	}

	return QPixmap::fromImage(image);
}

bool BackgroundFilePS::usedParams(const QByteArray &datDataDec, QHash<quint8, quint8> &usedParams, bool *layerExists)
{
	/*--- OUVERTURE DU DAT ---*/
	const char *constDatData = datDataDec.constData();
	quint32 datDataSize = datDataDec.size();
	quint32 i, debut1, debut2, debut3, debut4;

	memcpy(&debut1, constDatData, 4);
	memcpy(&debut2, constDatData + 4, 4);
	memcpy(&debut3, constDatData + 8, 4);
	memcpy(&debut4, constDatData + 12, 4);

	quint16 tilePos=0, tileCount=0;
	QList<quint32> nbTilesLayer;
	quint8 layerID=0;
	qint16 type;
	i = 16;
	while(i<debut1)
	{
		memcpy(&type, constDatData + i, 2);

		if(type==0x7FFF)
		{
			nbTilesLayer.append(tilePos+tileCount);
		}
		else
		{
			if(type==0x7FFE)
			{
				memcpy(&tilePos, constDatData + i-4, 2);
				memcpy(&tileCount, constDatData + i-2, 2);
			}
			else {
				memcpy(&tilePos, constDatData + i+2, 2);
				memcpy(&tileCount, constDatData + i+4, 2);
			}
			i+=4;
		}
		i+=2;
	}

	layer1Tile tile1;
	layer3Tile tile4;
	paramTile tile3;
	quint32 size, tileID=0;

	size = (debut2-debut1)/8;
	tileID += size;

	size = (debut4-debut3)/14;
	layerExists[0] = size > 0;
	for(i=0 ; i<size ; ++i)
	{
		memcpy(&tile1, constDatData + debut3+i*14, 8);
		if(qAbs(tile1.cibleX) < 1000 || qAbs(tile1.cibleY) < 1000) {
			memcpy(&tile3, constDatData + debut3+i*14+10, 4);
			if(tile3.param)
			{
				usedParams.insert(tile3.param, usedParams.value(tile3.param) | tile3.state);
			}
		}
		++tileID;
	}

	layerID = 2;
	layerExists[1] = layerExists[2] = false;

	size = (datDataSize-debut4)/10;
	for(i=0 ; i<size ; ++i)
	{
		memcpy(&tile1, constDatData + debut4+i*10, 8);
		if(qAbs(tile1.cibleX) < 1000 || qAbs(tile1.cibleY) < 1000) {
			memcpy(&tile4, constDatData + debut4+i*10+8, 2);
			if(tile4.param)
			{
				usedParams.insert(tile4.param, usedParams.value(tile4.param) | tile4.state);
			}

			if(layerID+1<nbTilesLayer.size() && tileID>=nbTilesLayer.at(layerID)) {
				++layerID;
			}

			if(layerID-1 < 3)
				layerExists[layerID-1] = true;
		}
		++tileID;
	}

	return true;
}
