#include "StdAfx.h"
#include <time.h>
#include <map>
#include "../server_Common/TIDefine.h"
#include "../server_Common/TIStruct.h"
#include "MapManager.h"

#include "CSVFile_CastleInfo.h"
#include "CSVFile_CastleWarpInfo.h"
#include "CSVFile_CastleNPCInfo.h"

#include "Unit.h"
#include "UnitPC.h"
#include "UnitNPC.h"
#include "Castle.h"

#include "ServerCastle.h"
#include "CastleManager.h"
#include "GameManager.h"
#include "GameMap.h"

 CCastleManager g_CastleManager;

void CCastleManager::ON_CM_Request_Castle_Ready_Data(CUnitPC* pPC)
{
	if(pPC == NULL)
		return;

	if (FALSE == g_ClientArray[g_ServerArray.Bravo_CashDB].IsConnected())
	{
		SP_SIEGE_READY_DATA sendMsg;
		sendMsg.i64CastleGoldBalance = 0;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_SIEGE_READY_DATA));
		return ;
	}

	DCP_SIEGE_READY_DATA sendMsg;
	sendMsg.i32Charunique = pPC->GetCharacterUnique();
	sendMsg.i32Group = pPC->GetGroupId();
	sendMsg.i32CastleIndex = 1;
	sendMsg.bReadyNot = TRUE;
	pPC->WriteGameDB((BYTE*)&sendMsg,sizeof(DCP_SIEGE_READY_DATA));


}

void CCastleManager::ON_Send_CastleReadyData(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;
	DSP_SIEGE_READY_DATA* pMsg = (DSP_SIEGE_READY_DATA*)pPacket;
	CUnitPC* pClient = g_GameManager.mUserManager.FindUser(pMsg->i32Charunique);
	if (pClient)
	{
		SP_SIEGE_READY_DATA sendMsg;
		sendMsg.i64CastleGoldBalance = pMsg->i64CastleGoldBalance;

		pClient->Write((BYTE*)&sendMsg,sizeof(SP_SIEGE_READY_DATA));
	}

}

void CCastleManager::Init()
{
	mbCastleLoop = FALSE;
	_ullCurTime = 0;
	_ullPrevTime = 0;
	_ullOccupationRuleCurTime = 0;
	_ullOccupationRulePrevTime = 0;

	_mapIOccupationRuleList.clear();
	for (auto iter : _mapCastleList)
	{
		if (iter.second != NULL)
		{
			delete iter.second;
			iter.second = NULL;
		}
	}
	_mapCastleList.clear();

}

void CCastleManager::LoadData()
{
	auto CastleData = g_CSVFile_CastleInfo.GetFirstRecordData();
	while (CastleData)
	{
		switch ((CASTLE_TYPE)CastleData->_i32Type)
		{
		case CASTLE_TYPE::SERVER_CASTLE:
		{
			CServerCastle* pCastle = new CServerCastle();
			pCastle->LoadData(CastleData);
			_mapCastleList.insert(make_pair(CastleData->_i32Index,pCastle));
			_mapIOccupationRuleList.insert(make_pair(CastleData->_i32Index,(IOccupationRule*)pCastle));
			_mapIClanFlagRuleList.insert(make_pair(CastleData->_i32Index,(IClanFlagRule*)pCastle));
		}
		break;

		default:
			break;
		}
		CastleData = g_CSVFile_CastleInfo.GetNextRecordData();
	}
	g_CSVFile_CastleInfo.Clear();

	auto CastleWarpData = g_CSVFile_CastleWarpInfo.GetFirstRecordData();
	while(CastleWarpData)
	{
		_mapCastleList[CastleWarpData->_i32Type]->LoadWarpInfo(CastleWarpData);

		CastleWarpData = g_CSVFile_CastleWarpInfo.GetNextRecordData();
	}
	g_CSVFile_CastleWarpInfo.Clear();


	auto CastleNpcData = g_CSVFile_CastleNPCInfo.GetFirstRecordData();
	while (CastleNpcData)
	{
		_mapCastleList[CastleNpcData->_i32CastleIndex]->LoadNPCInfo(CastleNpcData);

		CastleNpcData = g_CSVFile_CastleNPCInfo.GetNextRecordData();
	}
	g_CSVFile_CastleNPCInfo.Clear();

	auto CastleLevelData = g_CSVFile_CastleLevelInfo.GetFirstRecordData();
	while (CastleLevelData)
	{
		_mapCastleList[CastleLevelData->_i32CastleIndex]->LoadLevelInfo(CastleLevelData);

		CastleLevelData = g_CSVFile_CastleLevelInfo.GetNextRecordData();
	}
	g_CSVFile_CastleLevelInfo.Clear();

}

