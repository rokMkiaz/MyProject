#pragma once

#pragma region 선언

enum class ENUM_CLANS_RALLY_ITEM_MAX
{
	Master = 5,
	SubMaster = 3,
	Elder = 2,
	None = 1,
};

struct stMonsterKillInfo
{
	INT32 i32Charunique=0;
	INT32 i32KillCount = 0;
	BOOL bReEnter = FALSE; //TRUE일시 입장할때 버프 걸어준다.
};
struct stCLanRallyData
{
	DWORD dwClanUnique = 0;
	UINT32 dwRemainTick = 0;
	WORD wX = 0;
	WORD wY = 0;
	WCHAR wcUseCharName[LENGTH_CHARACTER_NICKNAME] = {0,}; //이름
	WORD wUseCharGrade = 0; //클랜 직급
};
#pragma endregion

class CServerCastle : public CCastle, IOccupationRule, IClanFlagRule
{
public:
	CServerCastle() { CCastle::Init(); Init(); }
	void Init();

	void Loop(DWORD dwDeltick) override;
	void SaveDB() override;

	void ON_Load_SaveData(BYTE* pPacket) override;

	void OnBattleReady() override;
	void OnBattleReadyCompute() override;
	void OnBattleStart() override;
	void OnBattleEnd() override;

//////////////////////////////////////////////////////////////
/////						전투 					     /////
//////////////////////////////////////////////////////////////
public:
	//일반 지역에서 맵볼때
	void SendBattlMap(CUnitPC* pPC) override;
	//기존 맵 -> 공성전맵
	void WarpBattleMap(CUnitPC* pPC, INT32 i32WarpIndex) override;
	//입장한 유저 기록
	void AddBattleMapUser(CUnitPC* pPC)override;
	//전장에서 요청할때
	void ShowBattleInfo(CUnitPC* pPC)override;
	
	void OutBattleMap(CUnitPC* pPC,BOOL bClear = FALSE) override;
	INT64 GetDoorDamage(CUnitNPC* pNPC, INT64* pi32Damage)override;

	void MapLoadEndSetting(CUnitPC* pPC) override;

	void CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE eType, INT32 i32Index, INT32 Val1 = 0, UINT32 Val2 = 0);
	void BroadCastBattleTime();
	
	void SendCastleBattleTime(CUnitPC* pPC);

//////////////////////////////////////////////////////////////
/////						선포 					     /////
//////////////////////////////////////////////////////////////
	void DeclareCompleate(CUnitPC* pPC, DeclareInfo* stDeclareInfo) override;

	//CP_CASTLE_INFO
	void ON_CM_CastleInfo(CUnitPC* pPC); 
	void ON_Send_CastleInfo(DWORD dwCharunique, INT64 i64GoldBalance,INT64 i64CastleDungeonBalance);


//////////////////////////////////////////////////////////////
/////					전투 세팅 					     /////
//////////////////////////////////////////////////////////////

	void CastleNpcKillCheck(CUnitNPC* pNpc, CUnitPC* pPC) override;

	void WarpEndReBuffSetting(CUnitPC* pPC); //서버연결 끊기고 첫속시 버프 다시걸어줄때
	void BuffNPCALLKill();
	//보상 넣어줄때
	void BattleEndReward();
	void BattleEnd();
	void InitBattle();

	void SettingEndTimeOut();
	void CastleEndOutBattleMap();

private:
	BOOL _bEndTime;
	UINT32 _ui32EndOutBattleTime; //5초뒤 유저 아웃
