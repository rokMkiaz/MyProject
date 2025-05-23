#include "StdAfx.h"

#include "UnitPC.h"
#include "Unit.h"
#include "WishingList.h"
#include "WishingManager.h"

#include "GameManager.h"
#include "Math.h"
#include "CastleBattleManager.h"
#include "ItemManager.h"
#include <time.h>




void CWishingList::Init()
{
	memset(_stone, 0x00, sizeof(stWishingStoneBuffData) * MAX_NECESSARY_TITLE * MAX_WISHING_STONE_COUNT);
	memset(_i32WishingCount, 0x00, sizeof(INT32) * MAX_NECESSARY_TITLE);
	memset(_wWishingLinkLevel, 0x00, sizeof(BYTE) * MAX_NECESSARY_TITLE);
}

void CWishingList::SaveWishingData(CUnitPC* pPC,INT32 i32TitleIndex, INT32 i32StoneIndex)
{
	if (pPC == NULL)
	{
		return;
	}
	DCP_WISHING_SAVE_DATA sendDB;
	sendDB._dwcharunique = pPC->GetCharacterUnique();
	sendDB.i32NecessaryTitleNum = i32TitleIndex;
	sendDB.i32StoneIndex = i32StoneIndex;
	memcpy(&sendDB._stonebuffData, &_stone[i32TitleIndex][i32StoneIndex], sizeof(stWishingStoneBuffData));


	pPC->WriteGameDB((BYTE*)&sendDB, sizeof(DCP_WISHING_SAVE_DATA));
}

void CWishingList::SaveAllWishingData(CUnitPC* pPC)
{
	if (pPC == NULL)
	{
		return;
	}

	DCP_WISHING_SAVE_DATA sendDB;
	sendDB._dwcharunique = pPC->GetCharacterUnique();

	for (INT32 i32I = 0; i32I < MAX_NECESSARY_TITLE; i32I++)
	{
		for (INT32 i32J = 0; i32J < MAX_WISHING_STONE_COUNT; i32J++)
		{
			sendDB.i32NecessaryTitleNum = i32I;
			sendDB.i32StoneIndex = i32J;
			memcpy(&sendDB._stonebuffData, &_stone[i32I][i32J], sizeof(stWishingStoneBuffData));

			pPC->WriteGameDB((BYTE*)&sendDB, sizeof(DCP_WISHING_SAVE_DATA));
		}
	}

}

void CWishingList::UpdateWishingAbility(CUnitPC* pPC,BOOL bSend)
{
	if (pPC == NULL)
	{
		return;
	}
	memset(_wWishingLinkLevel, 0x00, sizeof(BYTE) * MAX_NECESSARY_TITLE);

	memset(pPC->m_Add_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], 0x00, sizeof(INT32) * Add_Ability_Pos::AAP_ADD_MAX);
	memset(pPC->m_Increase_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], 0x00, sizeof(INT32) * Increase_Ability_Pos::IAP_INCREASE_MAX);
	for (INT32 i = 0; i < IAP_INCREASE_MAX; i++)
	{
		pPC->_i32Decrease_Ability[ABILITY_TYPE_WISHING][i].clear();
	}


	INT32 i32LowLevel = 0;
	for (int i = 0; i < MAX_NECESSARY_TITLE; i++)
	{
		if (pPC->GetTitleIndex() < i)
		{
			break;
		}
		for (int j = 0; j < MAX_WISHING_STONE_COUNT; j++)
		{
			for (int k = 0; k < MAX_WISHING_STONE_OPTION; k++)
			{
				auto StoneOption = g_WishingManager.FindBuffData(_stone[i][j].GetCSVIndex(k));
				if (StoneOption == NULL)
				{
					continue;
				}


				//g_ItemManager.GetAbility(pPC->m_Add_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], pPC->m_Increase_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], pPC->_i32Decrease_Ability[ABILITY_TYPE_WISHING],
				//	_stone[i][j].GetStoneOption(k), _stone[i][j].GetStoneOptionValue(k));

				g_ItemManager.GetAbility(pPC->m_Add_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], pPC->m_Increase_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], pPC->_i32Decrease_Ability[ABILITY_TYPE_WISHING],
					StoneOption->i32Option, StoneOption->i32Value);


			}
		

			i32LowLevel = _stone[i][j].GetGrade();
			if (i32LowLevel == 0  || _stone[i][j].GetNextFlag() ==FALSE) //등급이 없다
			{
				_wWishingLinkLevel[i] = 0;
				j = MAX_WISHING_STONE_COUNT;
				continue;
			}

			if (j == 0)
			{
				_wWishingLinkLevel[i] = i32LowLevel;
			}
			if (_wWishingLinkLevel[i] > i32LowLevel)
			{
				_wWishingLinkLevel[i] = i32LowLevel;
			}
		}
	}
	for (int i = 0; i < MAX_NECESSARY_TITLE; i++) //LINK 버프
	{
		auto pFindLinkData = g_WishingManager.FindLinkBuffData(i, _wWishingLinkLevel[i]);
		if (pFindLinkData == NULL)
		{
			continue;
		}
		for (int j = 0; j < MAX_WISHING_STONE_OPTION; j++)
		{
			g_ItemManager.GetAbility(pPC->m_Add_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], pPC->m_Increase_Ability[ABILITY_TYPE::ABILITY_TYPE_WISHING], pPC->_i32Decrease_Ability[ABILITY_TYPE_WISHING],
				pFindLinkData->i32BuffOption[j], pFindLinkData->i32BuffValue[j]);
		}
	}


	if (bSend)
	{
		SP_CharStat sendbalanceInfo;
		pPC->GetBalanceInfo(sendbalanceInfo.balanceInfo);
		pPC->Write((BYTE*)&sendbalanceInfo, sizeof(SP_CharStat));
	}


}

INT32 CWishingList::CheckLinkAbility(INT32 i32TitleIndex)
{
	if (i32TitleIndex < 1 && i32TitleIndex >= MAX_NECESSARY_TITLE)
		return FALSE;

	INT32 i32LinkLevel = GetWishingCount(i32TitleIndex);

	for (int j = 0; j < MAX_WISHING_STONE_COUNT; j++)
	{
		if (GetWishingStone(i32TitleIndex, j)->GetNextFlag() == FALSE) //하나라도 안되어있으면 FALSE;
		{
			i32LinkLevel = FALSE;
		}
	}
	

	return i32LinkLevel;
}

