#pragma once
#include "../server_Common/TIStructServer.h"
#include <vector>
#include <map>
#include <set>

#include"CSVFile_1003bM_Wishing.h"
#include"CSVFile_1003bM_Wishing_link.h"


class CUnitPC;

struct stWishingOption
{
	INT32 i32Option = 0;
	INT32 i32Value = 0;
	INT32 i32Grade = 0;
};

struct stWishingData
{
	INT32 i32CSVindex = 0;
	INT32 i32NecessaryTitle = 0;
	INT32 i32StoneType = 0;
	INT32 i32UseItemIndex = 0;
	INT32 i32MinPercent = 0;
	INT32 i32MaxPercent = 0;
};

struct stWishingUniqueData
{
	INT32 i32CSVindex = 0;
	INT32 i32NecessaryTitle = 0;
	INT32 i32StoneType = 0;
	INT32 i32UseRareItemIndex = 0;
	INT32 i32UniqueMinPercent = 0;
	INT32 i32UniqueMaxPercent = 0;
};


class CWishingManager
{
public:
	CWishingManager();
	~CWishingManager() {};
	
	void LoadData();
	
	void ON_CM_WISHING_INFO(CUnitPC* pPC, INT32 i32NecessaryTitle);
	void ON_CM_WISHING_CHANGE(CUnitPC* pPC, INT32 i32NecessaryTitle, INT32 i32StoneType);
	void ON_CM_UNIQUE_WISHING_CHANGE(CUnitPC* pPC, INT32 i32NecessaryTitle, INT32 i32StoneType);

	//잠금처리
	void ON_CM_WISHING_STONE_LOCK(CUnitPC* pPC, INT32 i32NecessaryTitle, INT32 i32StoneType, INT32 i32LockType);
	//천장교환
	void ON_CM_WISHING_ITEM_CHANGE(CUnitPC* pPC, INT32 i32NecessaryTitle);
	
public:
	stWishingOption* FindBuffData(INT32 i32CSVIndex);
	_CSV_WISHING_LINK_DATA* FindLinkBuffData(INT32 i32NecessaryTitle, INT32 i32Grade);
private:
	void CheckReset(CUnitPC* pPC, INT32 i32NecessaryTitle);


	vector<stWishingData*> FindWishingData(INT32 i32NecessaryTitle, INT32 i32StoneType);
	vector<stWishingUniqueData*> FindUniqueWishingData(INT32 i32NecessaryTitle, INT32 i32StoneType);

	stWishingData* ConfirmedOptionData(INT32 i32NecessaryTitle, INT32 i32StoneType);
	stWishingUniqueData* ConfirmedUniqueOptionData(INT32 i32NecessaryTitle, INT32 i32StoneType);

private:
	map<INT32, stWishingOption*> _mapOptionData; 

	map<INT32, map<INT32,vector<stWishingData*>>> _mapWishingData; //명호, 비석, 확률
	map<INT32, map<INT32,vector<stWishingUniqueData*>>> _mapWishingUniqueData; //명호, 비석, 확률

	map<INT32, vector<_CSV_WISHING_LINK_DATA*>> _mapWishingLinkData;
};

extern CWishingManager g_WishingManager;

