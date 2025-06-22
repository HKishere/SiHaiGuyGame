#pragma once
#include <thread>
#include <vector>
#include <list>
#include "Player.h"
#include <deque>
#include "WSServer.h" 


class GameInstance
{
public:
	GameInstance(int nPlayerNum);
	void RunGame();
	void SetWSServer(WSserver* pWS);

	void GetTreasure();
	void DivideTreasure();
	void SetTargetAndChooseAction();
	Player& GetNextPlayer(int Idx);
	void JudgeDivide();

private:
	std::thread m_tGameThread;
	std::deque<int> m_vecTeasure;
	int m_nPoliceIdx;
	std::vector<Player> m_vecPlayer;
	int m_nBossIdx;
	std::vector<int> m_vecTreasureInTable;
	std::map<int, int> m_TreasureDivideMap; // 表示第 i 份Treasure 给第 j 位玩家

	bool m_bGameOver;
	WSserver* m_WSser;
};

