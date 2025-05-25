#pragma once

#pragma region ����

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
	BOOL bReEnter = FALSE; //TRUE�Ͻ� �����Ҷ� ���� �ɾ��ش�.
};
struct stCLanRallyData
{
	DWORD dwClanUnique = 0;
	UINT32 dwRemainTick = 0;
	WORD wX = 0;
	WORD wY = 0;
	WCHAR wcUseCharName[LENGTH_CHARACTER_NICKNAME] = {0,}; //�̸�
	WORD wUseCharGrade = 0; //Ŭ�� ����
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
/////						���� 					     /////
//////////////////////////////////////////////////////////////
public:
	//�Ϲ� �������� �ʺ���
	void SendBattlMap(CUnitPC* pPC) override;
	//���� �� -> ��������
	void WarpBattleMap(CUnitPC* pPC, INT32 i32WarpIndex) override;
	//������ ���� ���
	void AddBattleMapUser(CUnitPC* pPC)override;
	//���忡�� ��û�Ҷ�
	void ShowBattleInfo(CUnitPC* pPC)override;
	
	void OutBattleMap(CUnitPC* pPC,BOOL bClear = FALSE) override;
	INT64 GetDoorDamage(CUnitNPC* pNPC, INT64* pi32Damage)override;

	void MapLoadEndSetting(CUnitPC* pPC) override;

	void CastleBattleSaveData(ENUM_CASTLE_BATTLE_SAVE eType, INT32 i32Index, INT32 Val1 = 0, UINT32 Val2 = 0);
	void BroadCastBattleTime();
	
	void SendCastleBattleTime(CUnitPC* pPC);

//////////////////////////////////////////////////////////////
/////						���� 					     /////
//////////////////////////////////////////////////////////////
	void DeclareCompleate(CUnitPC* pPC, DeclareInfo* stDeclareInfo) override;

	//CP_CASTLE_INFO
	void ON_CM_CastleInfo(CUnitPC* pPC); 
	void ON_Send_CastleInfo(DWORD dwCharunique, INT64 i64GoldBalance,INT64 i64CastleDungeonBalance);


//////////////////////////////////////////////////////////////
/////					���� ���� 					     /////
//////////////////////////////////////////////////////////////

	void CastleNpcKillCheck(CUnitNPC* pNpc, CUnitPC* pPC) override;

	void WarpEndReBuffSetting(CUnitPC* pPC); //�������� ����� ù�ӽ� ���� �ٽðɾ��ٶ�
	void BuffNPCALLKill();
	//���� �־��ٶ�
	void BattleEndReward();
	void BattleEnd();
	void InitBattle();

	void SettingEndTimeOut();
	void CastleEndOutBattleMap();

private:
	BOOL _bEndTime;
	UINT32 _ui32EndOutBattleTime; //5�ʵ� ���� �ƿ�
//////////////////////////////////////////////////////////////
/////						üũ						     /////
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
	vector<stMonsterKillInfo> _vecMonsterKillArray; //õ�� �󸶳� ų�ߴ��� ų��=����
	UINT32 _ui32BroadCastTimeTick;
	INT32 _i32BattleMin;
//////////////////////////////////////////////////////////////
/////					���� ����					     /////
//////////////////////////////////////////////////////////////
public:
	//���� ���� ��û(��ȭ,����)
	ENUM_ERROR_MESSAGE ON_CM_Castle_DutyOut(CUnitPC* pPC, INT32 i32Type, INT64 i64Cost);
	//CashDB���ٿ��� ó��
	void ON_DSM_CastleDutyOut(BYTE* pPacket);

	void ON_TAX_OutTime();
	void ReturnTax(); //11�ÿ� ���������� ���ݵ����ֱ�
private:
	INT32 _i32MoneyTaxPercent;
	BOOL _bTaxOutTime;
	DWORD _ui32TaxOutTimeTick;
//////////////////////////////////////////////////////////////
/////		IOccupationRule(����Ÿ�����ɷ�)			     /////
//////////////////////////////////////////////////////////////
public:
	//�ʱ�ȭ
	void IOccupationRuleInit()override;
	void IOccupationRuleLoop(DWORD dwDeltick)override;
	//����Ÿ�� ��� �󸶳� �ִ���
	void EnterUser(CUnitPC* pPC) override;
	void OutEnterUser(CUnitPC* pPC) override;
	//�������� �ȳ�������
	BOOL CheckChangeCastleMaster(CUnitPC* _pCastleMaster) override;

	void UpdateBattleTime();
	void ReturnBattleTime();
	void BroadCastOccupationRuleBroadCast(SP_OCCUPATION_STATE* pSendMsg);
private:
	CUnitPC* _pEnterPC; //Ÿ���� ���������
	CUnitPC* _pCastleMaster; //���ְ� �Ǿ�����
	UINT32 _ui32OccupationTimeTick;
	INT32 i32TimeSecond;
//////////////////////////////////////////////////////////////
/////				IClanFlagRule(Ŭ�� ����)			     /////
//////////////////////////////////////////////////////////////
public:
	void BroadCastEndClanFlag(DWORD dwClanUnique);

	void WarpClanFlag(CUnitPC* pPC) override;
	void SendClanOpenFlag(CUnitPC* pPC) override;

	//FALSE�� ��밡��
	BOOL RallyItemCheck(CUnitPC* pPC, DWORD dwClanUnique) override;
	void UseRallyItem(CUnitPC* pPC, DWORD dwClanUnique) override;
private:
	INT32 MyClanRallyAxisX(DWORD dwClanUnique) override;
	INT32 MyClanRallyAxisY(DWORD dwClanUnique)override;
	//Ŭ�� ����ũ, ����
	map<DWORD, stCLanRallyData> _mapClanRallyData;
	map<DWORD,vector<DWORD>> _mapvecUseWarpCount;

	//����ũ, ����
	map<DWORD, INT32> _mapUserRallyUseCount;
	
};

