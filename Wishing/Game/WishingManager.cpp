#include "StdAfx.h"
#include "../server_Common/DebugManager.h"
#include "../server_Common/INIFile_Server.h"
#include "../server_Common/CSVFile_Item.h"
#include "../server_Common/CSVFile_Shop.h"


#include "Unit.h"
#include "UnitPC.h"
#include "GameManager.h"
#include "WishingManager.h"
#include "CSVFile_1003bM_Wishing.h"
#include "CSVFile_1003bM_Wishing_link.h"

CWishingManager g_WishingManager;

CWishingManager::CWishingManager()
{
}

void CWishingManager::LoadData()
{
	_CSV_WISHING_DATA* pWishingData = g_CSVFile_1003bM_Wishing.GetFirstRecordData();

	INT32 i32StoneType = 0;
	INT32 i32MinPercent = 0;
	INT32 i32MinUniquePercent = 0;

	while (pWishingData)
	{
		stWishingData* WishingData = new stWishingData();
		stWishingOption* WishingOption = new stWishingOption();
		if (i32StoneType != pWishingData->i32StoneType)
		{
			i32StoneType = pWishingData->i32StoneType;
			i32MinPercent = 0;
			i32MinUniquePercent = 0;
		}

		WishingData->i32CSVindex = pWishingData->i32Index;
		WishingData->i32NecessaryTitle = pWishingData->i32NecessaryTitle;
		WishingData->i32StoneType = pWishingData->i32StoneType;



		WishingData->i32UseItemIndex = DEFAULT_WISHING_ITEM;
		WishingData->i32MinPercent = i32MinPercent;
		WishingData->i32MaxPercent = i32MinPercent + pWishingData->i32Percent;

		_mapWishingData[pWishingData->i32NecessaryTitle][pWishingData->i32StoneType].push_back(WishingData);

		WishingOption->i32Option = pWishingData->i32Option;
		WishingOption->i32Value = pWishingData->i32Value;
		WishingOption->i32Grade = pWishingData->i32OptionGrade;

		if (pWishingData->i32UniquePercent != 0)
		{
			stWishingUniqueData* WishingUniqueData = new stWishingUniqueData();

			WishingUniqueData->i32CSVindex = pWishingData->i32Index;
			WishingUniqueData->i32NecessaryTitle = pWishingData->i32NecessaryTitle;
			WishingUniqueData->i32StoneType = pWishingData->i32StoneType;


			WishingUniqueData->i32UseRareItemIndex = pWishingData->i32UseRareItemIndex;
			WishingUniqueData->i32UniqueMinPercent = i32MinUniquePercent;
			WishingUniqueData->i32UniqueMaxPercent = i32MinUniquePercent+ pWishingData->i32UniquePercent;

			_mapWishingUniqueData[pWishingData->i32NecessaryTitle][pWishingData->i32StoneType].push_back(WishingUniqueData);
		}

		i32MinPercent += pWishingData->i32Percent;	//확률중첩
		i32MinUniquePercent += pWishingData->i32UniquePercent;

		_mapOptionData.insert(make_pair(pWishingData->i32Index, WishingOption));


		pWishingData = g_CSVFile_1003bM_Wishing.GetNextRecordData();
	}
	g_CSVFile_1003bM_Wishing.Clear();


	_CSV_WISHING_LINK_DATA* pWishingLinkData = g_CSVFile_1003bM_Wishing_Link.GetFirstRecordData();//23.04.03_남일우_무한의 탑

	while (pWishingLinkData)
	{

		_CSV_WISHING_LINK_DATA* pData = new _CSV_WISHING_LINK_DATA();

		*pData = *pWishingLinkData;

		_mapWishingLinkData[pData->i32NecessaryTitle].push_back(pData);


		pWishingLinkData = g_CSVFile_1003bM_Wishing_Link.GetNextRecordData();
	}
	g_CSVFile_1003bM_Wishing_Link.Clear();


}