void CCastleManager::ON_ConnectGameDB(INT32 i32Client)
{
	for (auto& findIter : _mapCastleList)
	{
		if (findIter.second != NULL)
		{
			findIter.second->LoadDB(i32Client);
		}
	}
}

void CCastleManager::ON_Load_GameDB(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;

	DSP_REPLY_CASTLE_DATA* pMsg = (DSP_REPLY_CASTLE_DATA*)pPacket;
	if (_mapCastleList.find(pMsg->_i32CastleIndex) != _mapCastleList.end())
	{
		_mapCastleList.find(pMsg->_i32CastleIndex)->second->SetCastleData(pPacket);

		if (g_ClientArray[g_ServerArray.Bravo_AccountDBServer].IsConnected())
		{
			OnScheduleConnect(pMsg->_i32CastleIndex, (INT32)SCHEDULE_WORK_TYPE::CASTLE_START);
			OnScheduleConnect(pMsg->_i32CastleIndex, (INT32)SCHEDULE_WORK_TYPE::CASTLE_END);
		}
	}

}

void CCastleManager::OnScheduleConnect(INT32 i32CastleIndex, INT32 i32WorkType)
{
	DCP_CASTLE_SCHEDULE_READY sendReady;
	sendReady.wCastleIndex = (WORD)i32CastleIndex;
	sendReady.wWorkType = (WORD)i32WorkType;
	g_Server.SendMsg(g_ServerArray.Bravo_AccountDBServer, (BYTE*)&sendReady, sizeof(DCP_CASTLE_SCHEDULE_READY));	
}

void CCastleManager::SetCastleTimeStamp(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;
	DSP_SCHEDULE_TIMESTAMP* pMsg = (DSP_SCHEDULE_TIMESTAMP*)pPacket;
	
	if (_mapCastleList.find(pMsg->_i32RotationNum) == _mapCastleList.end())
		return;

	CCastle* pCastle = _mapCastleList.find(pMsg->_i32RotationNum)->second;
	pCastle->SetSchedule(pMsg->_ui32ReservationTimestamp);

	//하나만 성공해도 Loop시작
	CastleManagerInitCompleate();
}

void CCastleManager::SetCastleEndBattleStamp(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;
	DSP_SCHEDULE_TIMESTAMP* pMsg = (DSP_SCHEDULE_TIMESTAMP*)pPacket;

	if (_mapCastleList.find(pMsg->_i32RotationNum) == _mapCastleList.end())
		return;

	CCastle* pCastle = _mapCastleList.find(pMsg->_i32RotationNum)->second;
	if (pCastle->GetCastleState() != CTS_BATTLE)
	{
		pCastle->SetEndBattleTimeStamp(pMsg->_ui32ReservationTimestamp);
	}
}

void CCastleManager::CastleManagerInitCompleate()
{
	if (mbCastleLoop == FALSE)
	{
		mbCastleLoop = TRUE;
		_ullPrevTime = GetTickCount64();
		_ullOccupationRulePrevTime = GetTickCount64();
	}
}

void CCastleManager::CastleBattleLoad(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;

	DCP_LOAD_CASTLE_BATTLE_DATA* pMsg = (DCP_LOAD_CASTLE_BATTLE_DATA*)pPacket;
	if (_mapCastleList.find(pMsg->i32CastleIndex) == _mapCastleList.end())
		return;
	CCastle* pCastle = _mapCastleList.find(pMsg->i32CastleIndex)->second;
	if (pCastle == NULL)
		return;
	pCastle->ON_Load_SaveData(pPacket);

}

void CCastleManager::ON_DSM_CASTLE_DEBUFF(INT32 i32CastleIndex, INT32 i32DebuffPer)
{
	if (_mapCastleList.find(i32CastleIndex) == _mapCastleList.end())
		return;

	CCastle* pCastle = _mapCastleList.find(i32CastleIndex)->second;
	if (pCastle == NULL)
		return;
	pCastle->SetDebuffPer(i32DebuffPer);
}

