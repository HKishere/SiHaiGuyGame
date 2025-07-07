#pragma once
#include <libwebsockets.h>
#include <string>
#include <map>
#include <iostream>
#include <memory>

class GameInstance;

class WSserver
{
public:
	static WSserver *GetInstance();
	~WSserver();

	int Init();
	// 向指定客户端发送消息
	void SendToClient(struct lws *wsi, const std::string &message);

	std::map<struct lws *, std::string> client_buffers;
	std::map<std::string, std::unique_ptr<GameInstance>> m_AllGameMap;

private:
	WSserver();
	struct lws_context *m_WScontext;
	static WSserver *m_Instance;
};