void CWishingManager::ON_CM_WISHING_INFO(CUnitPC* pPC, INT32 i32NecessaryTitle)
{
	if (pPC == NULL)
	{
		return;
	}
	if (i32NecessaryTitle < 1 || i32NecessaryTitle >= MAX_NECESSARY_TITLE)
	{
		return;
	}

	SP_WISHING_INFO sendMsg;
	memset(sendMsg._stoneData, 0x00, sizeof(stWishingStoneInfo) * MAX_WISHING_STONE_COUNT);

	BOOL bActive = TRUE;
	for (int i = 0; i < MAX_WISHING_STONE_COUNT; i++)
	{
		for (int j = 0; j < MAX_WISHING_STONE_OPTION; j++)
		{
			sendMsg._stoneData[i].i32StoneCsvIndex[j] = pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i)->GetCSVIndex(j);

		}
		sendMsg._stoneData[i].i32Lock = pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i)->GetLock();

		sendMsg._stoneData[i].bActive = bActive;
		if (pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i)->GetNextFlag() == FALSE)
		{
			bActive = FALSE;
		}
	}
	sendMsg.i32ChallengeCount = pPC->GetWishingList()->GetWishingCount(i32NecessaryTitle);
	sendMsg.i32LinkGrade = pPC->GetWishingList()->GetWishingLinkeLevel(i32NecessaryTitle);
	sendMsg.i32NecessaryTitle = i32NecessaryTitle;

	pPC->Write((BYTE*)&sendMsg,sizeof(SP_WISHING_INFO));

}


void CWishingManager::ON_CM_WISHING_CHANGE(CUnitPC* pPC, INT32 i32NecessaryTitle, INT32 i32StoneType)
{
	if (pPC == NULL)
	{
		return;
	}

	SP_WISHING_CHANGE_RESULT sendMsg;
	sendMsg.i32Result = 1;
	sendMsg.i32NecessaryTitle = i32NecessaryTitle;
	sendMsg.i32StoneType = i32StoneType;
	memset(&sendMsg._stoneData, 0x00, sizeof(stWishingStoneInfo));

	if (i32NecessaryTitle < 1 || i32NecessaryTitle >= MAX_NECESSARY_TITLE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg,sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	//명호 검사
	if (pPC->GetTitleIndex() < i32NecessaryTitle)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	// 워프중
	if (pPC->m_bWARP_PENDING == TRUE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	//죽은상태
	if (pPC->GetAlive() == FALSE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_DIE;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	auto CheckWishing = ConfirmedOptionData(i32NecessaryTitle, i32StoneType);
	if (CheckWishing == NULL)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	//아이템 갯수 모자름
	auto pItem = pPC->FindInventory(CheckWishing->i32UseItemIndex);
	if (pItem == NULL)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_USEITEM_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	//이전 단계 명호각인이 열렸는지 검사
	if (i32StoneType > 0 && i32StoneType< MAX_WISHING_STONE_COUNT)
	{
		auto pFindUserStoneData = pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i32StoneType-1);
		if (pFindUserStoneData != NULL)
		{
			if (pFindUserStoneData->GetNextFlag() == FALSE)
			{
				sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
				pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
				return;
			}
		}
	}

	auto pFindUserStoneData = pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i32StoneType);
	if (pFindUserStoneData == NULL)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	if (pFindUserStoneData->GetLock() == TRUE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_LOCK;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	if (TRUE == pItem->decrease(pPC, 1, true))
	{
		stUseLog uselog;
		uselog.m_eType = ITEM_USE_LOG_TYPE::USE_WISHING_ITEM;
		uselog.m_dwBeforItemCnt = pItem->GetItemCount() + 1;
		CLogManager::ITEM_USE(pPC, pItem, uselog);
		pPC->RequestAddLimitProgress(ENUM_CASH_LIMIT_TYPE::ENUM_CASH_LIMIT_WISHING, 1); //24.05.17_윤의영_기간한정패키지 진행도 추가
	}
	else
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_USEITEM_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	if (pItem->GetItemCount() <= 0)
	{
		SP_Item_Del senddel;
		senddel.i64ItemUnique = pItem->GetItemUnique();
		senddel.iItemIndex = pItem->GetItemSaveInfo()->_i32Index;
		senddel.iResult = 0;
		senddel.iType = 0;
		pPC->Write((BYTE*)&senddel, sizeof(SP_Item_Del));

		stDel deldata;
		deldata.m_eType = ITEM_DELETE_TYPE::DEL_WISHING_ITEM;
		pPC->DeleteInventoryItem(deldata);
	}





	INT32 i32LowGrade = 0;
	INT32 i32IndexCheck = 0;
	BOOL i32NextFlag = TRUE;
	for (int i = 0; i < MAX_WISHING_STONE_OPTION; i++)
	{
		auto pOption = ConfirmedOptionData(i32NecessaryTitle, i32StoneType);
		if (pOption == NULL)
		{
			break;
		}
		auto pState = FindBuffData(pOption->i32CSVindex);
		if(pState==NULL)
		{
			break;
		}

		sendMsg._stoneData.i32StoneCsvIndex[i] = pOption->i32CSVindex;

		pFindUserStoneData->SetCSVIndex(i, pOption->i32CSVindex);
		//pFindUserStoneData->SetStoneOption(i, pOption->i32Option);
		//pFindUserStoneData->SetStoneOptionValue(i, pOption->i32Value);

		if (i == 0)
		{
			i32LowGrade = pState->i32Grade;
			i32IndexCheck = pState->i32Option;
		}

		if (i32LowGrade > pState->i32Grade)
		{
			i32LowGrade = pState->i32Grade;
		}

		if (i32IndexCheck != pState->i32Option)
		{
			i32NextFlag = FALSE;
		}
	}
	pFindUserStoneData->SetGrade(i32LowGrade);
	pFindUserStoneData->SetNextFlag(i32NextFlag);

	INT32 i32WishingCount = pPC->GetWishingList()->GetWishingCount(i32NecessaryTitle) + 1;
	pPC->GetWishingList()->SetWishingCount(i32NecessaryTitle, i32WishingCount );


	CLogManager::WISHING_CHANGE_LOG(pPC,i32NecessaryTitle,i32StoneType,sendMsg._stoneData.i32StoneCsvIndex, i32WishingCount, FALSE,i32NextFlag);

	sendMsg._stoneData.i32Lock = pFindUserStoneData->GetLock();


	sendMsg.i32Result = ENUM_ALL_ERROR::SUCCESS;
	pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));


	//이전명호 각인시 삭제처리
	CheckReset(pPC,i32NecessaryTitle);
	pPC->WishingAbilityUpdate(true);
	//pPC->AbilityUpdate(true);

	pPC->GetWishingList()->SaveWishingData(pPC, i32NecessaryTitle, i32StoneType);
	ON_CM_WISHING_INFO(pPC, i32NecessaryTitle);
}

