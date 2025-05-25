#include "../server_Common/TIStruct.h"
#include "../server_Common/TIStructServer.h"
#include "../server_Common/TIDefineServer.h"
#include "../server_Common/TIDefine.h"
#include "../server_Common/TIprotocolServer.h"
#include "../server_Common/TIprotocol.h"
#include<map>
#include<chrono>
#include<vector>


#include"UnitPC.h"
#include "NpcManager.h"
#include "GameManager.h"
#include "CastleManager.h"
#include "DungeonManager.h"
#include "BuffManager.h"
#include "MailManager.h"
#include "../server_Common/INIFile_Server.h"

#include "Castle.h"

#include"ServerCastle.h"

void CServerCastle::Loop(DWORD dwDeltick)
{
	//전투 종료후 내쫓는 로직
	if (_bEndTime == TRUE)
	{
		if (_ui32EndOutBattleTime >= dwDeltick)
		{
			_ui32EndOutBattleTime -= dwDeltick;
		}
		else
		{
			_ui32EndOutBattleTime = 0;
		}
		if (_ui32EndOutBattleTime == 0)
		{
			CastleEndOutBattleMap();
			_bEndTime = FALSE;
		}
	}
	//세금인출 시간용 루프(11시고정 -> 변경시 바꿀것)
	if (_bTaxOutTime == TRUE)
	{
		if (_ui32TaxOutTimeTick >= dwDeltick)
		{
			_ui32TaxOutTimeTick -= dwDeltick;
		}
		else
		{
			_ui32TaxOutTimeTick = 0;
		}
		if (_ui32TaxOutTimeTick == 0)
		{
			_bTaxOutTime = FALSE;
			ReturnTax();
		}
	}

	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
		return;

	//방송처리
	if (_ui32BroadCastTimeTick >= dwDeltick)
	{
		_ui32BroadCastTimeTick -= dwDeltick;
	}
	else
	{
		_ui32BroadCastTimeTick = 0;
	}
	if (_ui32BroadCastTimeTick == 0)
	{
		BroadCastBattleTime();
	}
	INT32 i32Min = (ui32CastleBattleTimeTick / 1000) / 60;
	if (i32Min <= 9) //10분전부터
	{
		if (i32Min != _i32BattleMin)
		{
			CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::VAL_MIN, NULL, i32Min+1); //승리
			_i32BattleMin = i32Min;
		}
	}

	//배틀타임
	if (ui32CastleBattleTimeTick >= dwDeltick)
	{
		ui32CastleBattleTimeTick -= dwDeltick;
		//틱이 오버플로우 되면, 강제종료처리
		if (GetEndBattleTimeStamp() < g_GameManager.GetTimeStamp_Second())
		{
			ui32CastleBattleTimeTick = 0;
		}
	}
	else
	{
		ui32CastleBattleTimeTick = 0;
	}
	//전투 종료
	if (ui32CastleBattleTimeTick == 0)
	{
		OnBattleEnd();
		return;
	}
	//IOccupationRuleLoop(dwDeltick); -> 100틱으로 검사필요

	for (auto iter = _mapClanRallyData.begin(); iter != _mapClanRallyData.end(); )
	{
		if (iter->second.dwRemainTick >= dwDeltick)
		{
			iter->second.dwRemainTick -= dwDeltick;
		}
		else
		{
			iter->second.dwRemainTick = 0;
		}

		if (iter->second.dwRemainTick == 0)
		{
			if (_mapvecUseWarpCount.find(iter->first) != _mapvecUseWarpCount.end())
			{
				_mapvecUseWarpCount.find(iter->first)->second.clear();
				_mapvecUseWarpCount.erase(iter->first);
			}
			BroadCastEndClanFlag(iter->first);
			iter = _mapClanRallyData.erase(iter); // 요소를 제거하고 다음 반복자로 이동
		}
		else
		{
			++iter; // 요소를 제거하지 않으면 반복자를 수동으로 증가
		}
	}


}

void CServerCastle::SaveDB()
{
	if (GetMasterClan())
	{
		GetSaveInfo()->stClanSimbol = GetMasterClan()->GetClanSimbolIdx();
		memcpy(GetSaveInfo()->_wcOwnerCharName, GetMasterClan()->GetMasterName(), sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
	}


	DCP_CASTLE_SAVE sendSaveData;
	sendSaveData._i32CastleIndex = GetCastleIndex();
	memcpy(&sendSaveData._stSaveInfo, GetSaveInfo(), sizeof(stCastleSaveInfo));

	g_Server.SendMsg(g_ServerArray.Bravo_GameDBServer1, (BYTE*)&sendSaveData, sizeof(DCP_CASTLE_SAVE));

	if (GetCastleState() == CASTLE_TIME_STATE::CTS_BATTLE)
	{
		CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::REMAINTIME, 0, ui32CastleBattleTimeTick);
	}
}

void CServerCastle::ON_Load_SaveData(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;

	DCP_LOAD_CASTLE_BATTLE_DATA* pMsg = (DCP_LOAD_CASTLE_BATTLE_DATA*)pPacket;

	switch ((ENUM_CASTLE_BATTLE_SAVE)pMsg->i32Type)
	{
	case ENUM_CASTLE_BATTLE_SAVE::MONSTER_INFO:
	{
		if (GetCastleState() != CTS_READY_TEN_MIN && GetCastleState() != CTS_BATTLE)
			return;

		//서버켜질시 몬스터를 소환-> 여기서 한마리씩 지우고 킬체크 추가해준다.
		auto findIter =_mapNpc.find(pMsg->i32SaveIndex);
		if (findIter == _mapNpc.end())
			return;
		findIter->second->SetDieAlone(TRUE);
		findIter->second = NULL;
		_mapNpc.erase(pMsg->i32SaveIndex);

		if (pMsg->i32Value1 != 0)
		{
			for (auto iter : _vecMonsterKillArray)
			{
				if (iter.i32Charunique == pMsg->i32Value1)
				{
					iter.i32KillCount++;
					return;
				}
			}
			stMonsterKillInfo killcount;
			killcount.i32Charunique = pMsg->i32Value1;
			killcount.i32KillCount = 1;
			killcount.bReEnter = TRUE;
			_vecMonsterKillArray.push_back(killcount);
		}
	}
	break;
	case ENUM_CASTLE_BATTLE_SAVE::ENTER_PC:
	{
		if (GetCastleState() != CTS_READY_TEN_MIN && GetCastleState() != CTS_BATTLE)
			return;

		stEnterPCInfo stChar;
		stChar.dwCharunique = pMsg->i32SaveIndex;
		stChar.dwClanUnique = pMsg->i32Value1;
		_vecBattlePCInfo.push_back(stChar);
	}
	break;
	case ENUM_CASTLE_BATTLE_SAVE::DEFAULT_BATTLEENDTIME:
	{
		if (GetCastleState() != CTS_READY_TEN_MIN && GetCastleState() != CTS_BATTLE)
			return;

		SetEndBattleTimeStamp(pMsg->ui32Value2);
	}
	break;
	case ENUM_CASTLE_BATTLE_SAVE::REMAINTIME :
	{
		if (GetCastleState() != CTS_READY_TEN_MIN && GetCastleState() != CTS_BATTLE)
			return;

		ui32CastleBattleTimeTick = pMsg->i32Value1;
	}
	break;
	case ENUM_CASTLE_BATTLE_SAVE::CLAN_RALLY_DATA :
	{

	}
	break;
	case ENUM_CASTLE_BATTLE_SAVE::CLAN_RALLY_USE_COUNT:
	{
		if (GetCastleState() != CTS_READY_TEN_MIN && GetCastleState() != CTS_BATTLE)
			return;

		if (_mapUserRallyUseCount.find(pMsg->i32SaveIndex) != _mapUserRallyUseCount.end())
		{
			_mapUserRallyUseCount.find(pMsg->i32SaveIndex)->second++;
		}
		else
		{
			_mapUserRallyUseCount.insert(make_pair(pMsg->i32SaveIndex, 1));
		}
	}
	break;
	case ENUM_CASTLE_BATTLE_SAVE::CASTLE_TAX_OUT_TIME:
	{
		if (GetCastleState() != CTS_NONE)
			return;

		if (_bTaxOutTime == FALSE)
		{
			ON_TAX_OutTime();
		}
	}
	break;
	}

}

void CServerCastle::OnBattleReady()
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_NONE)
		return;
	//사용중이던 사람들 자동퇴출
	OutRewardDungeonMap();

	SetCastleState(CASTLE_TIME_STATE::CTS_READY);
}

void CServerCastle::OnBattleReadyCompute()
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_READY)
		return;

	//NPC소환_성문은 못 때리게, BuffNPC(천군)사냥 가능
	SummonNpcList();
	ui32CastleBattleTimeTick = (GetEndBattleTimeStamp()-GetNextStartTimeStamp()) * 1000;

	//SetEndBattleTimeStamp(EndBattleTimeStamp);
	CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::DEFAULT_BATTLEENDTIME, 0, 0, GetEndBattleTimeStamp());
	CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::REMAINTIME, 0, ui32CastleBattleTimeTick);
	

	SetCastleState(CASTLE_TIME_STATE::CTS_READY_TEN_MIN);
}

void CServerCastle::OnBattleStart()
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_READY_TEN_MIN)
		return;

	//천군 제거
	BuffNPCALLKill();
	_ui32BroadCastTimeTick = TIME_MIN * 1000;
	CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::START_BATTLE,NULL,0);


	BroadCastNPCAttackTime(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A);
	BroadCastNPCAttackTime(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B);
	BroadCastNPCAttackTime(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C);

	SetCastleState(CASTLE_TIME_STATE::CTS_BATTLE);

}

