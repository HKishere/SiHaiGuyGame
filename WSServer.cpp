#include "WSServer.h"
#include <json/json.h>
#include "Logger.hpp"
#include "GameInstance.h"

#define PORT 8080

WSserver *WSserver::m_Instance = nullptr;

std::string GetJsonMsgAction(const std::string &input)
{
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

    std::string errors;
    const char *inputStart = input.c_str();
    const char *inputEnd = inputStart + input.size();

    // 1. 解析JSON字符串
    if (!reader->parse(inputStart, inputEnd, &root, &errors))
    {
        LOG_ERROR("json string parse errror");
        return "";
    }
    // 2. 检查是否存在action字段
    if (!root.isMember("action") || !root["action"].isString())
    {
        LOG_ERROR("json string do not have key named [action]");
        return "";
    }
    // 3. 提取action值
    std::string action = root["action"].asString();
    LOG_INFO("json [action] : {}", action.c_str());

    return action;
}

std::string CreateGame(const std::string &strJson)
{
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    std::string errors;
    const char *inputStart = strJson.c_str();
    const char *inputEnd = inputStart + strJson.size();

    // 1. 解析JSON字符串
    if (!reader->parse(inputStart, inputEnd, &root, &errors))
    {
        LOG_ERROR("json string parse errror");
        return "";
    }
    // 验证room_info
    if (!root.isMember("room_info") || !root["room_info"].isObject())
    {
        LOG_ERROR("json parse key : [room_info] error");
        return "";
    }

    Json::Value roomInfo = root["room_info"];

    // 验证player_num
    if (!roomInfo.isMember("player_num") || !roomInfo["player_num"].isInt())
    {
        LOG_ERROR("json parse key : [player_num] error");
        return "";
    }

    int playerNum = roomInfo["player_num"].asInt();
    LOG_INFO("json [player_num] {}", playerNum);

    // 获取密码（可选）
    std::string password;
    if (roomInfo.isMember("passwd") && roomInfo["passwd"].isString())
    {
        password = roomInfo["passwd"].asString();
        LOG_INFO("json [passwd] {}", password);
    }
    std::unique_ptr<GameInstance> newGame = std::make_unique<GameInstance>(playerNum, password);
    std::string strRoomId = newGame.get()->GetRoomId();
    LOG_INFO("new game room id : {}", strRoomId);
    WSserver::GetInstance()->m_AllGameMap.emplace(strRoomId, std::move(newGame));
    LOG_INFO("creata game success!");
    return strRoomId;
}

// 游戏逻辑处理函数
std::string process_game_command(const std::string &input)
{
    // 这里实现你的游戏逻辑
    std::string strAction = GetJsonMsgAction(input);
    if (strAction == "create")
    {
        std::string room_id = CreateGame(input);
        if (room_id == "") // 创建失败
        {
            return R"({"action":create, "success":false, "message":"ceate game failed!"})";
        }
        else
        {
            // 成功创建房间的响应
            std::stringstream ss;
            ss << R"({"action":"create","success":true,"room_id":")"
               << room_id
               << R"(","message":"Game created successfully!"})";
            return ss.str();
        }
    }
    return "";
}

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    std::string input((char *)in, len);
    LOG_DEBUG("get http msg: {}", input);
    return 0;
}
static int callback_game(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED: // WS客户端连接
        LOG_INFO("Client connected");
        WSserver::GetInstance()->client_buffers[wsi] = "";
        break;

    case LWS_CALLBACK_RECEIVE: // 接收WS消息
    {
        std::string input((char *)in, len);
        LOG_INFO("Received: {}", input);

        // 处理游戏逻辑
        std::string output = process_game_command(input);
        LOG_INFO("Response: {}", output);

        // 准备响应
        WSserver::GetInstance()->client_buffers[wsi] = output;
        lws_callback_on_writable(wsi);
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        if (!WSserver::GetInstance()->client_buffers[wsi].empty())
        {
            unsigned char buf[LWS_PRE + WSserver::GetInstance()->client_buffers[wsi].size()];
            memcpy(&buf[LWS_PRE], WSserver::GetInstance()->client_buffers[wsi].c_str(), WSserver::GetInstance()->client_buffers[wsi].size());
            lws_write(wsi, &buf[LWS_PRE], WSserver::GetInstance()->client_buffers[wsi].size(), LWS_WRITE_TEXT);
            WSserver::GetInstance()->client_buffers[wsi].clear();
        }
        break;
    }

    case LWS_CALLBACK_CLOSED: // WS客户端断开连接
        std::cout << "Client disconnected" << std::endl;
        WSserver::GetInstance()->client_buffers.erase(wsi);
        break;

    default:
        break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {"http-only",
     callback_http,
     0,
     0},
    {"game-protocol", // 创建游戏页面时要带上这个协议名
     callback_game,
     0,
     0},
    {NULL, NULL, 0, 0}};

WSserver::WSserver()
{
}

WSserver *WSserver::GetInstance()
{
    if (m_Instance == nullptr)
    {
        m_Instance = new WSserver();
    }
    return m_Instance;
}

WSserver::~WSserver()
{
}

int WSserver::Init()
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.protocols = protocols;
    info.port = PORT;
    info.gid = -1;
    info.uid = -1;

    m_WScontext = lws_create_context(&info);
    if (!m_WScontext)
    {
        std::cerr << "Failed to create WebSocket context" << std::endl;
        return -1;
    }
    std::cout << "Server started on port " << PORT << std::endl;

    while (true)
    {
        lws_service(m_WScontext, 50);
    }
}

void WSserver::SendToClient(lws *wsi, const std::string &message)
{
    if (!wsi)
        return;

    // 将消息存入该客户端的缓冲区
    client_buffers[wsi] = message;

    // 标记为可写状态
    lws_callback_on_writable(wsi);
}

// void BroadcastToRoom(const std::string& roomId, const std::string& message) 
// {
//     auto gameIt = WSserver::GetInstance()->m_AllGameMap.find(roomId);
//     if (gameIt != WSserver::GetInstance()->m_AllGameMap.end()) 
//     {
//         for (auto client : gameIt->second->GetConnectedClients()) 
//         {
//             WSserver::GetInstance()->SendToClient(client, message);
//         }
//     }
// }