#pragma once
#include "CSVFile_CastleInfo.h"
#include "CSVFile_CastleWarpInfo.h"
#include "CSVFile_CastleNPCInfo.h"
#include "CSVFile_CastleLevelInfo.h"

#pragma region 선언
class CUnit;
class CUnitPC;
class CUnitNPC;
class CClan;
struct _npcInfo;

struct stEnterPCInfo
{
	DWORD dwCharunique = 0;
	DWORD dwClanUnique = 0;
};

struct stCastleData
{
	INT32 i32BattleMapIdx = 0;
	INT32 i32RewardMapIdx = 0;
};
struct stCastleWarp
{
	INT32 i32X = 0;
	INT32 i32Y = 0;
};
struct stSummonMonsterList
{
	INT32 Index = 0; //문서상 인덱스
	CASTLE_BATTLE_NPC_TYPE i32Type = CASTLE_BATTLE_NPC_TYPE::CASTLE_NPC_BUFFNPC;		//동문,남문 등
	INT32 i32NpcIndex = 0;
	INT32 i32X = 0;
	INT32 i32Y = 0;
	INT32 i32BuffIndex = 0;
};

enum class ENUM_TEAM
{
	NONE = -1,
	DEFENDER = 0,
	ATTACKER = 1,
};
enum class SP_CLANFLAG_STATE_TYPE
{
	OPEN_FLAG = 0,
	END_FLAG = 1,
	WARP =2,

};

struct stCastleLevelInfo
{
	INT32 i32MinRelayWinCnt = 0;
	INT32 i32BossIndex = 0;
	INT32 i32RewardMapIndex = 0;
	INT32 i32MaxDrawItemCnt = 0;
	INT32 i32EnterX = 0;
	INT32 i32EnterY = 0;
	INT32 i32GoldDungeonSummonList[MAX_CASTLE_GOLD_DUNGEON_NPC_TYPE] = {0,};
};

#pragma endregion

class CCastle
{
public:
	CCastle() { Init(); }

	void Init();
	void LoadData(_CSVCASTLE_DATA* pData);
	void LoadNPCInfo(_CSVCASTLE_NPC_DATA* pData);
	void LoadWarpInfo(_CSV_CASTLE_WARP_DATA* pData);
	void LoadLevelInfo(_CSVCASTLE_LEVEL_DATA* pData);


	void LoadDB(INT32 i32Client) ;
	void SetCastleData(BYTE* pPacket);
	virtual void ON_Load_SaveData(BYTE* pPacket) = 0; //전투중 일대만 셋팅
	void SetSchedule(UINT32 ui32NextTimestamp) ;
	virtual void Loop(DWORD dwDeltick) = 0;
	virtual void SaveDB() = 0;

	BOOL GetDBLoad() { return bDBLoad; }
	BOOL GetScheduleLoad() { return bSchedule; }

	//공성준비
	virtual void OnBattleReady() = 0;
	//공성 10분전
	virtual void OnBattleReadyCompute() = 0;
	//공성 시작
	virtual void OnBattleStart() = 0;
	//공성 종료
	virtual void OnBattleEnd() = 0;


public:
	CClan* GetMasterClan() { return pMasterClan; }
	void SetMasterClan(CClan* pClan) { pMasterClan = pClan; }

	INT32 GetCastleIndex() { return _SaveInfo._i32CastleIndex; }
	INT32 GetCastleClanUnique() { return _SaveInfo._ui32OwnerClanunique; }
	CASTLE_TIME_STATE GetCastleState() { return (CASTLE_TIME_STATE)_SaveInfo._eState; }
	void SetCastleState(CASTLE_TIME_STATE eType)
	{
		_SaveInfo._eState = (INT32)eType; 
		printf("CASTLE_STATE : [%d]\n", (INT32)eType);
	}

	CASTLE_TYPE GetCastleType() { return _eCastleType; }
	INT32 GetMapIndex() { return _i32BattleMapIndex; }
	stCastleSaveInfo* GetSaveInfo() { return &_SaveInfo; }
	
	UINT32 GetEndBattleTimeStamp() { return _ui32EndBattleTimestamp; }
	void SetEndBattleTimeStamp(UINT32 ui32EndTime) { _ui32EndBattleTimestamp = ui32EndTime; }

	UINT32 GetNextStartTimeStamp() { return _ui32NextStarttimestamp; }


