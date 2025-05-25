#include "../server_Common/TIStruct.h"
#include "../server_Common/TIStructServer.h"
#include "../server_Common/TIDefineServer.h"
#include "../server_Common/TIDefine.h"
#include "../server_Common/TIprotocolServer.h"
#include "../server_Common/TIprotocol.h"
#include<map>
#include<chrono>
#include<vector>

#include "CSVFile_CastleInfo.h"
#include "CSVFile_CastleWarpInfo.h"


#include"UnitPC.h"
#include "NpcManager.h"
#include "GameManager.h"
#include "CastleManager.h"
#include "DungeonManager.h"
#include "BuffManager.h"
#include "GameMap.h"
#include "MapManager.h"
#include "DungeonBossSummonManager.h"
#include "DungeonManager.h"
#include "CSVFile_CastleInfo.h"
#include "CSVFile_CastleWarpInfo.h"
#include "CSVFile_CastleLevelInfo.h"
#include"Castle.h"



void CCastle::Init()
{
	_vecBattlePCInfo.clear();
	_vecDeclareList.clear();
	//_i32TeamUnique = 0;
	memset(&_SaveInfo,0x00,sizeof(stCastleSaveInfo));
	_SaveInfo._eState = CTS_LOADING;
	//memset(&_stCastleSchedule, 0x00, sizeof(stCastleScheduleInfo));

	_ui32NextStarttimestamp = 0;
	//_i32DeclareTick = 0;
	//_i32BattleReadyTick = 0;
	//_i64BattleStartTick = 0;
	//_i64BattleEndTick = 0;
	memset(&_LastBattleTime, 0x00, sizeof(DateTime));
	_i32MinDeclareCost = 0;
	_i32MaxDefenceCost = 0;
	
	for (auto finditer : _mmvCasterWarpInfo)
	{
		finditer.second.clear();
	}
	_mmvCasterWarpInfo.clear();
	_vecSummonList.clear();
	_i32BattleMapIndex = 0;
	bDBLoad = FALSE;
	bSchedule = FALSE;
	pMasterClan = NULL;
	_ui32EndBattleTimestamp = 0;
	_bFreeForAll = FALSE;
	ui32CastleBattleTimeTick = 0;

	_i32WinRewardIdx = 0;
	_i32LoseRwardIdx = 0;
	_mapLevelInfo.clear();
	i32CastleLevelStep = 0;
	_pBossNpc = NULL;
	_i32RewardDungeonMapIdx = 0;
	_i32GoldCostDungeonIdx = 0;
	_i32CastleDebuffPer = 0;
}

void CCastle::LoadData(_CSVCASTLE_DATA* pData)
{
	_SaveInfo._i32CastleIndex = pData->_i32Index;
	_eCastleType = (CASTLE_TYPE)pData->_i32Type;
	_i32BattleMapIndex = pData->_i32BattleMapIndex;
	_i32MinDeclareCost = pData->_i32MinDeclareCost;
	_i32MaxDefenceCost = pData->_i32MaxDefenceCost;

	_i32RewardDungeonMapIdx= pData->_i32RewardDungeonMapIndex;
	_i32WinRewardIdx = pData->_i32WinRewardIdx;
	_i32LoseRwardIdx = pData->_i32LoseRewardIdx;
	_i32GoldCostDungeonIdx = pData->_i32GoldSettingDungeonIdx;
}

void CCastle::LoadNPCInfo(_CSVCASTLE_NPC_DATA* pData)
{

	stSummonMonsterList NpcData;
	NpcData.Index = pData->_i32Index;
	NpcData.i32NpcIndex = pData->_i32NpcIndex;
	NpcData.i32Type = (CASTLE_BATTLE_NPC_TYPE)pData->_i32NpcType; //CASTLE_BATTLE_NPC_TYPE 요타입따라감
	NpcData.i32BuffIndex = pData->_i32BuffIdx;

	WCHAR* pTemp = NULL;
	pTemp = wcstok(pData->_wcPos, L"/");
	if (NULL == pTemp)
	{
		NpcData.i32X = 0;
	}
	else
	{
		NpcData.i32X = _wtoi(pTemp);
	}
	pTemp = wcstok(NULL, L"/");
	if (NULL == pTemp)
	{
		NpcData.i32Y = 0;
	}
	else
	{
		NpcData.i32Y = _wtoi(pTemp);
	}

	_vecSummonList.push_back(NpcData);
	


}

