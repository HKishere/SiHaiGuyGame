#include "WSServer.h"

#define PORT 8080

WSserver* WSserver::m_Instance = nullptr;


// ��Ϸ�߼�������
std::string process_game_command(const std::string& input) {
    // ����ʵ�������Ϸ�߼�
    if (input == "ping") {
        return "pong";
    }
    return "Unknown command: " + input;
}

static int callback_http(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) {
    return 0;
}
static int callback_game(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:// WS�ͻ�������
        std::cout << "Client connected" << std::endl;
        WSserver::GetInstance()->client_buffers[wsi] = "";
        break;

    case LWS_CALLBACK_RECEIVE: // ����WS��Ϣ
    {
        std::string input((char*)in, len);
        std::cout << "Received: " << input << std::endl;

        // ������Ϸ�߼�
        std::string output = process_game_command(input);

        // ׼����Ӧ
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

    case LWS_CALLBACK_CLOSED:// WS�ͻ��˶Ͽ�����
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
        "game-protocol",
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