void CCastleManager::CASTLE_DAMAGE_DEBUFF(INT64* i64Damage,INT32 i32Mapindex)
{
	CCastle* pCastle =  FindCastleMapindex(i32Mapindex);
	if (pCastle)
	{
		INT32 i32Damage = 100 - pCastle->GetDebuffPer();
		*i64Damage = ((*i64Damage * i32Damage) / 100);
	}
}

void CCastleManager::ON_CM_CastleInfo(BYTE* pPacket, CUnitPC* pPC)
{
	if (pPacket == NULL || pPC == NULL)
		return;

	CP_CASTLE_INFO* pMsg = (CP_CASTLE_INFO*)pPacket;
	SP_CASTLE_INFO SendError;
	SendError.i32Type = (INT32)ENUM_CP_CASTLE_INFO_TYPE::INFO;
	SendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;

	if (_mapCastleList.find(pMsg->i32CastleIndex) == _mapCastleList.end())
	{
		pPC->Write((BYTE*)&SendError,sizeof(SP_CASTLE_INFO));
		return;
	}

	CCastle* pCastle = _mapCastleList.find(pMsg->i32CastleIndex)->second;

	switch (pCastle->GetCastleType())
	{
	case CASTLE_TYPE::SERVER_CASTLE:
	{
		CServerCastle* pServerCastle = dynamic_cast<CServerCastle*> (pCastle);
		if (pServerCastle == NULL)
		{
			pPC->Write((BYTE*)&SendError, sizeof(SP_CASTLE_INFO));
		}
	
		switch ((ENUM_CP_CASTLE_INFO_TYPE)pMsg->i32Type)
		{
		case ENUM_CP_CASTLE_INFO_TYPE::INFO:
		{
			pServerCastle->ON_CM_CastleInfo(pPC);
		}
		break;
		case ENUM_CP_CASTLE_INFO_TYPE::DECLARE:
		{
			pServerCastle->Declare(pPC, pMsg->i32DeclareCost);
		}
		break;
		default:
		{
			pPC->Write((BYTE*)&SendError, sizeof(SP_CASTLE_INFO));
			return;
		}
		break;
		}

	}
	break;
	default:
	{
		pPC->Write((BYTE*)&SendError, sizeof(SP_CASTLE_INFO));
		return;
	}
	break;
	}

}

void CCastleManager::ON_Send_CastleInfo(BYTE* pPacket, WORD wID)
{
	if (pPacket == NULL || wID == NULL)
		return;

	switch (wID)
	{
	case DSM_SIEGE_READY_DATA:
	{
		DSP_SIEGE_READY_DATA* pMsg = (DSP_SIEGE_READY_DATA*)pPacket;

		if (_mapCastleList.find(pMsg->i32CastleIndex) == _mapCastleList.end())
		{
			return;
		}
		//공성문서 빠질경우 클라패킷요청
		if (pMsg->bReadyNot == TRUE)
		{
			ON_Send_CastleReadyData(pPacket);
			return;
		}

		CCastle* pCastle = _mapCastleList.find(pMsg->i32CastleIndex)->second;
		switch (pCastle->GetCastleType())
		{
		case CASTLE_TYPE::SERVER_CASTLE:
		{
			CServerCastle* pServerCastle = dynamic_cast<CServerCastle*> (pCastle);
			if (pServerCastle == NULL)
			{
				return;
			}
			pServerCastle->ON_Send_CastleInfo(pMsg->i32Charunique,pMsg->i64CastleGoldBalance,pMsg->i64CastleDungeonBalance);

		}
		break;
		}

	}
	break;
	default:
	{
		return;
	}
	}


}



void CCastleManager::ON_Declare_Add(CUnitPC* pPC, INT32 i32CastleIndex, INT64 i64Money)
{
	if (pPC == NULL)
		return;

	SP_CASTLE_INFO SendError;
	SendError.i32Type = (INT32)ENUM_CP_CASTLE_INFO_TYPE::DECLARE;
	SendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;


	if (_mapCastleList.find(i32CastleIndex) == _mapCastleList.end())
	{
		SendError.i32Result = 2;
		pPC->Write((BYTE*)&SendError, sizeof(SP_CASTLE_INFO));
		return;
	}
	CCastle* pCastle = _mapCastleList.find(i32CastleIndex)->second;
	if (pCastle)
	{
		pCastle->DeclareAdd(pPC, i64Money);
	}
}

