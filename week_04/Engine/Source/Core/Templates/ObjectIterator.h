/*****************************************************************//**
 * \file   ObjectIterator.h
 * \brief  
 *         
 * 사용법 예시
 * 
 * for (TObjectIterator<UStaticMesh> It; It; ++It)
 * {
 *		UStaticMesh* StaticMesh = *It;
 *		if (StaticMesh->GetAssetPathFileName() == PathFileName)
 *		return It;
 * }
 * 
 * \author jungle
 * \date   March 2026
 *********************************************************************/

#pragma once
#include "Object/Object.h"


template<typename TObject>
class TObjectIterator
{
public:
	TObjectIterator():
		CurrentIndex(0)
	{
		AdvanceToNextValidObject();
	}

	TObject* operator*()
	{
		return static_cast<TObject*>(GUObjectArray[CurrentIndex]);
	}

	TObjectIterator& operator++()
	{
		++CurrentIndex;
		AdvanceToNextValidObject();
		return *this;
	}

	operator bool() const
	{
		return CurrentIndex < GUObjectArray.size();
	}
private:
	int32 CurrentIndex;

	void AdvanceToNextValidObject()
	{
		const int32 Count = GUObjectArray.size();
		while (CurrentIndex < Count)
		{
			UObject* Obj = GUObjectArray[CurrentIndex];
			
			if (Obj && Obj->IsA(TObject::StaticClass()))
				break;

			CurrentIndex++;
		}
	}
};