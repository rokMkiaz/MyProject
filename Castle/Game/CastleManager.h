#pragma once

#include "Castle.h"

#pragma region 선언
class CUnit;
class CUnitPC;
class CUnitNPC;
class CGameMap;
struct CP_Attack;
struct _CSV_SKILL_DATA;

enum  class ENUM_CP_CASTLE_INFO_TYPE
{
	INFO = 0,
	DECLARE = 1,
};
enum class ENUM_CASTLE_BATTLEMAP_TYPE 
{
	WARP_WINDOW = 0,
	WARP = 1,
	BATTLE_WINDOW = 2,
	OUT_BATTLEMAP= 3,
	RESURRECTION=4,
};
enum class ENUM_CP_CASTLE_EDIT_TYPE
{
	DEFENCECOST_SETTING = 0,
	CLAN_MASTER_OUT = 1,
	CLAN_STORAGE_OUT = 2,
	REWARD_DUNGEON_ENTER = 3,
	REWARD_DUNGEON_ITEM_COUNT = 4,
	REWARD_DUNGEON_OUT = 5,
	CASTLE_LOSE_OUT = 6,
	SETTING_CASTLE_COST_DUNGEON = 7,
	ADD_CASTLE_COST_DUNGEON_TIME = 8,
};
enum class ENUM_SP_OCCUPATION_STATE
{
	TILE_ENTER = 0,
	LOOP =1,
	TILE_OUT =2 ,
	COMPLEATE =3,
};

#pragma endregion
class CCastleManager
{
public:
	void ON_CM_Request_Castle_Ready_Data(CUnitPC* pPC);
	void ON_Send_CastleReadyData(BYTE* pPacket);

public:
//////////////////////////////////////////////////////////////
/////						초기화					     /////
//////////////////////////////////////////////////////////////
	CCastleManager() { Init(); };
	~CCastleManager() {};
	void Init();
	void LoadData();
	void ON_ConnectGameDB(INT32 i32Client); //DB붙음
	void ON_Load_GameDB(BYTE* pPacket); //DB요청완료
	void OnScheduleConnect(INT32 i32CastleIndex,INT32 i32WorkType); //스케쥴 요청
	void SetCastleTimeStamp(BYTE* pPacket); //다음 공성 스케쥴 도착
	void SetCastleEndBattleStamp(BYTE* pPacket);
	void CastleManagerInitCompleate();
	void CastleBattleLoad(BYTE* pPacket); //전투중 저장리스트 셋팅


	CCastle* FindCastle(INT32 i32CastleIndex) { return _mapCastleList.find(i32CastleIndex)->second; }
	CCastle* FindCastleMapindex(INT32 i32Mapindex)	{
		for (auto finditer : _mapCastleList) {
			if (finditer.second->GetMapIndex() == i32Mapindex) {
				return finditer.second;
			}
		}
		return NULL;
	}
	CCastle* FindCastleRewardMapindex(INT32 i32Mapindex) {
		for (auto finditer : _mapCastleList) {
			if (finditer.second->GetRewardDungeonMapIndex() == i32Mapindex) {
				return finditer.second;
			}
		}
		return NULL;
	}
	map<INT32, CCastle*>* CastleList() { return &_mapCastleList; } //어카운트에서 붙을때만 쓴다.
	//25.02.19_임시로_공성전에서 공격력감소
	void ON_DSM_CASTLE_DEBUFF(INT32 i32CastleIndex, INT32 i32DebuffPer);
	void CASTLE_DAMAGE_DEBUFF(INT64* i64Damage, INT32 i32Mapindex);
//////////////////////////////////////////////////////////////
/////					클라 요청 처리				     /////
//////////////////////////////////////////////////////////////
	//인포요청-> 서버별로 정보를 다르게 가져올 수 있으니 캐스팅하였음.
	void ON_CM_CastleInfo(BYTE* pPacket,CUnitPC* pPC);
	//DB갔다오는 인포요청
	void ON_Send_CastleInfo(BYTE* pPacket,WORD wID);
	void ON_Declare_Add(CUnitPC* pPC, INT32 i32CastleIndex, INT64 i64Money);
	void ON_Declare_Compleate( BYTE* pPacket);
	void ON_CM_CastleDeclareList(CUnitPC* pPC,INT32 i32CastleIndex);