void CCastleManager::ON_Declare_Compleate(BYTE*pPacket )
{
	if (pPacket == NULL)
		return;
	DSP_CASTLE_DECLARE* pMsg = (DSP_CASTLE_DECLARE*)pPacket;

	if (pMsg->_i32Type == 1)
	{
		CUnitPC* pPC = g_GameManager.mUserManager.FindUser(pMsg->_dwCharunique);

		if (_mapCastleList.find(pMsg->_i32CastleIndex) != _mapCastleList.end())
		{
			CCastle* pCastle = _mapCastleList.find(pMsg->_i32CastleIndex)->second;
			pCastle->DeclareCompleate(pPC, &pMsg->_stInfo);
		}
	}
	else
	{
		printf("Declare Failed\n");
	}


}

void CCastleManager::ON_CM_CastleBattleMap(CUnitPC* pPC,INT32 i32CastleIndex,INT32 i32Type, INT32 i32WarpIndex)
{
	if (pPC == NULL)
		return;

	SP_CASTLE_BATTLEMAP sendError;
	sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	sendError.i32Type = i32Type;

	if (_mapCastleList.find(i32CastleIndex) == _mapCastleList.end())
	{
		pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}
	if (_mapCastleList.find(i32CastleIndex)->second == NULL)
	{
		pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	if (_mapCastleList.find(i32CastleIndex)->second->GetCastleState() != CTS_READY_TEN_MIN &&
		_mapCastleList.find(i32CastleIndex)->second->GetCastleState() != CTS_BATTLE)
	{
		sendError.i32Result = ENUM_ERROR_MESSAGE_CASTLE_BATTLE_NOTTIME;
		pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	switch ((ENUM_CASTLE_BATTLEMAP_TYPE)i32Type)
	{
	case ENUM_CASTLE_BATTLEMAP_TYPE::WARP_WINDOW:
	{
		_mapCastleList.find(i32CastleIndex)->second->SendBattlMap(pPC);
	}
	break;
	case ENUM_CASTLE_BATTLEMAP_TYPE::BATTLE_WINDOW:
	{
		_mapCastleList.find(i32CastleIndex)->second->ShowBattleInfo(pPC);
	}
	break;
	case ENUM_CASTLE_BATTLEMAP_TYPE::WARP:
	{
		_mapCastleList.find(i32CastleIndex)->second->WarpBattleMap(pPC, i32WarpIndex);
	}
	break;
	case ENUM_CASTLE_BATTLEMAP_TYPE::OUT_BATTLEMAP:
	{
		 CCastle* pCastle =FindCastleMapindex(pPC->GetMapIndex());
		 if (pCastle)
		 {
			 pCastle->OutBattleMap(pPC);
		 }
		 else
		 {
			 sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
			 pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_BATTLEMAP));
		 }
	}
	default:
	{
		pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_BATTLEMAP));
	}
	break;
	}


}