void CServerCastle::OnBattleEnd()
{

	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
		return;


	SetCastleState(CASTLE_TIME_STATE::CTS_BATTLE_END);

	BattleEnd();
	BattleEndReward();
	InitBattle();
	SettingEndTimeOut();
	g_CastleManager.OnScheduleConnect(GetCastleIndex(),(INT32)SCHEDULE_WORK_TYPE::CASTLE_START); //다음 공성 시작일 요청
	ON_TAX_OutTime();
	UpdateCastleCostDungeonSummonList();
	SetCastleState(CASTLE_TIME_STATE::CTS_NONE);
}

void CServerCastle::CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE eType, CUnitPC* pPC, INT32 i32Value, BOOL bBroadCast)
{
	CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
	if (pMap == NULL)
		return;

	SP_MESSAGE_BROADCAST sendMsg;
	sendMsg.wIndex = (WORD)ENUM_UICON_BROADCAST_TYPE::CASTLE;
	sendMsg.wType = (WORD)eType;
	if (pPC != NULL)
	{
		memcpy(sendMsg.wcName, pPC->GetName(), sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
	}
	sendMsg.i32Value1 = i32Value;


	if (bBroadCast == TRUE)
	{
		pMap->m_BlockManager.BroadCast_AllBlock((BYTE*)&sendMsg, sizeof(SP_MESSAGE_BROADCAST), TRUE);
	}
	else
	{
		if (pPC)
		{
			//클랜이름넣어주기
			if (eType == ENUM_BROADCAST_MESSAGE_TYPE::BATTLE_END)
			{
				if(GetMasterClan())
				{
					memcpy(sendMsg.wcName, GetMasterClan()->GetClanName(), sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
				}
			}

			pPC->Write((BYTE*)&sendMsg, sizeof(SP_MESSAGE_BROADCAST));
		}
	}

}

void CServerCastle::ON_CM_CastleInfo( CUnitPC* pPC)
{
	if ( pPC == NULL)
		return;

	SP_CASTLE_INFO sendFailData;
	sendFailData.i32Type = (INT32)ENUM_CP_CASTLE_INFO_TYPE::INFO;
	sendFailData.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	if (FALSE == g_ClientArray[g_ServerArray.Bravo_CashDB].IsConnected())
	{
		pPC->Write((BYTE*)&sendFailData, sizeof(SP_CASTLE_INFO));
		return;
	}

	DCP_SIEGE_READY_DATA sendMsg;
	sendMsg.i32Charunique = pPC->GetCharacterUnique();
	sendMsg.i32Group = pPC->GetGroupId();
	sendMsg.i32CastleIndex =GetCastleIndex();
	pPC->WriteGameDB((BYTE*)&sendMsg, sizeof(DCP_SIEGE_READY_DATA));

}

void CServerCastle::ON_Send_CastleInfo(DWORD dwCharunique, INT64 i64GoldBalance, INT64 i64CastleDungeonBalance)
{

	CUnitPC* pClient = g_GameManager.mUserManager.FindUser(dwCharunique);
	if (pClient ==NULL)
	{
		return;
	}
	SP_CASTLE_INFO sendMsg;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_INFO_TYPE::INFO; //인포요청
	sendMsg.i64CastleBalance = i64GoldBalance;
	sendMsg.i64CastleDungeonBalance = i64CastleDungeonBalance;
	//25.02.19_임시로_공성전에서 공격력감소
	sendMsg.i32PVPPercent = GetDebuffPer();

	sendMsg.ui32BattleStartTimeStmap = GetNextStartTimeStamp();
	sendMsg.i32MaxMember = TotalBattleMember();
	if (_mapLevelInfo.find(GetCastleLevel()) != _mapLevelInfo.end())
	{
		sendMsg.i32MaxRewardItemCount = _mapLevelInfo.find(GetCastleLevel())->second.i32MaxDrawItemCnt;
		sendMsg.i32MyRewardItemCount = pClient->m_DungeonList.GetServerCastletodayCount();
	}

	memcpy(&sendMsg.stSaveData,GetSaveInfo(), sizeof(stCastleSaveInfo));
	sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::NONE;
	CClan* pClan = pClient->GetClan();
	if (pClan == NULL)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
		sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::NONE;
		pClient->Write((BYTE*)&sendMsg,sizeof(SP_CASTLE_INFO));
		return;
	}

	if (GetMasterClan() != NULL)  //수성
	{
		if (pClient->GetCharacterUnique() == GetMasterClan()->GetMasterunique())
		{
			sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::CASTLE_MASTER;
		}
		else if (pClan->GetClanUnique() == GetMasterClan()->GetClanUnique())
		{
			sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::CASTLE_MEMBER;
		}
	}
	if (pClan->GetMasterunique() == pClient->GetCharacterUnique() && GetMasterClan()!= pClan)  //클랜장인지?
	{
		if (CheckDeclareList(pClan->GetClanUnique()) == TRUE)
		{
			sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::DECLARE_COMPLEATE;
		}
		else
		{
			sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::DECLARE_POSSIBLE_MASTER;
		}
	}
	else  if(pClan->GetMasterunique() != pClient->GetCharacterUnique() && GetMasterClan() != pClan)
	{
		if (CheckDeclareList(pClan->GetClanUnique()) == TRUE)
		{
			sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::DECLARE_COMPLEATE;
		}
		else
		{
			sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::NONE;
		}
	}

	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
	pClient->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_INFO));

}

ENUM_ERROR_MESSAGE CServerCastle::ON_CM_Castle_DutyOut(CUnitPC* pPC, INT32 i32Type , INT64 i64Cost)
{
	if (pPC == NULL )
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;


	//SP_CASTLE_EDIT sendError;
	//sendError.i32Type = i32Type;
	//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;


	CClan* pClan = pPC->GetClan();
	if (pClan == NULL
		|| pClan->GetMasterunique() != pPC->GetCharacterUnique() 
		|| GetMasterClan() == NULL 
		|| GetMasterClan() != pClan)
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CLAN_NOT_MANAGER;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CLAN_NOT_MANAGER;
	}
	if (GetCastleState() != CTS_NONE)
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}

	if (_bTaxOutTime != TRUE)
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}

	if (FALSE == g_ClientArray[g_ServerArray.Bravo_CashDB].IsConnected())
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}
	if (GetCastleState() != CTS_NONE)
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_SETTING_NOT;
	}

	if (i64Cost <= 0)
	{
		//sendError.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
		//pPC->Write((BYTE*)&sendError, sizeof(SP_CASTLE_EDIT));
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	}

	DCP_CONSUME_CASTLE_CASH sendCash;
	sendCash.i32CastleIndex = GetCastleIndex();
	sendCash.dwCharunique = pPC->GetCharacterUnique();
	sendCash.dwClanUnique = GetMasterClan()->GetClanUnique();
	sendCash.dwAccunique = pPC->GetAccountUnique();
	sendCash.i32Type = i32Type;
	sendCash.i64Cost = i64Cost;
	sendCash.i32GroupServerIdx = g_INIFile_Server.m_iServerGroupID;

	g_Server.SendMsg(g_ServerArray.Bravo_CashDB, (BYTE*)&sendCash, sizeof(DCP_CONSUME_CASTLE_CASH));

	CLogManager::CASTLE_TAX_OUT(GetCastleIndex(), pPC->GetCharacterUnique(), i32Type, 0, 0,i64Cost);
	return ENUM_ERROR_MESSAGE::SUCCESS;
}

