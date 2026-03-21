#include "Engine/Fonts/FontCache.h"

FFontCache::FFontCache()
{
}

FFontCache::~FFontCache()
{
}

void FFontCache::Initialize()
{
	int indexX = 0;
	int indexY = 0;

	// ASCII
	for (char i = 33; i <= 126; i++) {
		FCharacterInfo cInfo;
		cInfo.StartU = indexX / FontData.BitmapWidth;
		cInfo.StartV = indexY / FontData.BitmapHeight;
		cInfo.USize = FontData.CellWidth / FontData.BitmapWidth;
		cInfo.VSize = FontData.CellHeight / FontData.BitmapHeight;

		CharInfoMap.emplace(i, cInfo);

		indexX += FontData.CellWidth;
		if (indexX >= FontData.BitmapWidth) {
			indexX = 0;
			indexY += FontData.CellHeight;
		}
	}

	// 한국어
}

FCharacterInfo FFontCache::GetCharacterAtlasData(TCHAR c)
{
	return CharInfoMap[c];
}

void FFontCache::SetFontData()
{
	// Atlas Meta Data
	// 웹에서 제작해서 하드코딩
	FAtlasData data;
	data.BitmapWidth = 512;
	data.BitmapHeight = 512;
	data.CellsPerRow = 16;
	data.CellsperColumn = 16;
	data.CellWidth = 32;
	data.CellHeight = 32;

	data.FontSize = 15;
	data.OffsetX = 0;
	data.OffsetY = 0;

	FontData = data;
}