void CCastleManager::ON_CM_Castle_Edit(CUnitPC* pPC, INT32 i32CastleIndex, INT64 i64Cost, INT32 i32Type, BOOL bOpen)
{
	if (pPC == NULL)
		return;
	SP_CASTLE_EDIT sendError;
	sendError.i32CastleIndex = i32CastleIndex;
	sendError.i32Type = i32Type;
	sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;

	if (i32Type == (INT32)ENUM_CP_CASTLE_EDIT_TYPE::REWARD_DUNGEON_OUT)
	{
		CCastle* pCastle = FindCastleRewardMapindex(pPC->GetMapIndex());
		if(pCastle)
			i32CastleIndex = pCastle->GetCastleIndex();
	}


	if (_mapCastleList.find(i32CastleIndex) == _mapCastleList.end())
	{
		sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		pPC->Write((BYTE*)&sendError,sizeof(SP_CASTLE_EDIT));
		return;
	}
	CCastle* pCastle = _mapCastleList.find(i32CastleIndex)->second;


	switch ((ENUM_CP_CASTLE_EDIT_TYPE)i32Type)
	{
		case ENUM_CP_CASTLE_EDIT_TYPE::DEFENCECOST_SETTING:
		{
			sendError.i32Result = pCastle->SettingDefenceCost(pPC,i64Cost);
		}
		break;
		case ENUM_CP_CASTLE_EDIT_TYPE::CLAN_MASTER_OUT:
		case ENUM_CP_CASTLE_EDIT_TYPE::CLAN_STORAGE_OUT:
		{
			CServerCastle* pServerCastle = dynamic_cast<CServerCastle*>(pCastle);
			if (pServerCastle == NULL)
			{
				sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
				pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
			}
			sendError.i32Result = pServerCastle->ON_CM_Castle_DutyOut(pPC, i32Type, i64Cost);

		}
		break;
		case ENUM_CP_CASTLE_EDIT_TYPE::REWARD_DUNGEON_ENTER:
		{
			sendError.i32Result = pCastle->EnterRewardDungeon(pPC);
		}
		break;
		case ENUM_CP_CASTLE_EDIT_TYPE::REWARD_DUNGEON_OUT:
		{
			sendError.i32Result = pCastle->OutRewardDungeon(pPC);
		}
		break;
		case ENUM_CP_CASTLE_EDIT_TYPE::SETTING_CASTLE_COST_DUNGEON:
		{
			sendError.i32Result = pCastle->SettingCastleTimeDungeon(pPC, i64Cost, bOpen);
		}
		break;
		case ENUM_CP_CASTLE_EDIT_TYPE::ADD_CASTLE_COST_DUNGEON_TIME:
		{
			sendError.i32Result = pCastle->BuyCastleTimeDungeonTime(pPC, i64Cost);
		}
		break;
	}

	if (sendError.i32Result != ENUM_ERROR_MESSAGE::SUCCESS)
	{
		pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
	}

}

void CCastleManager::ON_CastleDungeon_Buy_Compleate(CUnitPC* pPC, INT32 i32CastleIndex, INT32 i32Count)
{
	if (pPC == NULL)
		return;

	SP_CASTLE_EDIT SendError;
	SendError.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::DEFENCECOST_SETTING;
	SendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;


	if (_mapCastleList.find(i32CastleIndex) == _mapCastleList.end())
	{
		SendError.i32Result = 2;
		pPC->Write((BYTE*)&SendError, sizeof(SP_CASTLE_EDIT));
		return;
	}
	CCastle* pCastle = _mapCastleList.find(i32CastleIndex)->second;
	if (pCastle)
	{
		pCastle->ON_CastleDungeon_Buy_Compleate(pPC, i32Count);
		//CServerCastle * pServerCastle =dynamic_cast<CServerCastle*>(pCastle);
		//if (pServerCastle)
		//{
		//	pServerCastle->ON_CM_CastleInfo(pPC); //24.11.13_클라에서 한번더 보내달라하였음.
		//}
	}


}

void CCastleManager::ON_DefenceCost_Compleate(CUnitPC* pPC, INT32 i32CastleIndex, INT64 i64Cost)
{
	if (pPC == NULL)
		return;

	SP_CASTLE_EDIT SendError;
	SendError.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::DEFENCECOST_SETTING;
	SendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;


	if (_mapCastleList.find(i32CastleIndex) == _mapCastleList.end())
	{
		SendError.i32Result = 2;
		pPC->Write((BYTE*)&SendError, sizeof(SP_CASTLE_EDIT));
		return;
	}
	CCastle* pCastle = _mapCastleList.find(i32CastleIndex)->second;
	if (pCastle)
	{
		pCastle->DefenceCostCompleate(pPC, i64Cost);
		//CServerCastle * pServerCastle =dynamic_cast<CServerCastle*>(pCastle);
		//if (pServerCastle)
		//{
		//	pServerCastle->ON_CM_CastleInfo(pPC); //24.11.13_클라에서 한번더 보내달라하였음.
		//}
	}
}

void CCastleManager::ON_DSM_CastleDutyOut(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;

	DSP_CONSUME_CASTLE_CASH* pMsg = (DSP_CONSUME_CASTLE_CASH*)pPacket;
	if (_mapCastleList.find(pMsg->i32CastleIndex) == _mapCastleList.end())
	{
		return;
	}
	CCastle* pCastle = _mapCastleList.find(pMsg->i32CastleIndex)->second;

	switch (pCastle->GetCastleType())
	{
		case CASTLE_TYPE::SERVER_CASTLE:
		{
			CServerCastle* pServerCastle = dynamic_cast<CServerCastle*>(pCastle);
			if (pServerCastle == NULL)
			{
				return;
			}
			pServerCastle->ON_DSM_CastleDutyOut(pPacket);
		}
		break;
	}

}