void CServerCastle::ON_DSM_CastleDutyOut(BYTE* pPacket)
{
	if (pPacket == NULL)
		return;

	if (FALSE == g_ClientArray[g_ServerArray.Bravo_CashDB].IsConnected())
	{
		//TODO::
		return;
	}

	DSP_CONSUME_CASTLE_CASH* pMsg = (DSP_CONSUME_CASTLE_CASH*)pPacket;
	SP_CASTLE_EDIT sendMsg;
	sendMsg.i32Type = (INT32)pMsg->wType;
	sendMsg.i64CostResult = pMsg->i64Cost;
	sendMsg.i32CastleIndex = pMsg->i32CastleIndex;
	sendMsg.i32Result = pMsg->wResult;
 	CUnitPC* pPC =  g_GameManager.mUserManager.FindUser(pMsg->dwCharunique);
	CLogManager::CASTLE_TAX_OUT(GetCastleIndex(), pMsg->dwCharunique, (INT32)pMsg->wType, 1,pMsg->wResult, pMsg->i64Cost);
	if (pMsg->wResult != ENUM_ERROR_MESSAGE::SUCCESS)
	{
		if (pPC)pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_EDIT));
		return;
	}

	if (pMsg->wType == (INT32)ENUM_CP_CASTLE_EDIT_TYPE::CLAN_MASTER_OUT)
	{
		if (pPC)
		{
			pPC->SetDark(pMsg->i64Cost,0,L"Castle_Duty",0, (BYTE)(CASH_DEPOSIT_TYPE::CASTLE_CASH_DEPOSIT),0);
			//캐슬인포 보내주게 변경
	
			DCP_SIEGE_READY_DATA sendMsg;
			sendMsg.i32Charunique = pPC->GetCharacterUnique();
			sendMsg.i32Group = pPC->GetGroupId();
			sendMsg.i32CastleIndex = GetCastleIndex();
		    pPC->WriteGameDB((BYTE*)&sendMsg, sizeof(DCP_SIEGE_READY_DATA));
			
		}
		else
		{
			DCP_REQUEST_CASH sendCashmsg;
			sendCashmsg.m_byType = ENUM_CASH_DB_GM_CASH_TYPE::ENUM_CASH_DB_REQUEST_CASH_TYPE_CHARGE_DARK;
			sprintf(sendCashmsg.m_eTypeStr, "CASTLE_CASH_DEPOSIT");
			sendCashmsg.m_eType = CASH_TYPE::CASTLE_DEPOSIT;
			sendCashmsg.m_i32UserType = CASH_USER_TYPE::ENUM_USER_TYPE_NOMAL;
			sendCashmsg.m_dwBundleUnique = 0;
			sendCashmsg.m_AccUnique = pMsg->dwAccunique;
			sendCashmsg.m_dwFieldUnique = pMsg->dwCharunique;
			sendCashmsg.m_CharUnique = pMsg->dwCharunique;
			sendCashmsg.m_dwPrice = pMsg->i64Cost;
			sendCashmsg.m_dwProductID = 0;
			sendCashmsg.m_byDepositType = (BYTE)(CASH_DEPOSIT_TYPE::CASTLE_CASH_DEPOSIT);

			
			wcsncpy(sendCashmsg.m_wProductName, L"Castle_Duty", MAX_ITEM_NAME_LENGTH);
			wcsncpy(sendCashmsg.m_wcName, L"", LENGTH_CHARACTER_NICKNAME);

			g_Server.SendMsg(g_ServerArray.Bravo_CashDB, (BYTE*)&sendCashmsg, sizeof(DCP_REQUEST_CASH));


		}
	}
	else if (pMsg->wType == (INT32)ENUM_CP_CASTLE_EDIT_TYPE::CLAN_STORAGE_OUT)
	{
		CClan* pClan = g_ClanManager.FindByClanUnique(pMsg->dwClanUnique);
		if (pClan)
		{
			pClan->AddGold(pMsg->i64Cost, pPC,CLAN_MONEY_GOLD_TYPE::CLAN_MONEY_CASTLE_OWNER_BACK_GOLD,pMsg->dwCharunique);
			
			//캐슬인포 보내주게 변경
			if (pPC)
			{
				DCP_SIEGE_READY_DATA sendMsg;
				sendMsg.i32Charunique = pPC->GetCharacterUnique();
				sendMsg.i32Group = pPC->GetGroupId();
				sendMsg.i32CastleIndex = GetCastleIndex();
				pPC->WriteGameDB((BYTE*)&sendMsg, sizeof(DCP_SIEGE_READY_DATA));
			}
		}
		else
		{
			CLogManager::CASTLE_TAX_OUT(GetCastleIndex(), pMsg->dwCharunique, (INT32)pMsg->wType, 2, pMsg->wResult, pMsg->i64Cost); //에러위치 찾기용
		}
	}
	else if (pMsg->wType == (INT32)ENUM_CP_CASTLE_EDIT_TYPE::CASTLE_LOSE_OUT)
	{
		CClan* pClan = g_ClanManager.FindByClanUnique(pMsg->dwClanUnique);
		if (pClan)
		{
			pClan->AddGold(pMsg->i64Cost, pPC, CLAN_MONEY_GOLD_TYPE::CLAN_MONEY_CASTLE_OWNER_BACK_GOLD, pMsg->dwCharunique);
		}
		else
		{
			CLogManager::CASTLE_TAX_OUT(GetCastleIndex(), pMsg->dwCharunique, (INT32)pMsg->wType, 4, pMsg->wResult, pMsg->i64Cost); //에러위치 찾기용
		}
	}
	else
	{
		CLogManager::CASTLE_TAX_OUT(GetCastleIndex(), pMsg->dwCharunique, (INT32)pMsg->wType, 3, pMsg->wResult, pMsg->i64Cost); //에러위치 찾기용
	}

}

void CServerCastle::ON_TAX_OutTime()
{
	DWORD dwEndTimeStamp =g_Global.MakeTimeStamp(23,0); //11시로 고정
	//이게 이상할경우...?출금 막기
	if (dwEndTimeStamp > g_GameManager.GetTimeStamp_Second())
	{
		CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::CASTLE_TAX_OUT_TIME, 0, 0);
		_ui32TaxOutTimeTick = (dwEndTimeStamp - g_GameManager.GetTimeStamp_Second()) * 1000;
		_bTaxOutTime = TRUE;
	}
}

void CServerCastle::ReturnTax()
{
	if (GetMasterClan())
	{
		DCP_CONSUME_CASTLE_CASH sendCash;
		sendCash.i32CastleIndex = GetCastleIndex();
		sendCash.dwCharunique = GetMasterClan()->GetMasterunique();
		sendCash.dwClanUnique = GetMasterClan()->GetClanUnique();
		sendCash.dwAccunique = 0;
		sendCash.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::CASTLE_LOSE_OUT;
		sendCash.i64Cost = 0;
		sendCash.i32GroupServerIdx = g_INIFile_Server.m_iServerGroupID;

		g_Server.SendMsg(g_ServerArray.Bravo_CashDB, (BYTE*)&sendCash, sizeof(DCP_CONSUME_CASTLE_CASH));
		CLogManager::CASTLE_TAX_OUT(GetCastleIndex(), GetMasterClan()->GetMasterunique(), (INT32)ENUM_CP_CASTLE_EDIT_TYPE::CASTLE_LOSE_OUT, 0, 0, 0);
	}
	DCP_SAVE_CASTLE_BATTLE_DATA sendDelete;
	sendDelete.i32CastleIndex = GetCastleIndex();
	sendDelete.i32AllDelete = TRUE;
	g_Server.SendMsg(g_ServerArray.Bravo_GameDBServer1, (BYTE*)&sendDelete, sizeof(DCP_SAVE_CASTLE_BATTLE_DATA));
}



void CServerCastle::Init()
{
	_i32MoneyTaxPercent = 0;
	_vecMonsterKillArray.clear();
	_mapClanRallyData.clear();
	_mapUserRallyUseCount.clear();
	_pCastleMaster = NULL;
	IOccupationRuleInit();
	_ui32BroadCastTimeTick = 0;
	_i32BattleMin = 0;
	_bEndTime = FALSE;
	_ui32EndOutBattleTime = 0;
	_bTaxOutTime = FALSE;
	_ui32TaxOutTimeTick = 0;
}


void CServerCastle::SendBattlMap(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	SP_CASTLE_BATTLEMAP sendMsg;
	sendMsg.i32Team = (INT32)ENUM_TEAM::NONE;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	sendMsg.i32Type = (INT32)ENUM_CASTLE_BATTLEMAP_TYPE::WARP_WINDOW;
	sendMsg.i32BuffNpcCount = GetBuffNPCCount();
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A);
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B);
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C);
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_RIGHT_2] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_RIGHT_2);
	sendMsg.ui32BattleEndTimeStamp = GetEndBattleTimeStamp();


	CClan* pClan = pPC->GetClan();
	if (pClan == NULL)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	if (pClan->GetClanUnique() == GetSaveInfo()->_ui32OwnerClanunique)
	{
		sendMsg.i32Team = (INT32)ENUM_TEAM::DEFENDER;
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}
	else if (CheckDeclareList(pClan->GetClanUnique()) == TRUE)
	{
		sendMsg.i32Team = (INT32)ENUM_TEAM::ATTACKER;
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}


	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
}

void CServerCastle::WarpBattleMap(CUnitPC* pPC, INT32 i32WarpIndex)
{
	if (pPC == NULL)
	{
		return;
	}

	SP_CASTLE_BATTLEMAP sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CASTLE_BATTLEMAP_TYPE::WARP;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;

	if (pPC->GetClan() == NULL)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	//던전 사용중
	if (pPC->m_DungeonList.CheckDungeonNow())
	{
		sendMsg.i32Result = DUNGEON_ENTER_ERROR::DUNGEON_ERROR_TRYING; // 이미 던전에 들어와있다. 
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}
	if (g_DungeonManager.CheckWaveDungeonMap(pPC->GetMapIndex()) == TRUE)
	{
		sendMsg.i32Result = DUNGEON_ENTER_ERROR::DUNGEON_ERROR_TRYING; // 이미 던전에 들어와있다. 
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	//워프중
	if (pPC->m_bWARP_PENDING == TRUE)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}
	if (pPC->GetCurrentMap() == NULL || pPC->GetCurrentMap()->GetMapTYPE() != eMAP_TYPE::MAP_TYPE_NORMAL)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	//공성중 아님
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE &&
		GetCastleState() != CASTLE_TIME_STATE::CTS_READY_TEN_MIN)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}
	//파티중
	if (pPC->GetParty())
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_PARTY_ALREADY_JOIN;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}


	ENUM_TEAM eTeam = ENUM_TEAM::NONE;
	if (pPC->GetClanUnique() == GetSaveInfo()->_ui32OwnerClanunique)
	{
		eTeam = ENUM_TEAM::DEFENDER;
	}
	else if (CheckDeclareList(pPC->GetClanUnique()) == TRUE)
	{
		eTeam = ENUM_TEAM::ATTACKER;
	}
	else
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NOT_DECLARE;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	if (_mmvCasterWarpInfo.find((INT32)eTeam) == _mmvCasterWarpInfo.end() || 
		_mmvCasterWarpInfo.find((INT32)eTeam)->second.find(i32WarpIndex) == _mmvCasterWarpInfo.find((INT32)eTeam)->second.end())
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_WARP_NOT_TEAM;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	vector<stCastleWarp> _vecRandWarp = _mmvCasterWarpInfo.find((INT32)eTeam)->second.find(i32WarpIndex)->second;


	INT32 i32Rand = g_GameManager.getRandomNumber(0, _vecRandWarp.size() - 1);
	INT32 i32WarpPosX = _vecRandWarp.at(i32Rand).i32X;
	INT32 i32WarpPosY = _vecRandWarp.at(i32Rand).i32Y;


	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
	sendMsg.i32Team = (INT32)eTeam;

	if (pPC->GetMapIndex() == GetMapIndex())
	{
		if (pPC->GetAlive() == FALSE)
		{
			pPC->SetForceAlive();
		}
		sendMsg.i32Type= (INT32)ENUM_CASTLE_BATTLEMAP_TYPE::RESURRECTION;
	}
	else
	{
		pPC->SetCastleWarpOldPos();
		AddBattleMapUser(pPC); //입장리스트 추가
		WarpEndReBuffSetting(pPC); //서버 종료후 재입장이면 버프걸어줌.

		//25.01.15_남일우_공성 타일활성화를 위해 늦게들어온 사람에게도 알려달라함.
		//if (_pCastleMaster != NULL)
		//{
		//	SP_OCCUPATION_STATE sendTime;
		//	sendTime.i32RemainTimeTick = _ui32OccupationTimeTick;
		//	sendTime.wState = (WORD)ENUM_SP_OCCUPATION_STATE::COMPLEATE;
		//	pPC->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
		//}

		CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
		if (pMap)
		{
			pMap->EnterUser(pPC);
		}
		CLogManager::CASTLE_IN_LOG(GetCastleIndex(),pPC, GetMapIndex(), i32WarpIndex);
	}

	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
	g_MapManager.UIWarp(pPC, 0, GetMapIndex(), i32WarpPosX, i32WarpPosY, FALSE);


	pPC->SetTeamUnique(GetTeamunique(pPC), GetTeamunique2(pPC), GetCastleIndex());
}

