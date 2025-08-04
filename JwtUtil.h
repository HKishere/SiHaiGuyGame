#include <string>
#include <chrono>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>  // 使用 JsonCpp 适配器
#include <json/json.h>
#define SECRET_JWT "kishereisgod"

class JWTUtil {
public:
    static std::string generateToken(const std::string& roomId, int playerIdx) {
        // 使用HS256算法，实际项目中应使用更安全的密钥
        auto token = jwt::create<jwt::traits::open_source_parsers_jsoncpp>()
            .set_issuer("game_server")
            .set_type("JWS")
            .set_payload_claim("room_id", jwt::claim(Json::Value(roomId)))
            .set_payload_claim("player_idx", jwt::claim(Json::Value(playerIdx)))
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{1})
            .sign(jwt::algorithm::hs256{SECRET_JWT});
        return token;
    }

    static bool verifyToken(const std::string& token, std::string& outRoomId, int& outPlayerIdx) {
        try {
            auto decoded = jwt::decode<jwt::traits::open_source_parsers_jsoncpp>(token);
            auto verifier = jwt::verify<jwt::traits::open_source_parsers_jsoncpp>()
                .allow_algorithm(jwt::algorithm::hs256{SECRET_JWT})
                .with_issuer("game_server");
            
            verifier.verify(decoded);
            
            outRoomId = decoded.get_payload_claim("room_id").as_string();
            outPlayerIdx = decoded.get_payload_claim("player_idx").as_integer();
            return true;
        } catch (...) {
            return false;
        }
    }
};