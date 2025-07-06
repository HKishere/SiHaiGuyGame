#include "WSServer.h"
#include <json/json.h>
#include "Logger.hpp"

#define PORT 8080

WSserver* WSserver::m_Instance = nullptr;


// 游戏逻辑处理函数
std::string process_game_command(const std::string& input) {
    // 这里实现你的游戏逻辑
    if (input == "ping") {
        return "pong";
    }
    return "Unknown command: " + input;
}

static int callback_http(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) {
    std::string input((char*)in, len);
    LOG_DEBUG("get http msg: {}", input);
    return 0;
}
static int callback_game(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:// WS客户端连接
        LOG_INFO("Client connected");
        WSserver::GetInstance()->client_buffers[wsi] = "";
        break;

    case LWS_CALLBACK_RECEIVE: // 接收WS消息
    {
        std::string input((char*)in, len);
        LOG_INFO("Received: {}",input);

        // 处理游戏逻辑
        std::string output = process_game_command(input);
        LOG_INFO("Response: {}",output);

        // 准备响应
        WSserver::GetInstance()->client_buffers[wsi] = output;
        lws_callback_on_writable(wsi);
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE: {
        if (!WSserver::GetInstance()->client_buffers[wsi].empty()) {
            unsigned char buf[LWS_PRE + WSserver::GetInstance()->client_buffers[wsi].size()];
            memcpy(&buf[LWS_PRE], WSserver::GetInstance()->client_buffers[wsi].c_str(), WSserver::GetInstance()->client_buffers[wsi].size());
            lws_write(wsi, &buf[LWS_PRE], WSserver::GetInstance()->client_buffers[wsi].size(), LWS_WRITE_TEXT);
            WSserver::GetInstance()->client_buffers[wsi].clear();
        }
        break;
    }

    case LWS_CALLBACK_CLOSED:// WS客户端断开连接
        std::cout << "Client disconnected" << std::endl;
        WSserver::GetInstance()->client_buffers.erase(wsi);
        break;

    default:
        break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "http-only",
        callback_http,
        0,
        0
    },
    {
        "game-protocol",//创建游戏页面时要带上这个协议名
        callback_game,
        0,
        0
    },
    { NULL, NULL, 0, 0 }
};

WSserver::WSserver()
{
}

WSserver* WSserver::GetInstance()
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
    if (!m_WScontext) {
        std::cerr << "Failed to create WebSocket context" << std::endl;
        return -1;
    }
    std::cout << "Server started on port " << PORT << std::endl;

    while (true) {
        lws_service(m_WScontext, 50);
    }
}
