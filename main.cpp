#include <cstdio>
#include "GameInstance.h"
#include "WSServer.h"
int main()
{
	printf("%s 启动!\n", "TableGame");
	WSserver * server =  WSserver::GetInstance();
	server->Init();
	GameInstance game(1);
	game.SetWSServer(server);
	game.RunGame();
	return 0;
}