void CCastleManager::ON_CM_CastleDeclareList(CUnitPC* pPC, INT32 i32CastleIndex)
{
	if (_mapCastleList.find(i32CastleIndex) == _mapCastleList.end())
	{
		return;
	}
	_mapCastleList.find(i32CastleIndex)->second->SendDeclareList(pPC);
}

void CCastleManager::OnBattleReady(INT32 i32CastleIdx)
{
	if (_mapCastleList.find(i32CastleIdx) == _mapCastleList.end())
		return;

	_mapCastleList.find(i32CastleIdx)->second->OnBattleReady();

	BroadCastCastleState(i32CastleIdx, _mapCastleList.find(i32CastleIdx)->second->GetCastleState());
}

void CCastleManager::OnBattleReadyCompute(INT32 i32CastleIdx)
{
	if (_mapCastleList.find(i32CastleIdx) == _mapCastleList.end())
		return;
	_mapCastleList.find(i32CastleIdx)->second->OnBattleReadyCompute();

	BroadCastCastleState(i32CastleIdx, _mapCastleList.find(i32CastleIdx)->second->GetCastleState());
}

void CCastleManager::OnBattleStart(INT32 i32CastleIdx)
{
	if (_mapCastleList.find(i32CastleIdx) == _mapCastleList.end())
		return;
	_mapCastleList.find(i32CastleIdx)->second->OnBattleStart();

	BroadCastCastleState(i32CastleIdx, _mapCastleList.find(i32CastleIdx)->second->GetCastleState());
}

void CCastleManager::OnBattleEnd(INT32 i32CastleIdx)
{
	if (_mapCastleList.find(i32CastleIdx) == _mapCastleList.end())
		return;
	_mapCastleList.find(i32CastleIdx)->second->OnBattleEnd();
	g_CastleManager.OnScheduleConnect(i32CastleIdx , (INT32)SCHEDULE_WORK_TYPE::CASTLE_END);
	BroadCastCastleState(i32CastleIdx, _mapCastleList.find(i32CastleIdx)->second->GetCastleState());
}

void CCastleManager::BroadCastCastleState(INT32 i32CastleIndex, CASTLE_TIME_STATE eType)
{
	// 모든유저에게 방송 
	auto timestamp = g_GameManager.GetTimeStamp_Second();
	auto serverdate = g_Global.GetCurTime();

	for (const auto& useriter : g_GameManager.mUserManager.m_pUsers)
	{
		SP_UICON_BROADCAST sendMsg;
		sendMsg.i32UIIndex = (INT32)ENUM_UICON_BROADCAST_TYPE::CASTLE;
		sendMsg.i32Value1 = i32CastleIndex;
		sendMsg.i32Value2 = eType;

		useriter->Write((BYTE*)&sendMsg,sizeof(SP_UICON_BROADCAST));
	}
}

void CCastleManager::SendBattleBagde(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	for (const auto& iter : _mapCastleList)
	{
		if (iter.second->GetCastleState() != CASTLE_TIME_STATE::CTS_NONE)
		{
			SP_UICON_BROADCAST sendMsg;
			sendMsg.i32UIIndex = (INT32)ENUM_UICON_BROADCAST_TYPE::CASTLE;
			sendMsg.i32Value1 = iter.first;
			sendMsg.i32Value2 = iter.second->GetCastleState();

			pPC->Write((BYTE*)&sendMsg, sizeof(SP_UICON_BROADCAST));

		}

	}


}

void CCastleManager::CastleNPCKill(CUnitNPC* pNpc, CUnitPC* pPC)
{
	//유닛은 필요에 따라서 내부에서 NULL체크한다
	if (pNpc == NULL)
		return;

	for (const auto& iter : _mapCastleList)
	{
		if (iter.second)iter.second->CastleNpcKillCheck(pNpc, pPC);
	}
}