	INT32 GetRewardDungeonMapIndex() { return _i32RewardDungeonMapIdx; }
	INT32 GetWinRewardItemIndex() { return _i32WinRewardIdx; }
	INT32 GetLoseRewardItemIndex() { return _i32LoseRwardIdx; }
	//SpecialDungeonInfo.csv //누가 던전인덱스를 이상하게 만들어두어 수정해둠
	INT32 GetGoldRewardDungeonIndex() { return _i32GoldCostDungeonIdx-1; } 

	INT32 GetCastleLevel() { return i32CastleLevelStep; }
	void SettingCastleStep() //레벨
	{
		INT32 i32MaxLevel = 0;
		for (const auto& iter : _mapLevelInfo){
			if (iter.second.i32MinRelayWinCnt <= _SaveInfo._i32RelayCount && i32MaxLevel < iter.first)
			{
				i32MaxLevel = iter.first;
			}
		}
		i32CastleLevelStep = i32MaxLevel;
	}
	INT32 FindCastleStep(INT32 i32RelayWinCount) 
	{
		INT32 i32MaxLevel = 0;
		if (i32RelayWinCount == 0) return 0;
		for (const auto& iter : _mapLevelInfo) {
			if (iter.second.i32MinRelayWinCnt <= i32RelayWinCount  && i32MaxLevel < iter.first)
			{
				i32MaxLevel = iter.first;
			}
		}
		return i32MaxLevel;
	}

	void EraseCastleBuff(CUnitPC* pPC);
	void SetDebuffPer(INT32 i32Per) { _i32CastleDebuffPer = i32Per; };
	INT32 GetDebuffPer() { return _i32CastleDebuffPer; }
//성정보
private:
	BOOL bDBLoad;
	BOOL bSchedule;
	//CASTLE_TYPE_SERVER _eServerType;
	CClan* pMasterClan;
	CASTLE_TYPE _eCastleType;
	
	INT32 _i32MinDeclareCost;
	INT32 _i32MaxDefenceCost;
	INT32 _i32BattleMapIndex;

	INT32 _i32RewardDungeonMapIdx;
	INT32 _i32GoldCostDungeonIdx;

	INT32 _i32WinRewardIdx;
	INT32 _i32LoseRwardIdx;

	INT32 _i32CastleDebuffPer;
protected:
	//팀,장소(인덱스),리스트
	map<INT32, map<INT32, vector<stCastleWarp>>> _mmvCasterWarpInfo;
	vector<stSummonMonsterList> _vecSummonList;
	//문서상Idx, 소환된 몬스터ptr
	map<INT32, CUnitNPC*> _mapNpc;
	//Step, 정보
	map<INT32, stCastleLevelInfo> _mapLevelInfo;
	INT32 i32CastleLevelStep;
//////////////////////////////////////////////////////////////
/////						선포 					     /////
//////////////////////////////////////////////////////////////
public:
	ENUM_ERROR_MESSAGE SettingDefenceCost(CUnitPC* pPC,INT64 i64Cost);
	void DefenceCostCompleate(CUnitPC* pPC, INT64 i64Money);

	void Declare(CUnitPC* pPC,INT64 i64DeclareCost);
	void DeclareAdd(CUnitPC* pPC, INT64 i64Money);
	virtual void DeclareCompleate(CUnitPC* pPC,DeclareInfo* stDeclareInfo) = 0;

	BOOL CheckDeclareList(INT32 ClanUnique);

	void SendDeclareList(CUnitPC* pPC);

	//INT32 SettingTeamUnique() { return ++_i32TeamUnique; }
//////////////////////////////////////////////////////////////
/////					전투 세팅 					     /////
//////////////////////////////////////////////////////////////
public:
	void OutRewardDungeonMap();//보상던전에서 퇴출
	void SummonNpcList(); 

//////////////////////////////////////////////////////////////
/////						체크						     /////
//////////////////////////////////////////////////////////////
	virtual void CastleNpcKillCheck(CUnitNPC* pNpc, CUnitPC* pPC) = 0;
	virtual BOOL CheckCastleNPCAttack(CUnitNPC* pNPC) = 0;

