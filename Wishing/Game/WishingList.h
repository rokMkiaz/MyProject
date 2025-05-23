#pragma once
#include "../server_Common/TIStructServer.h"
#include "../server_Common/TIStruct.h"
#include "../server_Common/TIDefineServer.h"
#include "../server_Common/TIprotocolServer.h"
#include "../server_Common/TIprotocol.h"

class CUnit;
class CUnitPC;
struct stWishingStoneBuffData;

class CWishingList
{
public:
	CWishingList()
	{ 
		Init();
	}
	~CWishingList() {}

	void Init();
public:
	void SaveWishingData(CUnitPC* pPC,INT32 i32TitleIndex, INT32 i32StoneIndex);
	void SaveAllWishingData(CUnitPC* pPC);
	void UpdateWishingAbility(CUnitPC* pPC,BOOL bSend = FALSE);
	INT32 CheckLinkAbility(INT32 i32TitleIndex);
public:
	stWishingStoneBuffData* GetWishingStone(INT32 index,INT32 i32StoneIndex) { return &_stone[index][i32StoneIndex]; }
	void SetStoneData(stWishingStoneBuffData* Data, INT32 index, INT32 i32StoneIndex) { memcpy(&_stone[index][i32StoneIndex], Data, sizeof(stWishingStoneBuffData)); }

	INT32 GetWishingCount(INT32 i32TitleIndex) { return _i32WishingCount[i32TitleIndex]; }
	void SetWishingCount(INT32 i32TitleIndex, INT32 i32Value) { _i32WishingCount[i32TitleIndex] = i32Value; }

	INT32 GetWishingLinkeLevel(INT32 i32Title) { return (INT32)_wWishingLinkLevel[i32Title]; }

public:
	void ON_LOAD_WISHING_DATA(stWishingStoneBuffData* Data,INT32 i32index )	{	memcpy(_stone[i32index], Data, sizeof(stWishingStoneBuffData) * MAX_WISHING_STONE_COUNT);	}
	void ON_LOAD_WISHING_COUNT(INT32 Data, INT32 i32index)
	{	
		_i32WishingCount[i32index] = Data;
	}
private:
	INT32 _i32WishingCount[MAX_NECESSARY_TITLE];	//¿°¿ø µµÀüÈ½¼ö
	BYTE _wWishingLinkLevel[MAX_NECESSARY_TITLE];   
	stWishingStoneBuffData _stone[MAX_NECESSARY_TITLE][MAX_WISHING_STONE_COUNT];
};