void CServerCastle::AddBattleMapUser(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;
	if (pPC->GetClan() == NULL)
		return;

	for (const auto& iter : _vecBattlePCInfo)
	{
		if (iter.dwCharunique == pPC->GetCharacterUnique())
			return;
	}

	stEnterPCInfo stChar;
	stChar.dwCharunique = pPC->GetCharacterUnique();
	stChar.dwClanUnique = pPC->GetClanUnique();

	_vecBattlePCInfo.push_back(stChar);

	
	CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::ENTER_PC, stChar.dwCharunique, stChar.dwClanUnique);
}

void CServerCastle::ShowBattleInfo(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	SP_CASTLE_BATTLEMAP sendMsg;
	sendMsg.i32Team = (INT32)ENUM_TEAM::NONE;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	sendMsg.i32Type = (INT32)ENUM_CASTLE_BATTLEMAP_TYPE::BATTLE_WINDOW;
	sendMsg.i32BuffNpcCount = GetBuffNPCCount();
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A);
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B);
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C);
	sendMsg.byDoorNpcHPpercent[(INT32)CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_RIGHT_2] = GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_RIGHT_2);
	sendMsg.ui32BattleEndTimeStamp = GetEndBattleTimeStamp();

	CClan* pClan = pPC->GetClan();
	if (pClan == NULL)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	if (pClan->GetClanUnique() == GetSaveInfo()->_ui32OwnerClanunique)
	{
		sendMsg.i32Team = (INT32)ENUM_TEAM::DEFENDER;
	}
	if (CheckDeclareList(pClan->GetClanUnique()) == TRUE)
	{
		sendMsg.i32Team = (INT32)ENUM_TEAM::ATTACKER;
	}


	SP_CASTLE_BATTLEMAP_CLANINFO sendClanINfo;
	sendClanINfo.wUseFlagPointX = MyClanRallyAxisX(pClan->GetClanUnique());
	sendClanINfo.wUseFlagPointY = MyClanRallyAxisY(pClan->GetClanUnique());


	list<CUnitPC*>* pMemberList = pClan->GetOnlineMemberList();

	if (pMemberList == NULL)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CLAN_NULL;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}

	INT32 i32ArrayIdx = 0;
	for (const auto& iter : *pMemberList)
	{
		if (iter == NULL)
			continue;
		if (iter->GetMapIndex() == GetMapIndex())
		{
			sendClanINfo.stClanList[i32ArrayIdx].byCurState  = iter->GetAlive();
			sendClanINfo.stClanList[i32ArrayIdx].byGrade = pClan->GetMemberGrade(iter->GetCharacterUnique());
			sendClanINfo.stClanList[i32ArrayIdx].byHpPercent = iter->GetHpPercent();
			sendClanINfo.stClanList[i32ArrayIdx].byMpPercent = iter->GetMPPercent();
			sendClanINfo.stClanList[i32ArrayIdx].wMemberX = iter->m_X;
			sendClanINfo.stClanList[i32ArrayIdx].wMemberY = iter->m_Y;
			memcpy(sendClanINfo.stClanList[i32ArrayIdx]._wcName, iter->GetName(),sizeof(WCHAR)* LENGTH_CHARACTER_NICKNAME);

			i32ArrayIdx++;
		}
	}

	//정렬은 클라에서하기로함.
	//int n = i32ArrayIdx; // 실제 멤버 수 사용
	//for (int i = 0; i < n - 1; i++) 
	//{
	//	for (int j = 0; j < n - i - 1; j++)
	//	{
	//		if (sendClanINfo.stClanList[j].byGrade > sendClanINfo.stClanList[j + 1].byGrade) 
	//		{
	//			stClanHp temp = sendClanINfo.stClanList[j];
	//			sendClanINfo.stClanList[j] = sendClanINfo.stClanList[j + 1];
	//			sendClanINfo.stClanList[j + 1] = temp;
	//		}
	//	}
	//}
	

	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
	pPC->Write((BYTE*)&sendClanINfo, sizeof(SP_CASTLE_BATTLEMAP_CLANINFO));
}

void CServerCastle::OutBattleMap(CUnitPC* pPC, BOOL bClear)
{
	if (pPC == NULL)
		return;
	SP_CASTLE_BATTLEMAP sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CASTLE_BATTLEMAP_TYPE::WARP;


	//워프중
	if (pPC->m_bWARP_PENDING == TRUE)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}
	if (pPC->GetMapIndex() != GetMapIndex())
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_BATTLEMAP));
		return;
	}


	if (bClear == FALSE)
	{
		CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
		if (pMap)
		{
			pMap->GetMapPlayerList()->erase(pPC->GetCharacterUnique());
		}
	}



	if (pPC->GetAlive() == FALSE)
	{
		pPC->SetForceAlive();
	}
	pPC->SetTeamUnique(0, 0,0);
	if (g_DungeonManager.CheckWaveDungeonMap(pPC->GetCastleWarpOldMapIndex()) == TRUE ||
		g_DungeonManager.CheckDailyMapIndex(pPC->GetCastleWarpOldMapIndex()) == TRUE)
	{
		g_MapManager.UIWarp(pPC, 0, 1001, 220, 262, FALSE);

	}
	else
	{
		g_MapManager.UIWarp(pPC, 0, pPC->GetCastleWarpOldMapIndex() , pPC->GetCastleWarpOldPosX(), pPC->GetCastleWarpOldPosY(), FALSE);
	}

}

INT64 CServerCastle::GetDoorDamage(CUnitNPC* pNPC, INT64* pi64Damage)
{
	if(pNPC==NULL)
		return  *pi64Damage;

	if (CheckCastleNPCAttack(pNPC) == FALSE)
		return *pi64Damage;

	for (const auto& iter : _mapNpc)
	{
		if (iter.second == pNPC)
		{
			for (const auto& subiter : _vecSummonList)
			{
				if (iter.first == subiter.Index)
				{
					if (subiter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A
						|| subiter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B
						|| subiter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C)

					{
						return 100;
					}
				}
			}
		}
	}


	return *pi64Damage;
}

void CServerCastle::MapLoadEndSetting(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	SendCastleBattleTime(pPC); //24.12.17_클라에서 MapLoadEnd에서 보내달라고함.
	//25.01.15_남일우_공성 타일활성화를 위해 늦게들어온 사람에게도 알려달라함.
	if (_pCastleMaster != NULL)
	{
		SP_OCCUPATION_STATE sendTime;
		sendTime.i32RemainTimeTick = _ui32OccupationTimeTick;
		sendTime.wState = (WORD)ENUM_SP_OCCUPATION_STATE::COMPLEATE;
		pPC->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
	}

}


void CServerCastle::CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE eType, INT32 i32Index, INT32 Val1, UINT32 Val2)
{
	DCP_SAVE_CASTLE_BATTLE_DATA sendMsg;
	sendMsg.i32CastleIndex = GetCastleIndex();
	sendMsg.i32AllDelete = 0;
	sendMsg.i32Type = (INT32)eType;
	sendMsg.i32SaveIndex = i32Index;
	sendMsg.i32Value1 = Val1;
	sendMsg.ui32Value2 = Val2;

	g_Server.SendMsg(g_ServerArray.Bravo_GameDBServer1, (BYTE*)&sendMsg, sizeof(DCP_SAVE_CASTLE_BATTLE_DATA));
}