	//전투 보기
	void ON_CM_CastleBattleMap(CUnitPC* pPC, INT32 i32CastleIndex, INT32 i32Type, INT32 i32WarpIndex);

	void ON_CM_Castle_Edit(CUnitPC* pPC, INT32 i32CastleIndex,INT64 i64Cost,INT32 i32Type,BOOL bOpen);

	void ON_CastleDungeon_Buy_Compleate(CUnitPC* pPC, INT32 i32CastleIndex, INT32 i32Count);
	void ON_DefenceCost_Compleate(CUnitPC* pPC, INT32 i32CastleIndex, INT64 i64Cost);
	void ON_DSM_CastleDutyOut(BYTE* pPacket);


//////////////////////////////////////////////////////////////
/////						스케줄					     /////
//////////////////////////////////////////////////////////////
	void OnBattleReady(INT32 i32CastleIdx);
	void OnBattleReadyCompute(INT32 i32CastleIdx);
	void OnBattleStart(INT32 i32CastleIdx);
	void OnBattleEnd(INT32 i32CastleIdx);

	void BroadCastCastleState(INT32 i32CastleIndex,CASTLE_TIME_STATE eType);
	void SendBattleBagde(CUnitPC* pPC);
	

//////////////////////////////////////////////////////////////
/////						전투관련					     /////
//////////////////////////////////////////////////////////////
	void CastleNPCKill(CUnitNPC* pNpc, CUnitPC* pPC);
	BOOL CheckNPCAttackTime(CUnitNPC* pNpc);
	BOOL CheckDoorTile(INT32 i32Mapindex,MAP_TILE_TYPE eMapTile);
	INT64 GetDoorDamage(CUnitNPC* pNpc, INT64* pi32Damage);
	void BattleMapEndSetting(CUnitPC* pPC);
//////////////////////////////////////////////////////////////
/////						기타						     /////
//////////////////////////////////////////////////////////////
	BOOL CheckCastleMap(INT32 i32Mapindex);
	BOOL CheckCastleRewardMap(INT32 i32Mapindex);
	BOOL CheckCastleBattleCheck(INT32 i32GuildUnique);
	BOOL CheckClanAction(DWORD dwGuildUnique);
	BOOL CheckCastleBattleOpen(); //25.01.09_남일우_공성 서버별변경
	void EraseCastleBuff(CUnitPC* pPC);
//////////////////////////////////////////////////////////////
/////						보상던전					     /////
//////////////////////////////////////////////////////////////
	void SummonCastleBossNPCIndex(INT32 i32RewardMapindex, INT32* pSummonBossIndex);
	void SettingCastleBossNPC(CUnitNPC* pBossNPC);
	void KillCastleBossNPC(CUnitNPC* pBossNpc);
	BOOL HuntRewardDungeon(CUnitPC* pPC, INT32 i32ItemIndex);

	void SendCastleDungeonInfo(CUnitPC* pPC);

//////////////////////////////////////////////////////////////
/////					인터페이스 관리자				     /////
//////////////////////////////////////////////////////////////
public:
	void OccupationRuleEnterUser(CUnitPC* pPC) ;
	void OccupationRuleLoop();

	BOOL ClanFlagRallyItemFailCheck(CUnitPC* pPC, DWORD ClanUnique);
	void ClanFlagUseRallyItem(CUnitPC* pPC, DWORD ClanUnique);
	void ClanFlagWarpClanFlag(CUnitPC* pPC);
	void ClanFlagSendClanOpenFlag(CUnitPC* pPC);
private:
	//성Idx, ptr
	map<INT32,IOccupationRule*> _mapIOccupationRuleList; //타일점령룰
	map<INT32, IClanFlagRule*> _mapIClanFlagRuleList; //집결룰

public:
	void Loop();
	void SaveDB();

	BOOL mbCastleLoop;
private:
	//Index
	map<INT32,CCastle*> _mapCastleList;
	ULONGLONG _ullCurTime;
	ULONGLONG _ullPrevTime;

	ULONGLONG _ullOccupationRuleCurTime;
	ULONGLONG _ullOccupationRulePrevTime;

};

extern  CCastleManager g_CastleManager;