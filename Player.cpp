#include "Player.h"
#include <libwebsockets.h>

Player::Player(int Idx) : m_nPlayerIdx(Idx)
{
	m_bBoss = false;
	m_nTreasure = 3000;
	m_nTargetIdx = -1;
	m_strName = "Player" + std::to_string(Idx + 1);
}

int Player::GetPlayerIdx()
{
	return m_nPlayerIdx;
}

void Player::SetTarget()
{
	
}

int Player::GetTarget()
{
	return m_nTargetIdx;
}

void Player::SetAction()
{
}

EAction Player::GetAction()
{
	return EAction();
}

void Player::GetPlayerTreasureDivide(std::map<int, int>& TreasureDivideMap)
{

}

void Player::DivideTreasure(std::vector<int>& TreasureInTable, std::map<int, int>& TreasureDivideMap)
{
	for (int i = 0; static_cast<size_t>(i) < TreasureDivideMap.size(); i++)
	{
		TreasureDivideMap.insert(std::make_pair(i, 0));
	}
	GetPlayerTreasureDivide(TreasureDivideMap);
}

void Player::GetTreasure(int TreasureValue)
{
	m_nTreasure += TreasureValue;
}

void Player::LostTreasure(int& TreasureValue)
{
	if (TreasureValue > m_nTreasure)
	{
		TreasureValue = m_nTreasure;
	}
	m_nTreasure -= TreasureValue;
}