void CCastle::LoadWarpInfo(_CSV_CASTLE_WARP_DATA* pData)
{
	stCastleWarp WarpInfo;
	if (pData->_bEnter == FALSE)
		return;

	for (INT32 i = 0; i < MAX_WARP_CASTLE_POINT; i++)
	{

		WCHAR* pTemp = NULL;
		pTemp = wcstok(pData->_i32WarpPos[i], L"/");
		if (NULL == pTemp)
		{
			WarpInfo.i32X = 0;
		}
		else
		{
			WarpInfo.i32X = _wtoi(pTemp);
		}
		pTemp = wcstok(NULL, L"/");
		if (NULL == pTemp)
		{
			WarpInfo.i32Y = 0;
		}
		else
		{
			WarpInfo.i32Y = _wtoi(pTemp);
		}
		_mmvCasterWarpInfo[pData->_i32TeamType][pData->_i32Index].push_back(WarpInfo);

	}

}

void CCastle::LoadLevelInfo(_CSVCASTLE_LEVEL_DATA* pData)
{

	stCastleLevelInfo Data;
	Data.i32BossIndex = pData->_i32SummonBossIndex;
	Data.i32MinRelayWinCnt = pData->_i32RelayWinCount;
	Data.i32RewardMapIndex = pData->_i32RewardMapIndex;
	Data.i32MaxDrawItemCnt = pData->_i32MaxItemCount;

	WCHAR* pTemp = NULL;
	pTemp = wcstok(pData->_EnterXY, L"/");
	if (NULL == pTemp)
	{
		Data.i32EnterX = 0;
	}
	else
	{
		Data.i32EnterX = _wtoi(pTemp);
	}
	pTemp = wcstok(NULL, L"/");
	if (NULL == pTemp)
	{
		Data.i32EnterY = 0;
	}
	else
	{
		Data.i32EnterY = _wtoi(pTemp);
	}

	for (int i = 0; i < MAX_CASTLE_GOLD_DUNGEON_NPC_TYPE; i++)
	{
		Data.i32GoldDungeonSummonList[i] = pData->_i32GoldDungeonNPCIndex[i];
	}

	_mapLevelInfo.insert(make_pair(pData->_i32Step, Data));

}

void CCastle::LoadDB(INT32 i32Client)
{
	DCP_REQUEST_CASTLE_DATA sendmsg;
	sendmsg._i32CastleIndex = GetCastleIndex();

	if (GetDBLoad() == FALSE)
	{
		g_Server.SendMsg(i32Client, (BYTE*)&sendmsg, sizeof(DCP_REQUEST_CASTLE_DATA));
	}
}

void CCastle::SetCastleData(BYTE* pPacket)
{
	DSP_REPLY_CASTLE_DATA* pMsg = (DSP_REPLY_CASTLE_DATA*)pPacket;

	if (pMsg->_stSaveInfo._eState == CTS_LOADING)
	{
		_SaveInfo._eState = CTS_NONE;
	}
	else if (pMsg->_stSaveInfo._eState == CTS_READY_TEN_MIN || pMsg->_stSaveInfo._eState == CTS_BATTLE)
	{
		SummonNpcList();
	}


	memcpy(&_SaveInfo, &pMsg->_stSaveInfo, sizeof(stCastleSaveInfo));
	CClan* pClan = g_ClanManager.FindByClanUnique(_SaveInfo._ui32OwnerClanunique);
	if (pClan)
	{
		pMasterClan = pClan;
		memcpy(_SaveInfo._wcOwnerCharName, pClan->GetMasterName(), sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
	}
	else
	{
		pMasterClan = NULL;
	}
	SettingCastleStep();
	bDBLoad = TRUE;

	//셋팅끝나고 호출
	LoadCastleTimeDungeon();
	UpdateCastleCostDungeonSummonList();
}

void CCastle::SetSchedule(UINT32 ui32NextTimestamp)
{
	_ui32NextStarttimestamp = ui32NextTimestamp;
	bSchedule = TRUE;
}


void CCastle::EraseCastleBuff(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	CBuffManager* buffManager = CBuffManager::GetInstance();
	list<CBuff*>::iterator iter = pPC->GetBuff(SERVER_CASTLE_MASTER);
	if (iter != pPC->GetBuffList()->end())
	{
		buffManager->BroadCastBuffOff(pPC, *(iter));
		pPC->RemoveBuff(iter);
	}
}

ENUM_ERROR_MESSAGE CCastle::SettingDefenceCost(CUnitPC* pPC, INT64 i64Cost)
{
	if (pPC == NULL)
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;

	//SP_CASTLE_EDIT sendError;
	//sendError.i32CastleIndex = GetCastleIndex();
	//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	//sendError.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::DEFENCECOST_SETTING;

	if (pPC->GetClan() == NULL || GetMasterClan() == NULL || GetSaveInfo()->_ui32OwnerClanunique != pPC->GetClanUnique())
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}
	if (GetCastleState() != CTS_NONE)
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return  ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}

	if (pPC->GetClan()->GetMasterunique() != pPC->GetCharacterUnique())
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return  ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}

	if (pPC->GetClan()->GetClanMoney() < i64Cost || _SaveInfo.i32DefenceCost + i64Cost > _i32MaxDefenceCost)
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_MONEY_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_MONEY_NOT;
	}

	pPC->GetClan()->AddMoney(-i64Cost, pPC, CLAN_MONEY_GOLD_TYPE::CLAN_MONEY_CASTLE_SETTINGDEFENCECOST, pPC->GetCharacterUnique(), 0, GetCastleIndex(), i64Cost);
	
	return ENUM_ERROR_MESSAGE::SUCCESS;
}

