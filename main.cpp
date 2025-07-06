#include <cstdio>
#include "GameInstance.h"
#include "WSServer.h"
#include "Logger.hpp"
int main()
{
	Logger::getInstance().init();
	LOG_INFO("{} 启动!\n", "TableGame");
	WSserver * server =  WSserver::GetInstance();
	server->Init();
	while (1)
	{
		sleep(1);
	}
	
	return 0;
}