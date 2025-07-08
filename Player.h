#pragma once
#include <string>
#include <map>
#include <vector>

struct lws;

enum class EAction
{
	E_Aggre,
	E_Oppose,
	E_Robbery,
	E_Steal,
	E_Defend
};

class Player
{
public:
	Player(int Idx);
	void SetWSsocket(lws *WSsocket) { m_WSsocket = WSsocket; };
	lws *GetWSsocket() { return m_WSsocket; };
	void SetName(const std::string &Name) { m_strName = Name; };
	int GetPlayerIdx();
	void SetTarget();
	int GetTarget();
	void SetAction();
	EAction GetAction();

	void GetPlayerTreasureDivide(std::map<int, int> &TreasureDivideMap);
	void DivideTreasure(std::vector<int> &TreasureInTable, std::map<int, int> &TreasureDivideMap);
	void GetTreasure(int TreasureValue);
	void LostTreasure(int &TreasureValue);

	int m_nTargetIdx;
	std::string m_strName;
	int m_nTreasure;
	bool m_bBoss;
	lws *m_WSsocket;

	int m_nPlayerIdx;
};
