#pragma once
#include <libwebsockets.h>
#include <string>
#include <map>
#include <iostream>

class GameInstance;

class WSserver
{
public:
	static WSserver* GetInstance();
	~WSserver();

	int Init();
	std::map<struct lws*, std::string> client_buffers;
private:
	std::map<unsigned int, GameInstance*> m_AllGameMap;
	WSserver();
	struct lws_context* m_WScontext;
	static WSserver* m_Instance;

};

 