void CCastle::DefenceCostCompleate(CUnitPC* pPC, INT64 i64Money)
{
	CLogManager::CASTLE_DEFENCECOST_LOG(GetCastleIndex(),pPC,i64Money, _SaveInfo.i32DefenceCost+ i64Money, _SaveInfo._ui32OwnerClanunique);
	_SaveInfo.i32DefenceCost += i64Money;

	if (pPC)
	{
		DCP_SIEGE_READY_DATA sendMsg;
		sendMsg.i32Charunique = pPC->GetCharacterUnique();
		sendMsg.i32Group = pPC->GetGroupId();
		sendMsg.i32CastleIndex = GetCastleIndex();
		pPC->WriteGameDB((BYTE*)&sendMsg, sizeof(DCP_SIEGE_READY_DATA));
	}
}

void CCastle::Declare(CUnitPC* pPC, INT64 i64DeclareCost)
{
	if (pPC == NULL)
		return;
	SP_CASTLE_INFO sendErrorMsg;
	sendErrorMsg.i32Type = (INT32)ENUM_CP_CASTLE_INFO_TYPE::DECLARE;
	sendErrorMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;

	CClan* pClan = pPC->GetClan();
	if (pClan == NULL)
	{
		sendErrorMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
		pPC->Write((BYTE*)&sendErrorMsg, sizeof(SP_CASTLE_INFO));
		return;
	}
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_READY)
	{
		sendErrorMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NOT_DECLARE;
		pPC->Write((BYTE*)&sendErrorMsg, sizeof(SP_CASTLE_INFO));
		return;
	}

	INT64 i64MinDeclareCost = (_SaveInfo.i32DefenceCost * 0.6f) + _i32MinDeclareCost;
	if (i64DeclareCost< i64MinDeclareCost || i64DeclareCost>_i32MaxDefenceCost)
	{
		sendErrorMsg.i32Result = 3;
		pPC->Write((BYTE*)&sendErrorMsg, sizeof(SP_CASTLE_INFO));
		return;
	}
	if (pClan == pMasterClan)
	{
		sendErrorMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NOT_DECLARE;
		pPC->Write((BYTE*)&sendErrorMsg, sizeof(SP_CASTLE_INFO));
		return;
	}

	if (CheckDeclareList(pClan->GetClanUnique()) == TRUE)
	{
		sendErrorMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NOT_DECLARE;
		pPC->Write((BYTE*)&sendErrorMsg, sizeof(SP_CASTLE_INFO));
		return;
	}
	if (pClan->GetMasterunique() != pPC->GetCharacterUnique())
	{
		sendErrorMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NOT_DECLARE;
		pPC->Write((BYTE*)&sendErrorMsg, sizeof(SP_CASTLE_INFO));
		return;
	}


	if (pClan->GetClanMoney() < i64DeclareCost)
	{
		sendErrorMsg.i32Result = 4;
		pPC->Write((BYTE*)&sendErrorMsg, sizeof(SP_CASTLE_INFO));
		return;
	}

	pClan->AddMoney(-i64DeclareCost, pPC, CLAN_MONEY_GOLD_TYPE::CLAN_MONEY_CASTLE_DECLARE, pPC->GetCharacterUnique(), 0, _SaveInfo._i32CastleIndex, i64DeclareCost);
}