void CWishingManager::ON_CM_UNIQUE_WISHING_CHANGE(CUnitPC* pPC, INT32 i32NecessaryTitle, INT32 i32StoneType)
{
	if (pPC == NULL)
	{
		return;
	}

	SP_WISHING_CHANGE_RESULT sendMsg;
	sendMsg.i32Result = 1;
	sendMsg.i32NecessaryTitle = i32NecessaryTitle;
	sendMsg.i32StoneType = i32StoneType;
	memset(&sendMsg._stoneData, 0x00, sizeof(stWishingStoneInfo));

	if (i32NecessaryTitle < 1 || i32NecessaryTitle >= MAX_NECESSARY_TITLE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	//명호 검사
	if (pPC->GetTitleIndex() < i32NecessaryTitle)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	// 워프중
	if (pPC->m_bWARP_PENDING == TRUE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	//죽은상태
	if (pPC->GetAlive() == FALSE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_DIE;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	auto CheckWishing = ConfirmedUniqueOptionData(i32NecessaryTitle, i32StoneType);
	if (CheckWishing == NULL)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	//아이템 갯수 모자름
	auto pItem = pPC->FindInventory(CheckWishing->i32UseRareItemIndex);
	if (pItem == NULL)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_USEITEM_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	//이전 단계 명호각인이 열렸는지 검사
	if (i32StoneType > 0 && i32StoneType < MAX_WISHING_STONE_COUNT)
	{
		auto pFindUserStoneData = pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i32StoneType - 1);
		if (pFindUserStoneData != NULL)
		{
			if (pFindUserStoneData->GetNextFlag() == FALSE)
			{
				sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
				pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
				return;
			}
		}
	}

	auto pFindUserStoneData = pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i32StoneType);
	if (pFindUserStoneData == NULL)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_TITLE_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}
	if (pFindUserStoneData->GetLock() == TRUE)
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_WISHING_LOCK;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	if (TRUE == pItem->decrease(pPC, 1, true))
	{
		stUseLog uselog;
		uselog.m_eType = ITEM_USE_LOG_TYPE::USE_WISHING_ITEM;
		uselog.m_dwBeforItemCnt = pItem->GetItemCount() + 1;
		CLogManager::ITEM_USE(pPC, pItem, uselog);
	}
	else
	{
		sendMsg.i32Result = ENUM_ALL_ERROR::ENUM_ALL_ERROR_USEITEM_ERROR;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));
		return;
	}

	if (pItem->GetItemCount() <= 0)
	{
		SP_Item_Del senddel;
		senddel.i64ItemUnique = pItem->GetItemUnique();
		senddel.iItemIndex = pItem->GetItemSaveInfo()->_i32Index;
		senddel.iResult = 0;
		senddel.iType = 0;
		pPC->Write((BYTE*)&senddel, sizeof(SP_Item_Del));

		stDel deldata;
		deldata.m_eType = ITEM_DELETE_TYPE::DEL_WISHING_ITEM;
		pPC->DeleteInventoryItem(deldata);
	}





	INT32 i32LowGrade = 0;
	INT32 i32IndexCheck = 0;
	BOOL i32NextFlag = TRUE;
	for (int i = 0; i < MAX_WISHING_STONE_OPTION; i++)
	{
		auto pOption = ConfirmedUniqueOptionData(i32NecessaryTitle, i32StoneType);
		if (pOption == NULL)
		{
			break;
		}
		auto pState = FindBuffData(pOption->i32CSVindex);
		if (pState == NULL)
		{
			break;
		}

		sendMsg._stoneData.i32StoneCsvIndex[i] = pOption->i32CSVindex;

		pFindUserStoneData->SetCSVIndex(i, pOption->i32CSVindex);
		//pFindUserStoneData->SetStoneOption(i, pOption->i32Option);
		//pFindUserStoneData->SetStoneOptionValue(i, pOption->i32Value);

		if (i == 0)
		{
			i32LowGrade = pState->i32Grade;
			i32IndexCheck = pState->i32Option;
		}

		if (i32LowGrade > pState->i32Grade)
		{
			i32LowGrade = pState->i32Grade;
		}

		if (i32IndexCheck != pState->i32Option)
		{
			i32NextFlag = FALSE;
		}
	}
	pFindUserStoneData->SetGrade(i32LowGrade);
	pFindUserStoneData->SetNextFlag(i32NextFlag);

	// 진염원구 사용할 때는 천장카운트 x
	INT32 i32WishingCount = pPC->GetWishingList()->GetWishingCount(i32NecessaryTitle) ;
	//pPC->GetWishingList()->SetWishingCount(i32NecessaryTitle, i32WishingCount);


	CLogManager::WISHING_CHANGE_LOG(pPC, i32NecessaryTitle, i32StoneType, sendMsg._stoneData.i32StoneCsvIndex, i32WishingCount,TRUE, i32NextFlag);

	sendMsg._stoneData.i32Lock = pFindUserStoneData->GetLock();


	sendMsg.i32Result = ENUM_ALL_ERROR::SUCCESS;
	pPC->Write((BYTE*)&sendMsg, sizeof(SP_WISHING_CHANGE_RESULT));


	//이전명호 각인시 삭제처리
	CheckReset(pPC, i32NecessaryTitle);
	pPC->WishingAbilityUpdate(true);
	//pPC->AbilityUpdate(true);

	pPC->GetWishingList()->SaveWishingData(pPC, i32NecessaryTitle, i32StoneType);
	ON_CM_WISHING_INFO(pPC, i32NecessaryTitle);

}