void CServerCastle::BroadCastBattleTime()
{
	CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
	if (pMap == NULL)
		return;

	SP_BROADCAST_CASTLE_TIME sendMsg;
	sendMsg.i32RemainTimeSecond = ui32CastleBattleTimeTick / 1000;
	if (_pCastleMaster)
	{
		memcpy(sendMsg._wcCharName, _pCastleMaster->GetName(), sizeof(sendMsg._wcCharName));
		memcpy(sendMsg._wcClanName, _pCastleMaster->GetClan()->GetClanName(), sizeof(sendMsg._wcClanName));
	}
	INT32 dwRemainTime = GetEndBattleTimeStamp() - g_GameManager.GetTimeStamp_Second();
	sendMsg.i32BattleRemainTimeSecond = dwRemainTime;

	pMap->m_BlockManager.BroadCast_AllBlock((BYTE*)&sendMsg, sizeof(SP_BROADCAST_CASTLE_TIME), TRUE);

	_ui32BroadCastTimeTick = TIME_MIN * 1000;
}

void CServerCastle::SendCastleBattleTime(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;
	SP_BROADCAST_CASTLE_TIME sendMsg;
	sendMsg.i32RemainTimeSecond = ui32CastleBattleTimeTick / 1000;
	if (_pCastleMaster)
	{
		memcpy(sendMsg._wcCharName, _pCastleMaster->GetName(), sizeof(sendMsg._wcCharName));
		memcpy(sendMsg._wcClanName, _pCastleMaster->GetClan()->GetClanName(), sizeof(sendMsg._wcClanName));
	}
	INT32 dwRemainTime = GetEndBattleTimeStamp() - g_GameManager.GetTimeStamp_Second();
	sendMsg.i32BattleRemainTimeSecond = dwRemainTime;

	pPC->Write((BYTE*)&sendMsg,sizeof(SP_BROADCAST_CASTLE_TIME));
}

void CServerCastle::DeclareCompleate(CUnitPC* pPC, DeclareInfo* stDeclareInfo)
{
	if (stDeclareInfo == NULL)
		return;

	SP_CASTLE_INFO sendMsg;
	sendMsg.i32Type = (INT32)ENUM_CP_CASTLE_INFO_TYPE::DECLARE;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
	sendMsg.i32Show = (INT32)ENUM_CASTLE_SHOW_TYPE::DECLARE_COMPLEATE;

	if (pPC != NULL)
	{
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CASTLE_INFO)); //실패해도 리스트는 추가해야함.
	}
	CLogManager::CASTLE_DECLARE_LOG(GetCastleIndex(),pPC, stDeclareInfo->_i64DeclareCost, stDeclareInfo->_ui32Clanunique);
	DeclareInfo addInfo = *stDeclareInfo;
	_vecDeclareList.push_back(addInfo);
	SendDeclareList(pPC);
}


void CServerCastle::CastleNpcKillCheck(CUnitNPC* pNpc, CUnitPC* pPC)
{
	if (pNpc == NULL)
		return;

	INT32 i32CastleNpcIndex = 0;
	for (const auto& iter : _mapNpc)
	{
		if (iter.second == pNpc)
		{
			i32CastleNpcIndex = iter.first;
			break;
		}
	}
	if (i32CastleNpcIndex == 0)
		return;
	//제거된 몬스터 뺌
	if (_mapNpc.find(i32CastleNpcIndex) != _mapNpc.end())
	{
		_mapNpc.find(i32CastleNpcIndex)->second = NULL;
		_mapNpc.erase(i32CastleNpcIndex);
	}


	INT32 i32BuffIdx = 0;
	CASTLE_BATTLE_NPC_TYPE eType = CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_MAX;
	for (const auto& iter : _vecSummonList)
	{
		if (iter.Index == i32CastleNpcIndex)
		{
			eType = iter.i32Type;

			DWORD dwCharunique = 0;
			if (pPC && eType == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_BUFFNPC)
			{
				dwCharunique = pPC->GetCharacterUnique();
			}
			i32BuffIdx = iter.i32BuffIndex;
			//FALSE면 죽음
			CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::MONSTER_INFO, i32CastleNpcIndex, dwCharunique,0);
		}
	}
	CLogManager::CASTLE_KILL_LOG(GetCastleIndex(), pPC, eType);
	//죽이고 처리
	switch (eType)
	{
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_BUFFNPC:
	{
		if (pPC == NULL)
			return;
		INT32 i32Count = 0;
		for (auto iter : _vecMonsterKillArray)
		{
			if (iter.i32Charunique == pPC->GetCharacterUnique())
			{
				iter.i32KillCount++;
				i32Count = iter.i32KillCount;
			}
		}
		if (i32Count == 0)
		{
			stMonsterKillInfo killcount;
			killcount.i32Charunique = pPC->GetCharacterUnique();
			killcount.i32KillCount = 1;
			i32Count = 1;
			_vecMonsterKillArray.push_back(killcount);
		}
		//CBuffManager* BuffManager = CBuffManager::GetInstance();
		//if (BuffManager)
		//{
		//	stBuffData* pBuffData = BuffManager->GetBuffData(i32BuffIdx);
		//	if (pBuffData)
		//	{
		//		CBuff* pBuff = g_BuffPool.GetAvailableObject();
		//		if (pBuff)
		//		{
		//			if (pBuff->SetBuffData(pPC->GetFieldUnique(), pBuffData, 0))
		//			{
		//				INT32 dwRemainTime = GetEndBattleTimeStamp() - g_GameManager.GetTimeStamp_Second();
		//				if (dwRemainTime < 0)
		//					dwRemainTime = 0;
		//				pBuff->SetRemainTick(dwRemainTime * 1000);
		//				pPC->AddBuffToBuffList(pBuff);
		//				BuffManager->BroadCastBuffAppear(pPC, pBuff);
		//
		//				pPC->AddBuff(pPC->GetFieldUnique(), i32BuffIdx);
		//			}
		//		}
		//	}
		//
		//}


	}
	break;
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A:
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B:
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C:
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_RIGHT_2:
	{
		CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::DOOR_KILL, NULL, pNpc->GetIndex()); 
	}
	break;
	default:
		break;
	}
}

BOOL CServerCastle::CheckCastleNPCAttack(CUnitNPC* pNPC)
{
	if (pNPC == NULL)
		return TRUE;

	INT32 i32CastleNpcIndex = 0;
	for (const auto& iter : _mapNpc)
	{
		if (iter.second == pNPC)
		{
			i32CastleNpcIndex = iter.first;
			break;
		}
	}
	if (i32CastleNpcIndex == 0)
		return TRUE;

	CASTLE_BATTLE_NPC_TYPE eType = CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_MAX;
	for (const auto& iter : _vecSummonList)
	{
		if (iter.Index == i32CastleNpcIndex)
		{
			eType = iter.i32Type;
			break;
		}
	}
	switch (eType)
	{
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_BUFFNPC:
	{
		if (GetCastleState() != CASTLE_TIME_STATE::CTS_READY_TEN_MIN) //전초전이 아니면 공격불가
			return FALSE;
	}
	break;
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_A:
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_B:
	case CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_GATE_C:
	{
		if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE) //공성시간이 아니면 공격불가
			return FALSE;
	}
	break;
	default:
		break;
	}

	return TRUE;
}

void CServerCastle::BattleEndReward()
{
	for (const auto& iter : _vecBattlePCInfo)
	{
		if (GetMasterClan() && iter.dwClanUnique == GetMasterClan()->GetClanUnique())
		{
			CUnitPC* pPC = g_GameManager.mUserManager.FindUser(iter.dwCharunique);
			if (pPC && pPC->GetMapIndex() == GetMapIndex())
			{
				CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::BATTLE_END, pPC, 1, FALSE); //승리
			}
			CLogManager::CASTLE_REWARD_LOG(TRUE, GetCastleIndex(), iter.dwCharunique, iter.dwClanUnique);
			g_MailManager.InputMail(0, iter.dwCharunique, ENUM_MAIL_TYPE::ENUM_MAIL_TYPE_CASTLE_BATTLE_WIN, ENUM_COST_TYPE::ENUM_COST_TYPE_ITEM, GetWinRewardItemIndex(), 1);
		}
		else
		{
			CUnitPC* pPC = g_GameManager.mUserManager.FindUser(iter.dwCharunique);
			if (pPC && pPC->GetMapIndex() == GetMapIndex())
			{
				CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::BATTLE_END, pPC, 0, FALSE); //패배
			}
			CLogManager::CASTLE_REWARD_LOG(FALSE, GetCastleIndex(), iter.dwCharunique, iter.dwClanUnique);
			g_MailManager.InputMail(0, iter.dwCharunique, ENUM_MAIL_TYPE::ENUM_MAIL_TYPE_CASTLE_BATTLE_LOSE, ENUM_COST_TYPE::ENUM_COST_TYPE_ITEM, GetLoseRewardItemIndex(), 1);
		}
	}

}