BOOL CCastleManager::CheckNPCAttackTime(CUnitNPC* pNpc)
{
	if (pNpc == NULL)
		return TRUE;

	CCastle* pCastle = FindCastleMapindex(pNpc->GetMapIndex());
	if (pCastle)
	{
		return pCastle->CheckCastleNPCAttack(pNpc);
	}

	return TRUE;
}

BOOL CCastleManager::CheckDoorTile(INT32 i32Mapindex,MAP_TILE_TYPE eMapTile)
{
	CCastle* pCastle =  FindCastleMapindex(i32Mapindex);
	if (pCastle)
	{
		return pCastle->CheckMovableDoorTile(eMapTile);
	}

	return FALSE;
}

INT64 CCastleManager::GetDoorDamage(CUnitNPC* pNpc, INT64* pi64Damage)
{
	if (pNpc == NULL)
		return *pi64Damage;
	
	CCastle* pCastle = FindCastleMapindex(pNpc->GetMapIndex());
	if (pCastle)
	{
		return pCastle->GetDoorDamage(pNpc, pi64Damage);
	}


	return *pi64Damage;
}

void CCastleManager::BattleMapEndSetting(CUnitPC* pPC)
{
	CCastle* pCastle = FindCastleMapindex(pPC->GetMapIndex());
	if (pCastle)
	{
		pCastle->MapLoadEndSetting(pPC);
	}
}