void CCastle::DeclareAdd(CUnitPC* pPC, INT64 i64Money)
{
	if (pPC == NULL)
		return;

	// 선포
	DCP_CASTLE_DECLARE sendDb;
	sendDb._i32CastleIndex = this->GetCastleIndex();
	sendDb._i32Type = 1;
	memcpy(sendDb._stInfo._wcMasterName, pPC->GetName(),sizeof(WCHAR)* LENGTH_CHARACTER_NICKNAME);
	sendDb._stInfo._ui32Clanunique = pPC->GetClanUnique();;
	sendDb._stInfo._i64DeclareCost = i64Money;
	sendDb._dwCharunique = pPC->GetCharacterUnique();
	//팀유니크 넣을꺼면여기!!

	CClan* pPCClan = pPC->GetClan();

	if (pPCClan)
	{
		memcpy(sendDb._stInfo._wcClanName, pPCClan->GetClanName(), sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
		memcpy(&sendDb._stInfo._stSimbolIndex,&pPCClan->GetClanSimbolIdx(), sizeof(stClanSymbol));
		sendDb._stInfo._i32MemberCnt = pPCClan->GetMemberCnt();

	}
	else
	{
		printf("pPCGuild NULL !! \n");
		return;
	}


	g_Server.SendMsg(g_ServerArray.Bravo_GameDBServer1, (BYTE*)&sendDb,	sizeof(DCP_CASTLE_DECLARE));
	
}



BOOL CCastle::CheckDeclareList(INT32 ClanUnique)
{
	for (auto const&  iter : _vecDeclareList)
	{
		if (iter._ui32Clanunique == ClanUnique)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CCastle::SendDeclareList(CUnitPC* pPc)
{
	if (pPc == NULL)
		return;

	SP_CASTLE_DECLARE_LIST sendList;
	sendList.bEnd = FALSE;
	//선포리스트가 없을 경우
	if (_vecDeclareList.size() < 1)
	{
		sendList.bEnd = TRUE;
		pPc->Write((BYTE*)&sendList, sizeof(SP_CASTLE_DECLARE_LIST));
		return;
	}
	for (auto iter = _vecDeclareList.begin(); iter != _vecDeclareList.end(); ++iter)
	{
		// 안전한 복사 방식 사용
		sendList.stDeclareInfo = *iter;

		// 마지막 원소인지 확인
		if (std::next(iter) == _vecDeclareList.end())
		{
			sendList.bEnd = TRUE;
		}
		else
		{
			sendList.bEnd = FALSE; // 마지막이 아닐 때는 명시적으로 FALSE로 설정하는 것이 좋습니다.
		}

		// 데이터 전송
		pPc->Write((BYTE*)&sendList, sizeof(SP_CASTLE_DECLARE_LIST));
	}


}

void CCastle::OutRewardDungeonMap()
{
	if(_mapLevelInfo.find(GetCastleLevel()) == _mapLevelInfo.end())
		return;

	CGameMap* pMap = g_MapManager.FindMapIndex(_mapLevelInfo.find(GetCastleLevel())->second.i32RewardMapIndex);
	if (pMap)
	{
		map<INT32, CUnitPC*>& playerList = *pMap->GetMapPlayerList();
		if (playerList.size() >0)
		{
			for (auto iter = playerList.begin(); iter != playerList.end(); ++iter)
			{
				if (iter->second != NULL)
				{
					OutRewardDungeon(iter->second,TRUE);
				}
			}
		}
		pMap->GetMapPlayerList()->clear();


		if (_pBossNpc)
		{
			_pBossNpc->SetDieAlone(TRUE);
			g_DungeonBossSummonManager.KillSummonBoss(_pBossNpc);
			_pBossNpc = NULL;
		}

	}

	for (const auto& iter : g_GameManager.mUserManager.m_pUsers)
	{
		if (iter)
		{
			if (iter->GetMapIndex() == _mapLevelInfo.find(GetCastleLevel())->second.i32RewardMapIndex)
			{
				OutRewardDungeon(iter, TRUE);
			}
		}
	}

}

void CCastle::SummonNpcList()
{
	CGameMap* pBattleMap = g_MapManager.FindMapIndex(GetMapIndex());  
	if (pBattleMap == NULL)
		return;

	for (const auto& iter : _vecSummonList)
	{

		int iNewNPCArray = g_GameManager.mNPCManager.GetEmptyArray();
		if (iNewNPCArray == -1)
			return;

		CUnitNPC* pNPC = &g_GameManager.mNPCManager.m_pNPC[iNewNPCArray];
		if (pNPC)
		{
			pNPC->SetNpcInfo(pBattleMap, iter.i32NpcIndex, iter.i32X, iter.i32Y);
			_mapNpc.insert(make_pair(iter.Index, pNPC));
			pNPC->SetTeamUnique(_SaveInfo._ui32OwnerClanunique, _SaveInfo._ui32OwnerClanunique, GetCastleIndex());
			pNPC->SetStatus(NPC_CURRENT_STATUS::REGEN);
		}
		CLogManager::CASTLE_NPC_SPAWN_LOG(GetCastleIndex(), iter.i32Type);
	}

}

INT32 CCastle::TotalBattleMember()
{
	CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
	if (pMap)
	{
		return pMap->m_BlockManager.GetUnitCount_PC();
	}

	INT32 DefenceMember = 0;
	if (pMasterClan)
	{
		DefenceMember = pMasterClan->GetMemberCnt();
	}
	INT32 Attackermember = 0;
	for (const auto& iter : _vecDeclareList)
	{
		Attackermember += iter._i32MemberCnt;
	}


	return DefenceMember+ Attackermember;
}



void CCastle::BroadCastNPCAttackTime(CASTLE_BATTLE_NPC_TYPE eType)
{
	CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
	if (pMap)
	{
		INT32 i32NpcDocIndex = -1;
		for (const auto& iter : _vecSummonList)
		{
			if (iter.i32Type == eType)
			{
				i32NpcDocIndex = iter.i32NpcIndex;
				break;
			}
		}

		if (_mapNpc.find(i32NpcDocIndex) == _mapNpc.end())
		{
			return;
		}

		CUnitNPC* pNpc = _mapNpc.find(i32NpcDocIndex)->second;

		if (pNpc)
		{
			SP_NPCAppear sendNPCAppear;
			sendNPCAppear.i16Transparent = pNpc->GetTransparent();
			sendNPCAppear._NpcIndex = pNpc->GetIndex();
			sendNPCAppear._position = pNpc->GetPosition();
			sendNPCAppear.dwFieldUnique = pNpc->GetFieldUnique();
			sendNPCAppear._dwOwner = pNpc->GetOwnerUnit();
			sendNPCAppear._detail = pNpc->GetDetail(Appear_Detail::OPTION_FOV);

			if (pNpc->CheckTeam(pNpc->GetCastleIndex(), pNpc->GetTeamunique()) == true )
				sendNPCAppear._detail |= 1 << 30;
			if (pNpc->CheckTeam2(pNpc->GetCastleIndex(), pNpc->GetTeamunique2()) == true)
				sendNPCAppear._detail |= 1 << 31;


			sendNPCAppear.i16AttackSpeed = pNpc->stCommonStatus.GetAbility(ABILITY_POS::ABILITY_POS_ATTACK_SPEED);
			sendNPCAppear.i16CastingSpeed = pNpc->stCommonStatus.GetAbility(ABILITY_POS::ABILITY_POS_CASTING_SPEED);

			sendNPCAppear._hpPercent = pNpc->GetHpPercent();
			pMap->m_BlockManager.BroadCast_AllBlock_Enemy((BYTE*)&sendNPCAppear, sizeof(SP_NPCAppear), TRUE, pNpc->GetCastleIndex(), pNpc->GetTeamunique());
		}
	}
}

BOOL CCastle::CheckMovableDoorTile( MAP_TILE_TYPE eMapTile)
{
	INT32 i32CastleNpcIdx = 0;
	for (const auto& iter : _vecSummonList)
	{
		if (iter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A && eMapTile == MAP_TILE_CASTLE_GATE_A)
		{
			if (_mapNpc.find(iter.Index) != _mapNpc.end())
			{
				return FALSE;
			}
			return TRUE;
		}
		else if (iter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B && eMapTile == MAP_TILE_CASTLE_GATE_B)
		{
			if (_mapNpc.find(iter.Index) != _mapNpc.end())
			{
				return FALSE;
			}
			return TRUE;
		}
		else if (iter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C && eMapTile == MAP_TILE_CASTLE_GATE_C)
		{
			if (_mapNpc.find(iter.Index) != _mapNpc.end())
			{
				return FALSE;
			}
			return TRUE;
		}
	
	}

	return TRUE;
}

UINT32 CCastle::GetTeamunique(CUnitPC* pPC)
{
	if (pPC == NULL)
		return 0;

	if (pPC->GetClan() == NULL || pPC->GetClanUnique() == 0)
		return 0;
	if (pPC->GetClanUnique() == this->GetCastleClanUnique())
		return pPC->GetClanUnique();


	if (CheckDeclareList(pPC->GetClanUnique()) == TRUE)
	{
		return pPC->GetClanUnique();
	}
	return 0;
}

UINT32 CCastle::GetTeamunique2(CUnitPC* pPC)
{
	if (pPC == NULL)
		return 0;

	if (pPC->GetClan() == NULL || pPC->GetClanUnique() == 0)
		return 0;
	if (pPC->GetClanUnique() == this->GetCastleClanUnique())
		return pPC->GetClanUnique();


	if (CheckDeclareList(pPC->GetClanUnique()) == TRUE)
	{
		return pPC->GetClanUnique();
	}
	return 0;
}



ENUM_ERROR_MESSAGE CCastle::EnterRewardDungeon(CUnitPC* pPC)
{
	if (pPC == NULL)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	}

	//SP_CASTLE_EDIT sendMsg;
	//sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::REWARD_DUNGEON_ENTER;
	//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;

	if (pPC->GetClan() == NULL)
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
	}

	if (pPC->GetClan() != GetMasterClan() || pPC->GetClanUnique() != _SaveInfo._ui32OwnerClanunique)
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
	}

	//던전 사용중
	if (pPC->m_DungeonList.CheckDungeonNow())
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_USE_DUNGEON; // 이미 던전에 들어와있다. 
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_USE_DUNGEON;
	}
	if (g_DungeonManager.CheckWaveDungeonMap(pPC->GetMapIndex()) == TRUE)
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_USE_DUNGEON; // 이미 던전에 들어와있다. 
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_USE_DUNGEON;
	}

	//워프중
	if (pPC->m_bWARP_PENDING == TRUE)
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
	}
	if (pPC->GetCurrentMap() == NULL || pPC->GetCurrentMap()->GetMapTYPE() != eMAP_TYPE::MAP_TYPE_NORMAL)
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
	}

	//공성중
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_NONE )
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
	}
	//파티중
	if (pPC->GetParty())
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_PARTY_ALREADY_JOIN;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_PARTY_ALREADY_JOIN;
	}

	if (_mapLevelInfo.find(GetCastleLevel()) == _mapLevelInfo.end())
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_REWARD_DUNGEON_INFO_NULL;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_REWARD_DUNGEON_INFO_NULL;
	}

	stCastleLevelInfo& pData = _mapLevelInfo.find(GetCastleLevel())->second;
	CGameMap* pMap = g_MapManager.FindMapIndex(pData.i32RewardMapIndex);
	if (pMap == NULL)
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_REWARD_DUNGEON_INFO_NULL;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_REWARD_DUNGEON_INFO_NULL;
	}


	SP_CASTLE_EDIT sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::REWARD_DUNGEON_ENTER;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;

	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));

	pPC->m_DungeonList.SetReturnPos(pPC->GetMapIndex(),pPC->m_X,pPC->m_Y);
	pPC->m_DungeonList.bProgress = TRUE;

	g_MapManager.UIWarp(pPC, 0, pData.i32RewardMapIndex, pData.i32EnterX, pData.i32EnterY, FALSE);
	pMap->EnterUser(pPC);
	return ENUM_ERROR_MESSAGE::SUCCESS;
}