void CServerCastle::BattleEnd()
{
	//승리자 세팅
	if (_pCastleMaster == NULL && GetCastleClanUnique() != 0) //수성측이 있고, 클랜장이 없음.
	{
		GetSaveInfo()->_i32RelayCount += 1;
		CLogManager::CASTLE_END_LOG(GetCastleIndex(),GetMasterClan(),NULL, GetSaveInfo()->_i32RelayCount);
	}
	else if (_pCastleMaster != NULL)
	{
		if (_pCastleMaster->GetClanUnique() == GetCastleClanUnique())
		{
			//수성 승리
			GetSaveInfo()->_i32RelayCount += 1;
			GetSaveInfo()->i32BeforeClanUnique = GetSaveInfo()->_ui32OwnerClanunique;
			CLogManager::CASTLE_END_LOG(GetCastleIndex(), GetMasterClan(), NULL, GetSaveInfo()->_i32RelayCount);
		}
		else if (_pCastleMaster->GetClanUnique() != GetCastleClanUnique())
		{
			//공성 승리
			if (_pCastleMaster->GetClan())
			{
				CClan* LossClan = g_ClanManager.FindByClanUnique(GetCastleClanUnique());
				if (LossClan)
				{
					//연속승리 까주고
					INT32 i32LoseClanWin = 0;
					if (_mapLevelInfo.find(GetCastleLevel() - 1) == _mapLevelInfo.end())
					{
						i32LoseClanWin = 0;
					}
					else
					{
						i32LoseClanWin = _mapLevelInfo.find(GetCastleLevel() - 1)->second.i32MinRelayWinCnt;
					}
					LossClan->SetServerCastleRelayWinCount(i32LoseClanWin);
					LossClan->SaveDB();

					//세금 빼주기_시간제한으로 변경
					//DCP_CONSUME_CASTLE_CASH sendCash;
					//sendCash.i32CastleIndex = GetCastleIndex();
					//sendCash.dwCharunique = LossClan->GetMasterunique();
					//sendCash.dwClanUnique = LossClan->GetClanUnique();
					//sendCash.dwAccunique = 0;
					//sendCash.i32Type = (INT32)ENUM_CP_CASTLE_EDIT_TYPE::CASTLE_LOSE_OUT;
					//sendCash.i64Cost = 0;
					//sendCash.i32GroupServerIdx = g_INIFile_Server.m_iServerGroupID;
					//
					//g_Server.SendMsg(g_ServerArray.Bravo_CashDB, (BYTE*)&sendCash, sizeof(DCP_CONSUME_CASTLE_CASH));
					//CLogManager::CASTLE_TAX_OUT(GetCastleIndex(), LossClan->GetMasterunique(), (INT32)ENUM_CP_CASTLE_EDIT_TYPE::CASTLE_LOSE_OUT, 0, 0, 0);

				}
				CClan* WinnerClan = _pCastleMaster->GetClan();
				SetMasterClan(WinnerClan);

				GetSaveInfo()->i32BeforeClanUnique = GetSaveInfo()->_ui32OwnerClanunique;

				GetSaveInfo()->stClanSimbol = WinnerClan->GetClanSimbolIdx();
				memcpy(GetSaveInfo()->_wcOwnerClanName, WinnerClan->GetClanName(),sizeof(WCHAR)*MAX_CLAN_NAME);
				memcpy(GetSaveInfo()->_wcOwnerCharName,WinnerClan->GetMasterName(),sizeof(WCHAR)*LENGTH_CHARACTER_NICKNAME);
				GetSaveInfo()->_i32RelayCount = WinnerClan->GetServerCastleRelayWinCount()<=0?1 : WinnerClan->GetServerCastleRelayWinCount(); //연속수성
				GetSaveInfo()->_i32ServerGroup = g_INIFile_Server.m_iServerGroupID;
				GetSaveInfo()->_dateLastBattleTimeStamp = g_Server.GetTimeStamp();

				GetSaveInfo()-> _ui32OwnerClanunique = WinnerClan->GetClanUnique();

				CLogManager::CASTLE_END_LOG(GetCastleIndex(), GetMasterClan(), LossClan, GetSaveInfo()->_i32RelayCount);
			}
			else
			{
				CLogManager::CASTLE_END_LOG(GetCastleIndex(), NULL, NULL, GetSaveInfo()->_i32RelayCount);
			}

		}

	}
	else if (_pCastleMaster == NULL && GetCastleClanUnique() == 0) //승자가 아무도 없음.
	{
		GetSaveInfo()->_ui32OwnerClanunique =0;
		GetSaveInfo()->i32BeforeClanUnique = 0;
		memset(&GetSaveInfo()->stClanSimbol, 0x00, sizeof(stClanSymbol));
		memset(GetSaveInfo()->_wcOwnerClanName, 0x00, sizeof(WCHAR) * MAX_CLAN_NAME);
		memset(GetSaveInfo()->_wcOwnerCharName, 0x00, sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
		GetSaveInfo()->_i32RelayCount = 0;
		GetSaveInfo()->_i32ServerGroup = g_INIFile_Server.m_iServerGroupID;
		GetSaveInfo()->_dateLastBattleTimeStamp = g_Server.GetTimeStamp();

		CLogManager::CASTLE_END_LOG(GetCastleIndex(), NULL, NULL, GetSaveInfo()->_i32RelayCount);
	}


	//한주 지날때 마다 단계 하락 -> 24.11.13_회의에서 빼기로함
	//for (const auto& iter : g_ClanManager.m_vActiveClans)
	//{
	//	if(iter->GetClanUnique() != GetCastleClanUnique() ||
	//		iter->GetClanUnique() != GetSaveInfo()->i32BeforeClanUnique)
	//	{
	//		if (iter->GetServerCastleRelayWinCount() != 0)
	//		{
	//			if (_mapLevelInfo.find(FindCastleStep(iter->GetServerCastleRelayWinCount())-1) == _mapLevelInfo.end())
	//			{
	//				iter->SetServerCastleRelayWinCount(0);
	//			}
	//			else
	//			{
	//				iter->SetServerCastleRelayWinCount(	_mapLevelInfo.find(FindCastleStep(iter->GetServerCastleRelayWinCount()) - 1)->second.i32MinRelayWinCnt);
	//			}
	//		}
	//		iter->SaveDB();
	//	}
	//
	//}

	SettingCastleStep();
}

void CServerCastle::InitBattle()
{

	DCP_CASTLE_DECLARE sendDb;
	sendDb._i32CastleIndex = this->GetCastleIndex();
	sendDb._i32Type = 3;
	g_Server.SendMsg(g_ServerArray.Bravo_GameDBServer1, (BYTE*)&sendDb, sizeof(DCP_CASTLE_DECLARE));

	DCP_SAVE_CASTLE_BATTLE_DATA sendDelete;
	sendDelete.i32CastleIndex = GetCastleIndex();
	sendDelete.i32AllDelete = TRUE;
	g_Server.SendMsg(g_ServerArray.Bravo_GameDBServer1, (BYTE*)&sendDelete, sizeof(DCP_SAVE_CASTLE_BATTLE_DATA));
	printf("INIT_BATTLE \n");

	//초기화 부분
	IOccupationRuleInit();
	for (const auto& iter : _mapNpc)
	{
		iter.second->SetDieAlone(TRUE);
	}
	_mapNpc.clear();
	_vecDeclareList.clear();
	_vecMonsterKillArray.clear();
	ui32CastleBattleTimeTick = 0;
	_vecBattlePCInfo.clear();
	_mapClanRallyData.clear();
	_mapUserRallyUseCount.clear();
	GetSaveInfo()->i32DefenceCost = 0;
	_i32BattleMin = 0;
}

void CServerCastle::SettingEndTimeOut()
{
	_bEndTime = TRUE;
	_ui32EndOutBattleTime = 5000;
}

void CServerCastle::CastleEndOutBattleMap()
{
	CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
	if (pMap)
	{
		map<INT32, CUnitPC*>& playerList = *pMap->GetMapPlayerList();
		if (playerList.size() > 0)
		{
			for (auto iter = playerList.begin(); iter != playerList.end(); ++iter)
			{
				if (iter->second != NULL)
				{
					OutBattleMap(iter->second,TRUE);
				}
			}
		}
	}
	pMap->GetMapPlayerList()->clear();

	//마지막 아웃처리까지
	for (const auto& iter : g_GameManager.mUserManager.m_pUsers)
	{
		if (iter)
		{
			if (iter->GetMapIndex() == GetMapIndex())
			{
				OutBattleMap(iter, TRUE);
			}
		}
	}
}


void CServerCastle::WarpEndReBuffSetting(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	//for (auto iter : _vecMonsterKillArray)
	//{
	//	if (pPC->GetCharacterUnique() == iter.i32Charunique && iter.bReEnter ==TRUE)
	//	{
	//		iter.bReEnter = FALSE;
	//
	//		//버프걸어준다!
	//		return;
	//	}
	//}

}

void CServerCastle::BuffNPCALLKill()
{
	for (const auto& iter : _vecSummonList)
	{
		if (iter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_BUFFNPC)
		{
			if (_mapNpc.find(iter.Index) != _mapNpc.end())
			{
				if (_mapNpc.find(iter.Index)->second != NULL)
				{
					if (_mapNpc.find(iter.Index)->second->GetAlive() == TRUE)
					{
						_mapNpc.find(iter.Index)->second->SetDieAlone(TRUE);
						_mapNpc.find(iter.Index)->second = NULL;

						CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::MONSTER_INFO, iter.Index, 0, FALSE);
					}
				}
				_mapNpc.erase(iter.Index);
			}
		}
	}

}

#pragma region  인터페이스

void CServerCastle::IOccupationRuleInit()
{
	_pEnterPC = NULL;
	if (_pCastleMaster != NULL)
	{
		EraseCastleBuff(_pCastleMaster);
	}
	_pCastleMaster = NULL;
	_ui32OccupationTimeTick = (TIME_MIN / 2) *1000;
	i32TimeSecond = 0;
}


