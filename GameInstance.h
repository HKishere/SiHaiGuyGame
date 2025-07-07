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
	GameInstance(int nPlayerNum, const std::string& strPassWord);
	void RunGame();
	void AddPlayer(const std::string strPlayerName, lws* WSsocket);

	void GetTreasure();
	void DivideTreasure();
	void SetTargetAndChooseAction();
	Player& GetNextPlayer(int Idx);
	void JudgeDivide();
	std::string GetRoomId(){return m_strRoomID;};
	int GetConnectedPlayerNum(){return m_ConnectedPlayerNum;};

private:
	std::thread m_tGameThread;
	std::deque<int> m_vecTeasure;
	int m_nPoliceIdx;
	std::vector<Player> m_vecPlayer;
	int m_nBossIdx;
	std::vector<int> m_vecTreasureInTable;
	std::map<int, int> m_TreasureDivideMap; // 表示第 i 份Treasure 给第 j 位玩家

	bool m_bGameOver;
	bool m_bGameStart;
	
	std::string m_strRoomID;
	std::string m_strRoomPassWord;

	int m_ConnectedPlayerNum;
};