//////////////////////////////////////////////////////////////
/////						체크						     /////
//////////////////////////////////////////////////////////////
public:
	BOOL CheckCastleNPCAttack(CUnitNPC* pNPC) override;

	INT32 GetBuffNPCCount()
	{  
		INT32 i32Count = 0;
		for (const auto& iter : _vecSummonList)
		{
			if (iter.i32Type == CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_BUFFNPC){
				if (_mapNpc.find(iter.Index) != _mapNpc.end()){
					if (_mapNpc.find(iter.Index)->second != NULL){
						i32Count++;
					}
				}
			}
		}
		return i32Count;
	}
	INT32 GetNPCHpPercent(CASTLE_BATTLE_NPC_TYPE eType)
	{
		for (const auto& iter : _vecSummonList)
		{
			if (iter.i32Type == eType) {
				if (_mapNpc.find(iter.Index) != _mapNpc.end()) {
					if (_mapNpc.find(iter.Index)->second != NULL) {
						return _mapNpc.find(iter.Index)->second->GetHpPercent();
					}
				}
			}
		}
		return 0;
	}
	void CastleMapBroadCastMessage(ENUM_BROADCAST_MESSAGE_TYPE eType, CUnitPC* pPC,INT32 i32Value,BOOL bBroadCast = TRUE);
public:

private:
	vector<stMonsterKillInfo> _vecMonsterKillArray; //천군 얼마나 킬했는지 킬수=레벨
	UINT32 _ui32BroadCastTimeTick;
	INT32 _i32BattleMin;
//////////////////////////////////////////////////////////////
/////					세금 인출					     /////
//////////////////////////////////////////////////////////////
public:
	//세금 인출 요청(금화,동전)
	ENUM_ERROR_MESSAGE ON_CM_Castle_DutyOut(CUnitPC* pPC, INT32 i32Type, INT64 i64Cost);
	//CashDB갔다오고서 처리
	void ON_DSM_CastleDutyOut(BYTE* pPacket);

	void ON_TAX_OutTime();
	void ReturnTax(); //11시에 강제적으로 세금돌려주기
private:
	INT32 _i32MoneyTaxPercent;
	BOOL _bTaxOutTime;
	DWORD _ui32TaxOutTimeTick;
//////////////////////////////////////////////////////////////
/////		IOccupationRule(성주타일점령룰)			     /////
//////////////////////////////////////////////////////////////
public:
	//초기화
	void IOccupationRuleInit()override;
	void IOccupationRuleLoop(DWORD dwDeltick)override;
	//점령타일 밟고 얼마나 있는지
	void EnterUser(CUnitPC* pPC) override;
	void OutEnterUser(CUnitPC* pPC) override;
	//나갔는지 안나갔는지
	BOOL CheckChangeCastleMaster(CUnitPC* _pCastleMaster) override;

	void UpdateBattleTime();
	void ReturnBattleTime();
	void BroadCastOccupationRuleBroadCast(SP_OCCUPATION_STATE* pSendMsg);
private:
	CUnitPC* _pEnterPC; //타일을 밟고있을때
	CUnitPC* _pCastleMaster; //성주가 되었을때
	UINT32 _ui32OccupationTimeTick;
	INT32 i32TimeSecond;
//////////////////////////////////////////////////////////////
/////				IClanFlagRule(클랜 집결)			     /////
//////////////////////////////////////////////////////////////
public:
	void BroadCastEndClanFlag(DWORD dwClanUnique);

	void WarpClanFlag(CUnitPC* pPC) override;
	void SendClanOpenFlag(CUnitPC* pPC) override;

	//FALSE야 사용가능
	BOOL RallyItemCheck(CUnitPC* pPC, DWORD dwClanUnique) override;
	void UseRallyItem(CUnitPC* pPC, DWORD dwClanUnique) override;
private:
	INT32 MyClanRallyAxisX(DWORD dwClanUnique) override;
	INT32 MyClanRallyAxisY(DWORD dwClanUnique)override;
	//클랜 유니크, 정보
	map<DWORD, stCLanRallyData> _mapClanRallyData;
	map<DWORD,vector<DWORD>> _mapvecUseWarpCount;

	//유니크, 사용수
	map<DWORD, INT32> _mapUserRallyUseCount;
	
};