void CServerCastle::IOccupationRuleLoop(DWORD dwDeltick)
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
		return;

	if (_pEnterPC == NULL && _pCastleMaster == NULL)
		return;


	if (_ui32OccupationTimeTick >= dwDeltick)
	{
		_ui32OccupationTimeTick -= dwDeltick;
	}
	else
	{
		_ui32OccupationTimeTick = 0;
	}

	if (_ui32OccupationTimeTick == 0 && _pCastleMaster ==NULL)
	{
		UpdateBattleTime();
		if (_pEnterPC == NULL)
		{
			IOccupationRuleInit();
			return;
		}
		_pCastleMaster = _pEnterPC;
		_pEnterPC = NULL;
		BroadCastBattleTime();
		CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::NEW_CASTLE_MASTER, _pCastleMaster,0);

		SP_OCCUPATION_STATE sendTime;
		sendTime.i32RemainTimeTick = _ui32OccupationTimeTick;
		sendTime.wState = (WORD)ENUM_SP_OCCUPATION_STATE::COMPLEATE;
		_pCastleMaster->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
		BroadCastOccupationRuleBroadCast(&sendTime);

		CLogManager::CASTLE_MASTER_LOG(GetCastleIndex(), _pCastleMaster, FALSE, 0);
		//성주 달아주기
		CBuffManager* BuffManager = CBuffManager::GetInstance();
		if (BuffManager)
		{
			stBuffData* pBuffData = BuffManager->GetBuffData(SERVER_CASTLE_MASTER);
			if (pBuffData)
			{
				CBuff* pBuff = g_BuffPool.GetAvailableObject();
				if (pBuff)
				{
					if (pBuff->SetBuffData(_pCastleMaster->GetFieldUnique(), pBuffData, 0))
					{
						INT32 dwRemainTime = GetEndBattleTimeStamp() - g_GameManager.GetTimeStamp_Second();
						if (dwRemainTime < 0)
							dwRemainTime = 0;
						pBuff->SetRemainTick(dwRemainTime * 1000);
						_pCastleMaster->AddBuffToBuffList(pBuff);

						_pCastleMaster->AddBuff(_pCastleMaster->GetFieldUnique(), SERVER_CASTLE_MASTER);
					}
				}
			}

		}
	}

	if (_pCastleMaster && _pEnterPC == NULL)
	{
		BOOL bResult= CheckChangeCastleMaster(_pCastleMaster);
		if (bResult == TRUE)
		{
			IOccupationRuleInit();
			ReturnBattleTime();
			BroadCastBattleTime();
		}
	}
	else if (_pEnterPC && _pCastleMaster== NULL)
	{
		INT32 i32Second = _ui32OccupationTimeTick / 1000;
		if (i32TimeSecond != i32Second)
		{
			SP_OCCUPATION_STATE sendTime;
			sendTime.i32RemainTimeTick = _ui32OccupationTimeTick;
			sendTime.wState = (WORD)ENUM_SP_OCCUPATION_STATE::LOOP;
			_pEnterPC->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
			BroadCastOccupationRuleBroadCast(&sendTime);
			i32TimeSecond = i32Second;
		}

		OutEnterUser(_pEnterPC);
	}
	else
	{
		printf("IOccupationRuleErrorLoop!!!!!!!\n");
	}

}

void CServerCastle::EnterUser(CUnitPC* pPC)
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
		return;

	if (pPC == NULL)
		return;
	if (GetMapIndex() != pPC->GetMapIndex())
		return;
	if (_pCastleMaster != NULL)
		return;

	IOccupationRuleInit();
	_pEnterPC = pPC;

	SP_OCCUPATION_STATE sendTime;
	sendTime.i32RemainTimeTick = _ui32OccupationTimeTick;
	sendTime.wState = (WORD)ENUM_SP_OCCUPATION_STATE::TILE_ENTER;
	_pEnterPC->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
	BroadCastOccupationRuleBroadCast(&sendTime);
}

void CServerCastle::OutEnterUser(CUnitPC* pPC)
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
		return;

	if (pPC == NULL || _pEnterPC == NULL)
		return;

	if (_pCastleMaster != NULL)
		return;

	if (_pEnterPC != pPC)
		return;


	CGameMap* pMap = _pEnterPC->GetCurrentMap();

	SP_OCCUPATION_STATE sendTime;
	sendTime.i32RemainTimeTick = _ui32OccupationTimeTick;
	sendTime.wState = (WORD)ENUM_SP_OCCUPATION_STATE::TILE_OUT;

	if (pPC->GetAlive() == FALSE)
	{
		_pEnterPC->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
		BroadCastOccupationRuleBroadCast(&sendTime);
		IOccupationRuleInit();
		return;
	}

	if (pMap == NULL)
	{
		_pEnterPC->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
		BroadCastOccupationRuleBroadCast(&sendTime);
		IOccupationRuleInit();
		return ;
	}

	if (pMap->m_MapInfo->m_MapData.tileInfo[_pEnterPC->m_X][_pEnterPC->m_Y].type != MAP_TILE_TYPE::MAP_TILE_CASTLEMASTER_ZONE)
	{
		_pEnterPC->Write((BYTE*)&sendTime, sizeof(SP_OCCUPATION_STATE));
		BroadCastOccupationRuleBroadCast(&sendTime);
		IOccupationRuleInit();
		return;
	}
	
}

BOOL CServerCastle::CheckChangeCastleMaster(CUnitPC* _pCastleMaster)
{
	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
	{
		return FALSE;
	}

	if (_pCastleMaster == NULL)
	{
		CLogManager::CASTLE_MASTER_LOG(GetCastleIndex(), _pCastleMaster, TRUE, 1);
		return TRUE;
	}

	//INT32 i32Grade = 0;
	//if (_pCastleMaster->GetClan())
	//{
	//	i32Grade = _pCastleMaster->GetClan()->GetMemberGrade(_pCastleMaster->GetCharacterUnique());
	//}

	

	if (_pCastleMaster->GetAlive() == FALSE)
	{
		CLogManager::CASTLE_MASTER_LOG(GetCastleIndex(), _pCastleMaster, TRUE, 2);
		CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::CASTLE_MASTER_CHANGE, _pCastleMaster, 0);
		return TRUE;
	}
	if (_pCastleMaster->m_bWARP_PENDING == TRUE)
	{
		CLogManager::CASTLE_MASTER_LOG(GetCastleIndex(), _pCastleMaster, TRUE, 3);
		CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::CASTLE_MASTER_CHANGE, _pCastleMaster, 0);
		return TRUE;
	}
	CGameMap* pMap = _pCastleMaster->GetCurrentMap();
	if (pMap == NULL)
	{
		CLogManager::CASTLE_MASTER_LOG(GetCastleIndex(), _pCastleMaster, TRUE, 4);
		CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::CASTLE_MASTER_CHANGE, _pCastleMaster, 0);
		return TRUE;
	}

	if (pMap->m_MapInfo->m_MapData.tileInfo[_pCastleMaster->m_X][_pCastleMaster->m_Y].type != MAP_TILE_TYPE::MAP_TILE_CASTLEMASTER_ZONE &&
		pMap->m_MapInfo->m_MapData.tileInfo[_pCastleMaster->m_X][_pCastleMaster->m_Y].type != MAP_TILE_TYPE::MAP_TILE_CASTLE_PVP)
	{
		CLogManager::CASTLE_MASTER_LOG(GetCastleIndex(), _pCastleMaster, TRUE, 5);
		CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE::CASTLE_MASTER_CHANGE, _pCastleMaster, 0);
		return TRUE;
	}


	return FALSE;

}

void CServerCastle::UpdateBattleTime()
{
	DWORD dwBattleTimeStamp = g_GameManager.GetTimeStamp_Second() + CASTLE_CHANGE_BATTLE;
	if (GetEndBattleTimeStamp() < dwBattleTimeStamp)
	{
		ui32CastleBattleTimeTick = (GetEndBattleTimeStamp() - g_GameManager.GetTimeStamp_Second())*1000;
	}
	else
	{
		ui32CastleBattleTimeTick = CASTLE_CHANGE_BATTLE * 1000;
	}

	
}

void CServerCastle::ReturnBattleTime()
{
	if (GetEndBattleTimeStamp() > g_GameManager.GetTimeStamp_Second() && _pCastleMaster == NULL)
	{
		ui32CastleBattleTimeTick = (GetEndBattleTimeStamp() - g_GameManager.GetTimeStamp_Second()) * 1000;
	}
}

void CServerCastle::BroadCastOccupationRuleBroadCast(SP_OCCUPATION_STATE* pSendMsg)
{
	CGameMap* pMap = g_MapManager.FindMapIndex(GetMapIndex());
	if (pSendMsg == NULL)
		return;
	pSendMsg->wIsCastleMaster = ENUM_ERROR_MESSAGE_FAILED;
	if (pMap)
	{
		for (int i = 0; i < MAX_BLOCK_SECTION; i++)
		{
			for (int j = 0; j < MAX_BLOCK_SECTION; j++)
			{
				list< CUnit* >* pLinkedList = &pMap->m_BlockManager.m_Block[i][j].m_Linked_User_List;
				for (auto iter = pLinkedList->begin(); iter != pLinkedList->end(); ++iter) {
					if ((*iter)->GetUnitTYPE() == eUnitType::PC) {
						if ((*iter) == _pEnterPC)
						{
							continue;
						}
						if ((*iter) != NULL)
						{
							(*iter)->Write((BYTE*)pSendMsg, sizeof(SP_OCCUPATION_STATE),TRUE);
						}
					}
				}
			}
		}
	}

}