void CCastle::SummonBossLevel(CGameMap* pMap, INT32* i32BossIndex, INT32* i32BossX, INT32* i32BossY)
{
	if (_mapLevelInfo.find(GetCastleLevel()) == _mapLevelInfo.end())
	{
		return;
	}
	if (_pBossNpc != NULL)
		return;

	stCastleLevelInfo& pData = _mapLevelInfo.find(GetCastleLevel())->second;

	CGameMap* pRewardMap = g_MapManager.FindMapIndex(pData.i32RewardMapIndex);
	if (pRewardMap == NULL)
	{
		return;
	}
	//pMap = pRewardMap;
	*i32BossIndex = pData.i32BossIndex;


}

void CCastle::KillCastleBossNPC(CUnitNPC* pBossNpc)
{
	if (_pBossNpc != pBossNpc)
		return;


	_pBossNpc = NULL;
}

BOOL CCastle::CheckHuntNPC(CUnitPC* pPC,INT32 i32ItemIndex)
{
	if (pPC == NULL)
		return FALSE;

	if (i32ItemIndex != SERVER_CASTLE_COIN)
		return TRUE;


	if (_mapLevelInfo.find(GetCastleLevel()) == _mapLevelInfo.end())
	{
		return FALSE;
	}

	stCastleLevelInfo& pData = _mapLevelInfo.find(GetCastleLevel())->second;

	CGameMap* pRewardMap = g_MapManager.FindMapIndex(pData.i32RewardMapIndex);
	if (pRewardMap->m_wMapIndex != pPC->GetMapIndex())
	{
		return FALSE;
	}

	INT32 i32TodayCount = pPC->m_DungeonList.GetServerCastletodayCount();
	if (i32TodayCount + 1 > pData.i32MaxDrawItemCnt)
	{
		return FALSE;	
	}

	SP_CASTLE_EDIT sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::REWARD_DUNGEON_ITEM_COUNT;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
	sendMsg.i32CastleIndex = GetCastleIndex();

	pPC->m_DungeonList.SetServerCastleTodayCount(i32TodayCount+1);
	//pPC->Write((BYTE*)&sendMsg,sizeof(SP_CASTLE_EDIT));
	return TRUE;
	
}