void CWishingManager::ON_CM_WISHING_STONE_LOCK(CUnitPC* pPC, INT32 i32NecessaryTitle, INT32 i32StoneType,INT32 i32LockType)
{
	if (pPC == NULL)
	{
		return;
	}

	if (i32LockType != 0 && i32LockType != 1)
	{
		ON_CM_WISHING_INFO(pPC, i32NecessaryTitle);
		return;
	}


	pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i32StoneType)->SetLock(i32LockType);

	ON_CM_WISHING_INFO(pPC, i32NecessaryTitle);
	CLogManager::WISHING_LOCK_LOG(pPC, i32NecessaryTitle, i32StoneType, i32LockType);
}

void CWishingManager::ON_CM_WISHING_ITEM_CHANGE(CUnitPC* pPC, INT32 i32NecessaryTitle)
{
	if (pPC == NULL)
		return;
	if (pPC->GetWishingList()->GetWishingCount(i32NecessaryTitle) < CHANGE_WISHING_ITEM)
		return;

	auto pFindData = FindUniqueWishingData(i32NecessaryTitle, 0).at(0);
	if (pFindData == NULL)
		return;

	INT32 i32Count = 0;
	i32Count = floor(pPC->GetWishingList()->GetWishingCount(i32NecessaryTitle) / (DOUBLE)CHANGE_WISHING_ITEM);
	
	INT32 ResultCount = pPC->GetWishingList()->GetWishingCount(i32NecessaryTitle) - (CHANGE_WISHING_ITEM * i32Count);
	pPC->GetWishingList()->SetWishingCount(i32NecessaryTitle, ResultCount);

	g_GameManager.InputMail(pPC->GetCharacterUnique(), pPC->GetAccountUnique(), g_DataManager.GetLocaliZation(ENUM_LOCALIZATION_NUM::ENUM_LOCALIZATION_34),  g_DataManager.GetLocaliZation(ENUM_LOCALIZATION_NUM::ENUM_LOCALIZATION_33),
		3, pFindData->i32UseRareItemIndex, i32Count);

	CLogManager::WISHING_ITEM_CHANGE(pPC,i32NecessaryTitle,pFindData->i32UseRareItemIndex,i32Count, pPC->GetWishingList()->GetWishingCount(i32NecessaryTitle));
	ON_CM_WISHING_INFO(pPC, i32NecessaryTitle);
}