void CServerCastle::BroadCastEndClanFlag(DWORD dwClanUnique)
{
	CClan* pClan = g_ClanManager.FindByClanUnique(dwClanUnique);
	if (pClan == NULL)
		return;

	SP_CLANFLAG_STATE sendMsg;
	sendMsg.i32Type = (INT32)SP_CLANFLAG_STATE_TYPE::END_FLAG;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;

	for (const auto& iter : *pClan->GetOnlineMemberList())
	{
		if (iter)
		{
			if (iter->GetMapIndex() == GetMapIndex())
			{
				iter->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));
			}
		}
	}

}

void CServerCastle::WarpClanFlag(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;

	SP_CLANFLAG_STATE sendMsg;
	sendMsg.i32Type = (INT32)SP_CLANFLAG_STATE_TYPE::WARP;
	sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_FAILED;
	if (pPC->GetAlive() == FALSE)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg,sizeof(SP_CLANFLAG_STATE));
		return ;
	}
	if (pPC->m_bWARP_PENDING == TRUE)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));
		return;
	}
	if (pPC->GetMapIndex() != GetMapIndex())
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_NO_WARP;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));
		return;
	}

	if (_mapClanRallyData.find(pPC->GetClanUnique()) == _mapClanRallyData.end())
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_NOT_FLAGSKILL;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));
		return;
	}
	stCLanRallyData RallyData = _mapClanRallyData.find(pPC->GetClanUnique())->second;

	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_MESSAGE_CASTLE_BATTLE_NOTTIME;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));
		return;
	}

	//TODO::집결무제한으로 사용하게 임시
	//집결워프는 1회만 사용가능
	if (_mapvecUseWarpCount.find(pPC->GetClanUnique()) != _mapvecUseWarpCount.end())
	{
		for (const auto& iter : _mapvecUseWarpCount.find(pPC->GetClanUnique())->second)
		{
			if (iter == pPC->GetCharacterUnique())
			{
				sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_USE_FLAGSKILL;
				pPC->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));
				return;
			}
		}
	}


	if (_pCastleMaster == pPC)
	{
		sendMsg.i32Result = ENUM_ERROR_MESSAGE::ENUM_ERROR_CASTLE_NOT_ACTION;
		pPC->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));
		return;
	}


	sendMsg.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
	pPC->Write((BYTE*)&sendMsg, sizeof(SP_CLANFLAG_STATE));

	_mapvecUseWarpCount[pPC->GetClanUnique()].push_back(pPC->GetCharacterUnique());
	CLogManager::CASTLE_CLANFLAG_LOG(GetCastleIndex(), pPC, 1, 1);

	g_MapManager.UIWarp(pPC, 0, GetMapIndex(), RallyData.wX, RallyData.wY, FALSE);
}

void CServerCastle::SendClanOpenFlag(CUnitPC* pPC)
{
	if (pPC == NULL)
		return;
	if (pPC->GetMapIndex() != GetMapIndex())
		return;

	if (pPC->GetClan() == NULL || pPC->GetClanUnique() == 0)
		return;

	if (_mapClanRallyData.find(pPC->GetClan()->GetClanUnique()) == _mapClanRallyData.end())
	{
		return ;
	}
	stCLanRallyData RallyData = _mapClanRallyData.find(pPC->GetClan()->GetClanUnique())->second;

	SP_CLANFLAG_STATE sendFlagInfo;
	sendFlagInfo.i32Type = (INT32)SP_CLANFLAG_STATE_TYPE::OPEN_FLAG;
	sendFlagInfo.i32RemainTimeSecond = RallyData.dwRemainTick/1000;
	memcpy(sendFlagInfo.wcCharName, RallyData.wcUseCharName, sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
	sendFlagInfo.i32Result = ENUM_ERROR_MESSAGE::SUCCESS;
	sendFlagInfo.byGrade = RallyData.wUseCharGrade;

	if (_mapvecUseWarpCount.find(pPC->GetClanUnique()) != _mapvecUseWarpCount.end())
	{
		for (const auto& iter : _mapvecUseWarpCount.find(pPC->GetClanUnique())->second)
		{
			if (iter == pPC->GetCharacterUnique())
			{
				sendFlagInfo.byUse = TRUE;
				break;
			}
		}
	}

	pPC->Write((BYTE*)&sendFlagInfo, sizeof(SP_CLANFLAG_STATE));
}

BOOL CServerCastle::RallyItemCheck(CUnitPC* pPC, DWORD dwClanUnique)
{
	if (pPC == NULL)
		return TRUE;

	CClan* pClan = g_ClanManager.FindByClanUnique(dwClanUnique);
	if(pClan == NULL)
		return TRUE;

	if (pPC->GetMapIndex() != GetMapIndex())
		return 	ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_CASTLEMAP;
	if (pPC->m_bWARP_PENDING == TRUE)
		return TRUE;


	//사용중
	if (_mapClanRallyData.find(dwClanUnique) != _mapClanRallyData.end())
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_USE_FLAGSKILL;
	}

	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_CASTLETIME;
	}

	if (pPC->GetCurrentMap()->m_MapInfo->m_MapData.tileInfo[pPC->m_X][pPC->m_Y].type == MAP_TILE_TYPE::MAP_TILE_CASTLEMASTER_ZONE)
	{
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_CASTLEMAP;
	}


	INT32 i32UseCount = 1;
	if (_mapUserRallyUseCount.find(pPC->GetCharacterUnique()) != _mapUserRallyUseCount.end())
	{
		i32UseCount += _mapUserRallyUseCount.find(pPC->GetCharacterUnique())->second;
	}
	//TODO::집결무제한으로 사용하게 임시
	//등급별 사용량 설정
	CLAN_GRADE_TYPE eGramde  = (CLAN_GRADE_TYPE)pClan->GetMemberGrade(pPC->GetCharacterUnique());
	switch (eGramde)
	{
	case CLAN_GRADE_TYPE::CLAN_MASTER:
	{
		if (i32UseCount > (INT32)ENUM_CLANS_RALLY_ITEM_MAX::Master)
			return ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_COUNT;
	}
	break;
	case CLAN_GRADE_TYPE::CLAN_SUB_MASTER:
	{
		if (i32UseCount > (INT32)ENUM_CLANS_RALLY_ITEM_MAX::SubMaster)
			return ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_COUNT;
	}
	break;
	case CLAN_GRADE_TYPE::CLAN_ELDER:
	{
		if (i32UseCount > (INT32)ENUM_CLANS_RALLY_ITEM_MAX::Elder)
			return ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_COUNT;
	}
	break;
	case CLAN_GRADE_TYPE::CLAN_NORMAL:
	{
		if (i32UseCount > (INT32)ENUM_CLANS_RALLY_ITEM_MAX::None)
			return ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_COUNT;
	}
	break;
	default:
		return ENUM_ERROR_MESSAGE::ENUM_ERROR_FLAG_USE_NOT_COUNT;
		break;
	}





	return FALSE;
}

void CServerCastle::UseRallyItem(CUnitPC* pPC, DWORD dwClanUnique)
{
	if (pPC == NULL)
		return;
	if (pPC->GetMapIndex() != GetMapIndex())
		return;
	if (pPC->m_bWARP_PENDING == TRUE)
		return;
	if (dwClanUnique == 0)
		return;

	if (GetCastleState() != CASTLE_TIME_STATE::CTS_BATTLE)
	{
		return ;
	}
	INT32 i32UseCount = 0;
	if (_mapUserRallyUseCount.find(pPC->GetCharacterUnique()) != _mapUserRallyUseCount.end())
	{
		i32UseCount =_mapUserRallyUseCount.find(pPC->GetCharacterUnique())->second ++;
		CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::CLAN_RALLY_USE_COUNT, pPC->GetCharacterUnique(), _mapUserRallyUseCount.find(pPC->GetCharacterUnique())->second);
	}
	else
	{
		i32UseCount = 1;
		_mapUserRallyUseCount.insert(make_pair(pPC->GetCharacterUnique(), 1));
		CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE::CLAN_RALLY_USE_COUNT, pPC->GetCharacterUnique(), 1);
	}


	stCLanRallyData stData;
	stData.dwClanUnique = dwClanUnique;
	stData.dwRemainTick = (TIME_MIN / 2) * 1000; //30초
	stData.wX = pPC->m_X;
	stData.wY = pPC->m_Y;
	memcpy(stData.wcUseCharName, pPC->GetName(), sizeof(WCHAR) * LENGTH_CHARACTER_NICKNAME);
	if (pPC->GetClan())
	{
		stData.wUseCharGrade = (WORD)pPC->GetClan()->GetMemberGrade(pPC->GetCharacterUnique());
	}

	_mapClanRallyData.insert(make_pair(dwClanUnique, stData));

	CClan* pClan = pPC->GetClan();
	if (pClan == NULL)
		return;

	CLogManager::CASTLE_CLANFLAG_LOG(GetCastleIndex(), pPC, 0, i32UseCount);

	for (const auto& pIter : *pClan->GetOnlineMemberList())
	{
		if (pIter->GetMapIndex() == GetMapIndex())
		{
			SendClanOpenFlag(pIter);
		}
	}


}

INT32 CServerCastle::MyClanRallyAxisX(DWORD dwClanUnique)
{
	if (_mapClanRallyData.find(dwClanUnique) != _mapClanRallyData.end())
	{
		return _mapClanRallyData.find(dwClanUnique)->second.wX;
	}
	else
	{
		return 0;
	}
}

INT32 CServerCastle::MyClanRallyAxisY(DWORD dwClanUnique)
{
	if (_mapClanRallyData.find(dwClanUnique) != _mapClanRallyData.end())
	{
		return _mapClanRallyData.find(dwClanUnique)->second.wY;
	}
	else
	{
		return 0;
	}
}

#pragma endregion