BOOL CCastleManager::CheckCastleMap(INT32 i32Mapindex)
{
	for (const auto& iter : _mapCastleList)
	{
		if (iter.second->GetMapIndex() == i32Mapindex)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CCastleManager::CheckCastleRewardMap(INT32 i32Mapindex)
{
	for (const auto& iter : _mapCastleList)
	{
		if (iter.second->GetRewardDungeonMapIndex() == i32Mapindex)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CCastleManager::CheckCastleBattleCheck(INT32 i32GuildUnique)
{
	for (const auto& iter : _mapCastleList)
	{
		if (iter.second->CheckDeclareList(i32GuildUnique) == TRUE)
		{
			return TRUE;
		}
		if ((iter.second->GetCastleState() == CTS_BATTLE ||
			iter.second->GetCastleState() == CTS_READY ||
			iter.second->GetCastleState() == CTS_READY_TEN_MIN) &&
			iter.second->GetCastleClanUnique() == i32GuildUnique)
		{
			return TRUE;
		}
	}



	return FALSE;
}

BOOL CCastleManager::CheckClanAction(DWORD dwGuildUnique)
{
	for (auto iter : _mapCastleList)
	{
		if (iter.second->GetCastleState() != CASTLE_TIME_STATE::CTS_NONE)
		{
			if (iter.second->GetCastleClanUnique() == dwGuildUnique)
			{
				return FALSE;
			}
			else if (iter.second->CheckDeclareList(dwGuildUnique) == TRUE)
			{
				return FALSE;
			}

		}
	}
	return TRUE;
}

BOOL CCastleManager::CheckCastleBattleOpen()
{
	BOOL bOpen = FALSE;

	for (auto iter : _mapCastleList)
	{
		if (iter.second->GetNextStartTimeStamp() != 0)
		{
			bOpen = TRUE;
		}
	}


	return bOpen;
}

void CCastleManager::EraseCastleBuff(CUnitPC* pPC)
{
	for (auto iter : _mapCastleList)
	{
		iter.second->EraseCastleBuff(pPC);
	}
}

void CCastleManager::SummonCastleBossNPCIndex(INT32 i32RewardMapindex, INT32* pSummonBossIndex)
{
	if (pSummonBossIndex == NULL)
		return;

	CCastle* pCastle =FindCastleRewardMapindex(i32RewardMapindex);
	if (pCastle)
	{
		//일단은 이렇게 넣는다.
		pCastle->SummonBossLevel(NULL, pSummonBossIndex,NULL,NULL);
	}
}

void CCastleManager::SettingCastleBossNPC(CUnitNPC* pBossNPC)
{
	if (pBossNPC == NULL)
		return;

	CCastle* pCastle = FindCastleRewardMapindex(pBossNPC->GetMapIndex());
	if (pCastle)
	{
		pCastle->SetCastleBossNPC(pBossNPC);
	}
}

void CCastleManager::KillCastleBossNPC(CUnitNPC* pBossNpc)
{
	if (pBossNpc == NULL)
		return;

	CCastle* pCastle = FindCastleRewardMapindex(pBossNpc->GetMapIndex());
	if (pCastle)
	{
		pCastle->KillCastleBossNPC(pBossNpc);
	}
}

BOOL CCastleManager::HuntRewardDungeon(CUnitPC* pPC, INT32 i32ItemIndex)
{
	if (pPC == NULL)
		return FALSE;
	CCastle* pCastle = FindCastleRewardMapindex(pPC->GetMapIndex());
	if (pCastle)
	{
		return pCastle->CheckHuntNPC(pPC, i32ItemIndex);
	}

	return TRUE;
}

void CCastleManager::SendCastleDungeonInfo(CUnitPC* pPC)
{
	for (auto finditer : _mapCastleList)
	{
		finditer.second->SendCastleDungeonInfo(pPC);
	}
}



void CCastleManager::OccupationRuleEnterUser(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	for (const auto& iter : _mapIOccupationRuleList)
	{
		iter.second->EnterUser(pPC);
	}

}

void CCastleManager::OccupationRuleLoop()
{
	ULONGLONG curTm = GetTickCount64();

	_ullOccupationRuleCurTime = curTm;
	DWORD dwDelTick = _ullOccupationRuleCurTime - _ullOccupationRulePrevTime;

	for (const auto& iter : _mapIOccupationRuleList)
	{
		iter.second->IOccupationRuleLoop(dwDelTick);
	}

	_ullOccupationRulePrevTime = _ullOccupationRuleCurTime;
}



BOOL CCastleManager::ClanFlagRallyItemFailCheck(CUnitPC* pPC, DWORD ClanUnique)
{
	if (pPC == NULL)
		return TRUE;

	CCastle* pCastle = FindCastleMapindex(pPC->GetMapIndex());
	if (pCastle)
	{
		if (_mapIClanFlagRuleList.find(pCastle->GetCastleIndex()) != _mapIClanFlagRuleList.end())
		{
			return _mapIClanFlagRuleList.find(pCastle->GetCastleIndex())->second->RallyItemCheck(pPC, ClanUnique);
		}
	}


	return  ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_CASTLEMAP;
}

void CCastleManager::ClanFlagUseRallyItem(CUnitPC* pPC, DWORD ClanUnique)
{
	if (pPC == NULL)
		return;

	CCastle* pCastle = FindCastleMapindex(pPC->GetMapIndex());
	if (pCastle)
	{
		if (_mapIClanFlagRuleList.find(pCastle->GetCastleIndex()) != _mapIClanFlagRuleList.end())
		{
			_mapIClanFlagRuleList.find(pCastle->GetCastleIndex())->second->UseRallyItem(pPC, ClanUnique);
		}
	}
	
}

void CCastleManager::ClanFlagWarpClanFlag(CUnitPC* pPC)
{
	CCastle* pCastle = FindCastleMapindex(pPC->GetMapIndex());
	if (pCastle)
	{
		if (_mapIClanFlagRuleList.find(pCastle->GetCastleIndex()) != _mapIClanFlagRuleList.end())
		{
			_mapIClanFlagRuleList.find(pCastle->GetCastleIndex())->second->WarpClanFlag(pPC);
		}
	}
}

void CCastleManager::ClanFlagSendClanOpenFlag(CUnitPC* pPC)
{
	CCastle* pCastle = FindCastleMapindex(pPC->GetMapIndex());
	if (pCastle)
	{
		if (_mapIClanFlagRuleList.find(pCastle->GetCastleIndex()) != _mapIClanFlagRuleList.end())
		{
			_mapIClanFlagRuleList.find(pCastle->GetCastleIndex())->second->SendClanOpenFlag(pPC);
		}
	}
}

void CCastleManager::Loop()
{
	ULONGLONG curTm = GetTickCount64();

	_ullCurTime = curTm;
	DWORD dwDelTick = _ullCurTime - _ullPrevTime;


	for (const auto& iter : _mapCastleList)
	{
		iter.second->Loop(dwDelTick);
	}

	_ullPrevTime = _ullCurTime;
}

void CCastleManager::SaveDB()
{
	for (auto iter : _mapCastleList)
	{
		if (iter.second->GetDBLoad() == TRUE)
		{
			iter.second->SaveDB();
		}
	}

}