void CCastle::LoadCastleTimeDungeon()
{
	auto pDungeon = g_DungeonManager.FindDungeonData(GetGoldRewardDungeonIndex(),1);
	if (pDungeon == NULL)
	{
		printf("CastleGoldDungeon Data NULL\n");
		return;
	}

	CGameMap* pMap = g_MapManager.FindMapIndex(pDungeon->GetDungeonMapIndex());
	if (pMap == NULL)
	{
		printf("CastleGoldDungeon Map NULL\n");
		return;
	}
	INT32 i32GroupNumber = 0;
	//npcindex,번호
	map<int, int> mapNpcGroup;
	//해당맵의 npcInfo를 다 들고있는다.
	for (auto& iter : pMap->m_MapInfo->GetMapData()->npcInfo[0])
	{
		if (mapNpcGroup.find(iter._npcIndex) == mapNpcGroup.end())
		{
			_mapTimeDungeonNpcInfo[i32GroupNumber].push_back(&iter);
			mapNpcGroup.insert(make_pair(iter._npcIndex, i32GroupNumber));
			i32GroupNumber++;
		}
		else
		{
			_mapTimeDungeonNpcInfo[mapNpcGroup.find(iter._npcIndex)->second].push_back(&iter);
		}
	}
	mapNpcGroup.clear();


	for(auto iter :g_GameManager.mNPCManager.m_NpcArray)
	{
		if (iter->GetMapIndex() == pDungeon->GetDungeonMapIndex())
		{
			_vecTimeDungeonNpcPtr.push_back(iter);
		}
	}
	

}

