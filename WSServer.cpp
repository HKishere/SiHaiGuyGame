#include "WSServer.h"
#include <json/json.h>
#include "Logger.hpp"
#include "GameInstance.h"
#include "JwtUtil.h"

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

std::string CreateGame(const std::string &strJson, lws *wsi)
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
        return R"({"action":create, "success":false, "message":"ceate game failed!"})";
    }
    // 验证room_info
    if (!root.isMember("room_info") || !root["room_info"].isObject())
    {
        LOG_ERROR("json parse key : [room_info] error");
        return R"({"action":create, "success":false, "message":"ceate game failed!"})";
    }

    Json::Value roomInfo = root["room_info"];

    // 验证player_num
    if (!roomInfo.isMember("player_num") || !roomInfo["player_num"].isInt())
    {
        LOG_ERROR("json parse key : [player_num] error");
        return R"({"action":create, "success":false, "message":"ceate game failed!"})";
    }

    int playerNum = roomInfo["player_num"].asInt();
    LOG_INFO("json [player_num] {}", playerNum);

    // 获取密码
    std::string password;
    if (roomInfo.isMember("passwd") && roomInfo["passwd"].isString())
    {
        password = roomInfo["passwd"].asString();
        LOG_INFO("json [passwd] {}", password);
    }
    std::unique_ptr<GameInstance> newGame = std::make_unique<GameInstance>(playerNum, password);
    if (newGame == nullptr)
    {
        LOG_ERROR("GameInstance create failed!", password);
        return R"({"action":create, "success":false, "message":"ceate game failed!"})";
    }

    // 获取房间ID
    std::string strRoomId = newGame.get()->GetRoomId();
    LOG_INFO("new game room id : {}", strRoomId);

    // 组合回复报文
    Json::StreamWriterBuilder writer;
    Json::Value response;
    response["action"] = "create";
    response["success"] = true;
    response["message"] = "Game created successfully!";
    response["room_id"] = strRoomId;
    response["player_idx"] = newGame.get()->GetConnectedPlayerNum(); // 从房间信息获取
    response["jwt"] = JWTUtil::generateToken(strRoomId, newGame.get()->GetConnectedPlayerNum());

    // 加入Player到游戏实例中
    std::string strDefaultName = "Player_" + newGame.get()->GetConnectedPlayerNum();
    newGame.get()->AddPlayer(strDefaultName, wsi);

    // 加入map中进行管理
    WSserver::GetInstance()->m_AllGameMap.emplace(strRoomId, std::move(newGame));
    LOG_INFO("creata game success!");

    return Json::writeString(writer, response);
}

