#ifndef LOGGER_H
#define LOGGER_H

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>  // 控制台彩色输出
#include <filesystem>
#include <stdexcept>

#define MAX_LOG_FILE_SIZE_DEFAULT 50000000
#define MAX_LOG_FILE_NUM_DEFAULT 5

class Logger
{
public:
    // 删除拷贝构造函数和赋值运算符
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    // 获取单例实例
    static Logger &getInstance()
    {
        static Logger instance;
        return instance;
    }

    // 初始化日志系统
    void init(size_t max_file_size = MAX_LOG_FILE_SIZE_DEFAULT, size_t max_files = MAX_LOG_FILE_NUM_DEFAULT, const std::string &logFilePath = "logs/game_server.log")
    {
        try
        {
            // 确保日志目录存在
            std::filesystem::path path(logFilePath);
            if (!path.parent_path().empty()) {
                std::filesystem::create_directories(path.parent_path());
            }

            // 创建两个sink：控制台和文件
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logFilePath, max_file_size, max_files);

            // 设置日志格式
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

            // 创建logger并添加两个sink
            logger_ = std::make_shared<spdlog::logger>("multi_sink", 
                    spdlog::sinks_init_list({console_sink, file_sink}));

            // 设置全局日志级别
            logger_->set_level(spdlog::level::trace);
            
            // 注册logger
            spdlog::register_logger(logger_);
            
            // 刷新策略
            logger_->flush_on(spdlog::level::warn);
        }
        catch (const std::exception &ex)
        {
            throw std::runtime_error(std::string("Failed to initialize logger: ") + ex.what());
        }
    }

    // 获取日志器
    std::shared_ptr<spdlog::logger> getLogger() const
    {
        if (!logger_)
        {
            throw std::runtime_error("Logger not initialized");
        }
        return logger_;
    }

    // 设置日志级别
    void setLevel(spdlog::level::level_enum level)
    {
        getLogger()->set_level(level);
    }

private:
    Logger() = default;
    ~Logger()
    {
        spdlog::drop_all();
    }

    std::shared_ptr<spdlog::logger> logger_;
};

// 改进后的日志宏，明确支持多参数
#define LOG_DEBUG(...) Logger::getInstance().getLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().getLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().getLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().getLogger()->error(__VA_ARGS__)

#endif // LOGGER_H