void CCastle::UpdateCastleCostDungeonSummonList()
{
	for (auto iter : _vecTimeDungeonNpcPtr)
	{
		if (iter)
		{
			iter->SetStatus(NPC_CURRENT_STATUS::DIE);
		}
	}


	stCastleLevelInfo pCastleData = _mapLevelInfo.find(GetCastleLevel())->second;

	pCastleData.i32GoldDungeonSummonList;
	for(auto finditer  :_mapTimeDungeonNpcInfo)
	{
		for (auto iter : finditer.second)
		{
			if (iter)
			{
				iter->_npcIndex = pCastleData.i32GoldDungeonSummonList[finditer.first];
			}
		}
	} 
}

ENUM_ERROR_MESSAGE CCastle::SettingCastleTimeDungeon(CUnitPC* pPC, INT32 i32SettingCost, BOOL bEnter)
{
	if (pPC == NULL)
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;

	//공성중
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_NONE)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}
	CClan* pClan = pPC->GetClan();
	if (pClan == NULL
		|| pClan->GetMasterunique() != pPC->GetCharacterUnique()
		|| GetMasterClan() == NULL
		|| GetMasterClan() != pClan)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CLAN_NOT_MANAGER;
	}

	if (i32SettingCost < 10 || i32SettingCost > 100)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CASTLE_DUNGEON_NOT_SETTING_COST;
	}

	//12시간 쿨타임 ::TODO:: 임시로 10초
	if (_SaveInfo.ui32LastSettingTimeStamp + 10/*43200*/ > g_GameManager.GetTimeStamp_Second())
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CASTLE_DUNGEON_COOLTIME;
	}

	_SaveInfo.i32TimeBuyCost = i32SettingCost;	//설정 금액
	_SaveInfo.bTimeDungeonOpen = bEnter;	//성주가 열었는지 닫았는지


	_SaveInfo.ui32LastSettingTimeStamp = g_GameManager.GetTimeStamp_Second(); //셋팅 타임스템프

	SP_CASTLE_EDIT sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::SETTING_CASTLE_COST_DUNGEON;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;

	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));

	DCP_SIEGE_READY_DATA sendInfo;
	sendInfo.i32Charunique = pPC->GetCharacterUnique();
	sendInfo.i32Group = pPC->GetGroupId();
	sendInfo.i32CastleIndex = GetCastleIndex();
	pPC->WriteGameDB((BYTE*)&sendInfo, sizeof(DCP_SIEGE_READY_DATA));


	return ENUM_ERROR_MESSAGE::SUCCESS;
}

ENUM_ERROR_MESSAGE CCastle::BuyCastleTimeDungeonTime(CUnitPC* pPC, INT32 i32Count)
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_NONE)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}

	if (GetMasterClan() == NULL)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}
	//성주측은 사용불가
	if (pPC->GetClan() == GetMasterClan())
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}

	if (_SaveInfo.bTimeDungeonOpen == FALSE)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CASTLE_DUNGEON_NOTADDTIME;
	}
	if (pPC->m_DungeonList.CheckDungeonNow() == TRUE)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CASTLE_DUNGEON_NOTADDTIME;
	}
	auto pDungeon = g_DungeonManager.FindDungeonData(GetGoldRewardDungeonIndex(), 1);
	if (pDungeon == NULL)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CASTLE_DUNGEON_NOTADDTIME;
	}

	INT32 i32Cost = i32Count * _SaveInfo.i32TimeBuyCost;
	if (pPC->GetDark() < i32Cost)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_LACK_DARK;
	}
	


	pPC->SetDark(-i32Cost, i32Count, L"CASTLE_DUNGEON_TIME", CASH_CONSUME_TYPE::CASH_CONSUME_ADD_CASTLE_DUNGEON_TIME, 0, GetCastleIndex());


	SP_CASTLE_EDIT sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::SETTING_CASTLE_COST_DUNGEON;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;

	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
	return ENUM_ERROR_MESSAGE::SUCCESS;
}

