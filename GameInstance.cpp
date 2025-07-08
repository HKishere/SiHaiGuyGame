#include "GameInstance.h"
#include <random>
#include <iostream>
#include <sstream>
#include <map>
#include <iomanip>
#include "Logger.hpp"

constexpr int MAX_TREASURE_NUM = 150;
constexpr int POLICE_TREASURE_VALUE = -1;
constexpr int MIN_VALUE = 1000;
constexpr int MAX_VALUE = 5000;
constexpr int DIVISOR = 500;

int GetRandomtreasure()
{
	std::random_device rd;	// 用于获取随机种子
	std::mt19937 gen(rd()); // 使用Mersenne Twister引擎
	// 计算可能的取值范围 (1000-5000之间能被500整除的数只有1000,1500,2000,2500,3000,3500,4000,4500,5000)
	constexpr int min = (MIN_VALUE + DIVISOR - 1) / DIVISOR; // 向上取整
	constexpr int max = MAX_VALUE / DIVISOR;
	std::uniform_int_distribution<> distrib(min, max);
	int random_multiple = distrib(gen) * DIVISOR;
	return random_multiple;
}
std::string GenerateRoomID()
{
	// 1. 获取当前毫秒时间戳（保证唯一性基础）
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				  now.time_since_epoch())
				  .count();

	// 2. 生成随机数（增加碰撞防护）
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 9999);
	int rand_num = dis(gen);

	// 3. 组合成可读ID（示例格式：R1625091234567-4242）
	std::stringstream ss;
	ss << "R" << ms << "-" << std::setw(4) << std::setfill('0') << rand_num;
	return ss.str();
}

GameInstance::GameInstance(int nPlayerNum, const std::string &strPassWord)
{
	m_ConnectedPlayerNum = 0;
	m_strRoomPassWord = strPassWord;
	m_strRoomID = GenerateRoomID();

	for (int i = 0; i < nPlayerNum; i++)
	{
		m_vecPlayer.emplace_back(Player(i));
	}

	std::random_device rd;	// 用于获取随机种子
	std::mt19937 gen(rd()); // 使用Mersenne Twister引擎
	// 计算可能的取值范围 (1000-5000之间能被500整除的数只有1000,1500,2000,2500,3000,3500,4000,4500,5000)
	constexpr int min = (MIN_VALUE + DIVISOR - 1) / DIVISOR; // 向上取整
	constexpr int max = MAX_VALUE / DIVISOR;
	std::uniform_int_distribution<> distrib(min, max);

	m_vecTeasure.clear();
	// 生成宝藏牌堆
	for (int i = 0; i < MAX_TREASURE_NUM; i++)
	{
		int random_multiple = distrib(gen) * DIVISOR;
		m_vecTeasure.push_back(random_multiple);
	}

	// 3. 从 MAX_TREASURE_NUM * 2/3 ~ MAX_TREASURE_NUM 随机选一个整数
	const int lower_bound = (MAX_TREASURE_NUM * 2) / 3;
	const int upper_bound = MAX_TREASURE_NUM - 1;
	std::uniform_int_distribution<> distrib_count(lower_bound, upper_bound);
	m_nPoliceIdx = distrib_count(gen);
	m_vecTeasure.at(m_nPoliceIdx) = POLICE_TREASURE_VALUE;

	m_vecTreasureInTable.clear();
	m_bGameOver = false;
}

void GameInstance::RunGame()
{
	while (!m_bGameOver)
	{
		GetTreasure();
		if (m_bGameOver)
		{
			break;
		}
		DivideTreasure();
		SetTargetAndChooseAction();
		JudgeDivide();
	}
}

bool GameInstance::AddPlayer(const std::string strPlayerName, lws *WSsocket)
{
	if (m_ConnectedPlayerNum < m_vecPlayer.size()) // 人数不足时才允许加入
	{
		m_vecPlayer.at(m_ConnectedPlayerNum).SetWSsocket(WSsocket);
		m_ConnectedPlayerNum++;
		return true;
	}
	else
	{
		return false;
	}
}

void GameInstance::GetTreasure()
{
	LOG_INFO("start to get Treasure!>>>>>");
	int nTreasureCount = 0;
	if (m_vecTreasureInTable.size() > 0)
	{
		nTreasureCount = 2;
		LOG_INFO("treasure num in table: [{}]", m_vecTreasureInTable.size());
	}
	else
	{
		nTreasureCount = 4;
	}
	for (int i = 0; i < nTreasureCount; i++)
	{
		int nTreasure = m_vecTeasure.front();
		LOG_INFO("NO.{} treasure: [{}]", i, nTreasure);
		m_vecTeasure.pop_front();
		if (nTreasure == POLICE_TREASURE_VALUE)
		{
			m_bGameOver = true;
			return;
		}
	}
}

void GameInstance::DivideTreasure()
{
	m_vecPlayer[m_nBossIdx].DivideTreasure(m_vecTreasureInTable, m_TreasureDivideMap);
}

void GameInstance::SetTargetAndChooseAction()
{
	for (auto &player : m_vecPlayer)
	{
		player.SetAction();
		player.SetTarget();
	}
}

Player &GameInstance::GetNextPlayer(int Idx)
{
	if (static_cast<size_t>(Idx + 1) >= m_vecPlayer.size())
	{
		Idx = 0;
	}
	else
	{
		Idx++;
	}
	return m_vecPlayer[Idx];
}

void GameInstance::JudgeDivide()
{
	int nAggreNum = 0;
	int nOpposeNum = 0;
	auto &player = m_vecPlayer[m_nBossIdx];
	bool bStealFlag = false;
	do
	{
		EAction ePlayerAction = player.GetAction();
		switch (ePlayerAction)
		{
		case EAction::E_Aggre:
			nAggreNum++;
			break;
		case EAction::E_Oppose:
			nOpposeNum++;
			break;
		case EAction::E_Robbery:
			if (m_vecPlayer[player.GetTarget()].GetAction() == EAction::E_Defend) // 失败
			{
				int TreasureLoss = GetRandomtreasure();
				player.LostTreasure(TreasureLoss);
				m_vecPlayer[player.GetTarget()].GetTreasure(TreasureLoss);
			}
			else // 成功
			{
				int TreasureLoss = GetRandomtreasure();
				m_vecPlayer[player.GetTarget()].LostTreasure(TreasureLoss);
				player.GetTreasure(TreasureLoss);
			}
			break;
		case EAction::E_Steal:
			if (!bStealFlag)
			{
				player.GetTreasure(m_vecTeasure.front());
				m_vecTeasure.pop_front();
			}
			break;
		case EAction::E_Defend:
			break;
		default:
			break;
		}
		player = GetNextPlayer(player.GetPlayerIdx());
	} while (player.GetPlayerIdx() != m_nBossIdx);

	if (nAggreNum > nOpposeNum) // 通过，分钱
	{
		for (auto &[TreasureIdx, PlayerIdx] : m_TreasureDivideMap)
		{
			m_vecPlayer[PlayerIdx].GetTreasure(m_vecTreasureInTable[TreasureIdx]);
		}
	}
	else // 分赃失败，下一个当老大
	{
		m_nBossIdx = GetNextPlayer(m_nBossIdx).GetPlayerIdx();
		m_TreasureDivideMap.clear();
	}
}

std::vector<Player> GameInstance::GetAllPlayers()
{
	return m_vecPlayer;
}

bool GameInstance::CheckPassword(const std::string &inputPassword) const
{
	return m_strRoomPassWord.empty() || m_strRoomPassWord == inputPassword;
}