std::string WaitGame(const std::string& strJson, lws* wsi)
{
       try {
        // 1. 解析输入JSON
        Json::Value jsonInput;
        Json::Reader reader;
        if (!reader.parse(strJson, jsonInput)) {
            return R"({"error":"Invalid JSON input"})";
        }

        // 2. 获取JWT token
        if (!jsonInput.isMember("token")) {
            return R"({"error":"Missing JWT token"})";
        }
        std::string token = jsonInput["token"].asString();

        // 3. 验证JWT
        auto decoded = jwt::decode<jwt::traits::open_source_parsers_jsoncpp>(token);
        
        // 验证签名和有效期
        // 正确的验证器声明方式
        auto verifier = jwt::verify<jwt::traits::open_source_parsers_jsoncpp>()
            .allow_algorithm(jwt::algorithm::hs256{SECRET_JWT})
            .with_issuer("game_server");

        // 检查必须存在的claim的正确方式
        if (!decoded.has_payload_claim("room_id")) {
            return R"({"error":"Missing room_id in token"})";
        }
        if (!decoded.has_payload_claim("player_idx")) {
            return R"({"error":"Missing player_idx in token"})";
        }

        verifier.verify(decoded);

        // 4. 检查有效期
        auto now = std::chrono::system_clock::now();
        if (decoded.has_expires_at()) {
            auto exp = decoded.get_expires_at();
            if (now > exp) {
                return R"({"error":"Token expired"})";
            }
        }

        // 5. 提取必要信息
        std::string room_id = decoded.get_payload_claim("room_id").as_string();
        int player_idx = decoded.get_payload_claim("player_idx").as_integer();

        // 6. 返回成功响应
        Json::Value jsonResponse;
        jsonResponse["status"] = "success";
        jsonResponse["room_id"] = room_id;
        jsonResponse["player_idx"] = player_idx;
        
        Json::StreamWriterBuilder writer;
        return Json::writeString(writer, jsonResponse);

    } catch (const jwt::error::token_verification_exception& e) {
        return R"({"error":"JWT verification failed: )" + std::string(e.what()) + "\"}";
    } catch (const std::exception& e) {
        return R"({"error":")" + std::string(e.what()) + "\"}";
    } 
}
std::string JoinGame(const std::string& strJson, lws* wsi) 
{
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    std::string errors;
    const char* inputStart = strJson.c_str();
    const char* inputEnd = inputStart + strJson.size();

    // 1. 解析JSON字符串
    if (!reader->parse(inputStart, inputEnd, &root, &errors)) 
    {
        LOG_ERROR("json string parse error");
        return R"({"action":"join","success":false,"message":"Invalid request format"})";
    }

    // 2. 验证必要字段
    if (!root.isMember("room_id") || !root["room_id"].isString()) 
    {
        LOG_ERROR("Missing or invalid room_id");
        return R"({"action":"join","success":false,"message":"Missing room ID"})";
    }

    std::string roomId = root["room_id"].asString();
    std::string password;

    // 3. 获取密码（可选）
    if (root.isMember("passwd") && root["passwd"].isString()) 
    {
        password = root["passwd"].asString();
        LOG_INFO("Join request with password for room: {}", roomId);
    }

    // 4. 查找游戏房间
    auto it = WSserver::GetInstance()->m_AllGameMap.find(roomId);
    if (it == WSserver::GetInstance()->m_AllGameMap.end()) 
    {
        LOG_WARN("Room not found: {}", roomId);
        return R"({"action":"join","success":false,"message":"Room not found"})";
    }

    GameInstance* game = it->second.get();

    // 5. 验证密码
    if (!game->CheckPassword(password)) 
    {
        LOG_WARN("Password mismatch for room: {}", roomId);
        return R"({"action":"join","success":false,"message":"Incorrect password"})";
    }

    int playerIdx = game->GetConnectedPlayerNum();

    // 添加玩家到游戏
    std::string playerName = "Player_" + std::to_string(game->GetConnectedPlayerNum());
    if (!game->AddPlayer(playerName, wsi)) 
    {
        LOG_WARN("Room is full: {}", roomId);
        return R"({"action":"join","success":false,"message":"Room is full"})";
    }

    // 8. 准备响应
    Json::Value response;
    response["action"] = "join";
    response["success"] = true;
    response["room_id"] = roomId;
    response["player_idx"] = playerIdx;
    response["player_name"] = playerName;
    response["message"] = "Joined game successfully";
    response["jwt"] = JWTUtil::generateToken(roomId, playerIdx);
    // // 9. 通知房间内其他玩家
    // Json::Value notifyOthers;
    // notifyOthers["action"] = "player_joined";
    // notifyOthers["player_idx"] = playerIdx;
    // notifyOthers["player_name"] = playerName;
    // notifyOthers["total_players"] = game->GetConnectedPlayerNum();

    Json::StreamWriterBuilder writer;
    // std::string notifyMsg = Json::writeString(writer, notifyOthers);
    
    // // 广播给房间内其他玩家（排除自己）
    // for (const auto& player : game->GetAllPlayers()) 
    // {
    //     if (player.GetWSsocket() != wsi) 
    //     {
    //         WSserver::GetInstance()->SendToClient(player.GetWSsocket(), notifyMsg);
    //     }
    // }

    return Json::writeString(writer, response);
}

// 游戏逻辑处理函数
std::string process_game_command(const std::string &input, lws *wsi)
{
    std::string response = "";
    // 这里实现你的游戏逻辑
    std::string strAction = GetJsonMsgAction(input);
    if (strAction == "create")
    {
        response = CreateGame(input, wsi);
    }
    else if (strAction == "join")
    {
        response = JoinGame(input, wsi);
    }
    else if (strAction == "wait")
    {
        response = WaitGame(input, wsi);
    }
    
    return response;
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
        std::string output = process_game_command(input, wsi);
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
    {"http-only", callback_http, 0, 0},
    {"game-protocol", // 创建游戏页面时要带上这个协议名
     callback_game, 0, 0},
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

void BroadcastToRoom(const std::string &roomId, const std::string &message)
{
    auto gameIt = WSserver::GetInstance()->m_AllGameMap.find(roomId);
    if (gameIt != WSserver::GetInstance()->m_AllGameMap.end())
    {
        for (auto player : gameIt->second->GetAllPlayers())
        {
            WSserver::GetInstance()->SendToClient(player.GetWSsocket(), message);
        }
    }
}