void CCastle::ON_CastleDungeon_Buy_Compleate(CUnitPC* pPC, INT32 i32Count)
{
	if (pPC == NULL)
		return;

	auto pDungeon = g_DungeonManager.FindDungeonData(GetGoldRewardDungeonIndex(), 1);
	if (pDungeon == NULL)
		return;


	INT32 iDungeonArrayIdx = pDungeon->_i32ArrayIndex;
	INT32 i32AddTime = 3600 * i32Count;//23.03.08남일우 추가
	pPC->m_DungeonList.m_DungeonInfo[iDungeonArrayIdx]._ui32RestTimePlus += i32AddTime; //23.03.08남일우 수정


	SP_DUNGEON_PROC sendDungeonInfo;
	sendDungeonInfo.dwValue = pPC->m_DungeonList.m_DungeonInfo[iDungeonArrayIdx]._ui32RestTimePlus + pPC->m_DungeonList.m_DungeonInfo[iDungeonArrayIdx].m_dwRestTime;
	sendDungeonInfo.iDungeonType = 0;
	sendDungeonInfo.iDungeonIndex = pDungeon->GetDungeonIndex();
	sendDungeonInfo.iResult = 100;
	pPC->Write((BYTE*)&sendDungeonInfo, sizeof(SP_DUNGEON_PROC));


	CLogManager::DUNGEON_ADD_TIME(pPC, pDungeon->GetDungeonIndex()+1, 0, i32Count, pPC->m_DungeonList.m_DungeonInfo[iDungeonArrayIdx].m_dwRestTime,
		pPC->m_DungeonList.m_DungeonInfo[iDungeonArrayIdx]._ui32RestTimePlus, pPC->m_DungeonList.m_DungeonInfo[iDungeonArrayIdx]._ui32RestTimePlus - i32AddTime);


	SP_CASTLE_EDIT sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::ADD_CASTLE_COST_DUNGEON_TIME;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;

	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
}

void CCastle::SendCastleDungeonInfo(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	SP_CASTLE_GOLDDUNGEON_INFO sendMsg;
	sendMsg.bEnter =_SaveInfo.bTimeDungeonOpen;
	sendMsg.bEnterTime = GetCastleState() == CASTLE_TIME_STATE::CTS_NONE? TRUE:FALSE ;
	sendMsg.i32CastleDungeonCost = _SaveInfo.i32TimeBuyCost;
	sendMsg.i32CastleGrade = GetCastleLevel();

	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_GOLDDUNGEON_INFO));

}

ENUM_ERROR_MESSAGE CCastle::OutRewardDungeon(CUnitPC* pPC,BOOL bClear)
{
	if (pPC == NULL)
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	//SP_CASTLE_EDIT sendMsg;
	//sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::REWARD_DUNGEON_OUT;
	//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;


	//워프중
	if (pPC->m_bWARP_PENDING == TRUE)
	{
		//sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		//pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
	}

	if (pPC->GetAlive() == FALSE)
	{
		pPC->SetForceAlive();
	}

	INT32 i32ReturnMapindex = pPC->m_DungeonList.GetReturnMapIndex();
	INT32 i32ReturnX = pPC->m_DungeonList.GetReturnX();
	INT32 i32ReturnY = pPC->m_DungeonList.GetReturnY();

	pPC->m_DungeonList.bProgress = FALSE;

	if (bClear == FALSE)
	{
		CGameMap* pMap = g_MapManager.FindMapIndex(_mapLevelInfo.find(GetCastleLevel())->second.i32RewardMapIndex);
		if (pMap)
		{
			pMap->GetMapPlayerList()->erase(pPC->GetCharacterUnique());
		}
	}

	if (g_DungeonManager.CheckWaveDungeonMap(i32ReturnMapindex) == TRUE || g_DungeonManager.CheckDailyMapIndex(i32ReturnMapindex) == TRUE)
	{
		g_MapManager.UIWarp(pPC, 0, 1001, 220, 262, FALSE);

	}
	else
	{
		g_MapManager.UIWarp(pPC, 0, i32ReturnMapindex, i32ReturnX, i32ReturnY, FALSE);
	}

	return ENUM_ERROR_MESSAGE::SUCCESS;
}