	void BroadCastNPCAttackTime(CASTLE_BATTLE_NPC_TYPE eType);
	BOOL CheckMovableDoorTile(MAP_TILE_TYPE eMapTile);
	INT32 TotalBattleMember();

//////////////////////////////////////////////////////////////
/////						전투 					     /////
//////////////////////////////////////////////////////////////
public:
	//일반 지역에서 맵볼때
	virtual void SendBattlMap(CUnitPC* pPC) = 0;
	//기존 맵 -> 공성전맵
	virtual void WarpBattleMap(CUnitPC* pPC,INT32 i32WarpIndex) = 0;
	//입장한 유저 기록
	virtual void AddBattleMapUser(CUnitPC* pPC) = 0;
	//전장에서 요청할때
	virtual void ShowBattleInfo(CUnitPC* pPC) = 0;

	virtual void OutBattleMap(CUnitPC* pPC,BOOL bClear = FALSE) = 0;
	virtual INT64 GetDoorDamage(CUnitNPC* pNPC, INT64* pi64Damage) = 0;
	virtual void MapLoadEndSetting(CUnitPC* pPC) = 0;

	UINT32 GetTeamunique(CUnitPC* pPC);
	UINT32 GetTeamunique2(CUnitPC* pPC);

protected:
	vector<stEnterPCInfo> _vecBattlePCInfo;
	vector<DeclareInfo> _vecDeclareList;
	UINT32 ui32CastleBattleTimeTick;

private:
	stCastleSaveInfo _SaveInfo;
	UINT32 _ui32NextStarttimestamp;
	UINT32 _ui32EndBattleTimestamp;
	BOOL _bFreeForAll;
	DateTime _LastBattleTime; //아직 안씀

//////////////////////////////////////////////////////////////
/////					혜택관련(보상 던전)			     /////
//////////////////////////////////////////////////////////////
public:
	ENUM_ERROR_MESSAGE EnterRewardDungeon(CUnitPC* pPC);
	ENUM_ERROR_MESSAGE OutRewardDungeon(CUnitPC* pPC,BOOL bClear = FALSE);
	void SummonBossLevel(CGameMap* pMap, INT32* i32BossIndex, INT32* i32BossX, INT32* i32BossY);
	void SetCastleBossNPC(CUnitNPC* pBossNPC) { _pBossNpc = pBossNPC; }
	void KillCastleBossNPC(CUnitNPC* pBossNpc);
	BOOL CheckHuntNPC(CUnitPC* pPC, INT32 i32ItemIndex);
private:
	CUnitNPC* _pBossNpc; //제거용
//////////////////////////////////////////////////////////////
/////				혜택관련(성주 금화 던전)			     /////
//////////////////////////////////////////////////////////////
public:
	void LoadCastleTimeDungeon(); //로드시
	void UpdateCastleCostDungeonSummonList();

	ENUM_ERROR_MESSAGE SettingCastleTimeDungeon(CUnitPC* pPC,INT32 i32SettingCost, BOOL bEnter);
	ENUM_ERROR_MESSAGE BuyCastleTimeDungeonTime(CUnitPC* pPC,INT32 i32Count);
	void ON_CastleDungeon_Buy_Compleate(CUnitPC* pPC,INT32 i32Count);
	void SendCastleDungeonInfo(CUnitPC* pPC);


private:
	map<INT32, vector<_npcInfo*>> _mapTimeDungeonNpcInfo;
	vector<CUnitNPC*> _vecTimeDungeonNpcPtr;
};

//타일점령룰
class IOccupationRule
{
public:
	//초기화
	virtual void IOccupationRuleInit() = 0;
	virtual void IOccupationRuleLoop(DWORD dwDeltick) = 0;
	//점령타일 밟고 얼마나 있는지
	virtual void EnterUser(CUnitPC* pPC) = 0;
	virtual void OutEnterUser(CUnitPC* pPC) = 0;
	//나갔는지 안나갔는지
	virtual BOOL CheckChangeCastleMaster(CUnitPC* pPC) = 0;
};
//집결
class IClanFlagRule
{
public:
	virtual void WarpClanFlag(CUnitPC* pPC) = 0;
	virtual void SendClanOpenFlag(CUnitPC* pPC) = 0;
	//FALSE가 사용가능
	virtual BOOL RallyItemCheck(CUnitPC* pPC, DWORD ClanUnique) = 0;
	virtual void UseRallyItem(CUnitPC* pPC, DWORD ClanUnique) = 0;
private:
	virtual INT32 MyClanRallyAxisX(DWORD ClanUnique) = 0;
	virtual INT32 MyClanRallyAxisY(DWORD ClanUnique) = 0;
};