void CWishingManager::CheckReset(CUnitPC* pPC, INT32 i32NecessaryTitle)
{
	if (pPC == NULL)
		return;

	INT32 i32DeleteFlag = FALSE;
	for (int i = 0; i < MAX_WISHING_STONE_COUNT; i++)
	{
		if(pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle,i)->GetNextFlag() == FALSE)
		{
			i32DeleteFlag = TRUE;
		}
		if (i32DeleteFlag == TRUE && i+1 < MAX_WISHING_STONE_COUNT)
		{
			//pPC->GetWishingList()->GetWishingStone(i32NecessaryTitle, i+1)->Init();
			//CLogManager::WISHING_INIT_LOG(pPC, i32NecessaryTitle,i+1);
		}
	}


}

stWishingOption* CWishingManager::FindBuffData(INT32 i32CSVIndex)
{
	if (_mapOptionData.find(i32CSVIndex) == _mapOptionData.end())
	{
		return NULL;
	}

	return _mapOptionData.find(i32CSVIndex)->second;
}

_CSV_WISHING_LINK_DATA* CWishingManager::FindLinkBuffData(INT32 i32NecessaryTitle, INT32 i32Grade)
{
	auto finditer = _mapWishingLinkData.find(i32NecessaryTitle);
	if (finditer == _mapWishingLinkData.end())
	{
		return NULL;
	}

	for (auto iter = finditer->second.begin(); iter != finditer->second.end(); ++iter)
	{
		if ((*iter)->i32Grade == i32Grade)
		{
			return (*iter);
		}
	}


	return NULL;
}

vector<stWishingData*> CWishingManager::FindWishingData(INT32 i32NecessaryTitle, INT32 i32StoneType)
{
	auto finditer = _mapWishingData.find(i32NecessaryTitle);
	if (finditer == _mapWishingData.end())
	{
		return vector<stWishingData*>();
	}

	auto stonefind = finditer->second.find(i32StoneType);
	if (stonefind == finditer->second.end())
	{
		return vector<stWishingData*>();
	}


	return stonefind->second;


}

vector<stWishingUniqueData*> CWishingManager::FindUniqueWishingData(INT32 i32NecessaryTitle, INT32 i32StoneType)
{
	auto finditer = _mapWishingUniqueData.find(i32NecessaryTitle);
	if (finditer == _mapWishingUniqueData.end())
	{
		return vector<stWishingUniqueData*>();
	}

	auto stonefind = finditer->second.find(i32StoneType);
	if (stonefind == finditer->second.end())
	{
		return vector<stWishingUniqueData*>();
	}


	return stonefind->second;
}

stWishingData* CWishingManager::ConfirmedOptionData(INT32 i32NecessaryTitle, INT32 i32StoneType)
{
	auto vecFindData = FindWishingData(i32NecessaryTitle, i32StoneType);
	if (vecFindData.begin() == vecFindData.end())
	{
		return NULL;
	}

	INT32 i32Random = g_GameManager.getRandomNumber(1, 100000000);


	auto iter = vecFindData.begin();
	for (; iter != vecFindData.end(); ++iter)
	{
		if (i32Random > (*iter)->i32MinPercent && i32Random <= (*iter)->i32MaxPercent)
		{
			return (*iter);
		}
	}

	return NULL;
}

stWishingUniqueData* CWishingManager::ConfirmedUniqueOptionData(INT32 i32NecessaryTitle, INT32 i32StoneType)
{
	auto vecFindData = FindUniqueWishingData(i32NecessaryTitle, i32StoneType);
	if (vecFindData.begin() == vecFindData.end())
	{
		return NULL;
	}

	INT32 i32Random = g_GameManager.getRandomNumber(1, 100000000);


	auto iter = vecFindData.begin();
	for (; iter != vecFindData.end(); ++iter)
	{
		if (i32Random > (*iter)->i32UniqueMinPercent && i32Random <= (*iter)->i32UniqueMaxPercent)
		{
			return (*iter);
		}
	}

	return NULL;
}


