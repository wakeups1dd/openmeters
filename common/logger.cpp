#include "logger.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace openmeters::common {

std::unique_ptr<std::ofstream> Logger::s_logFile = nullptr;
std::recursive_mutex Logger::s_logMutex;
LogLevel Logger::s_minLevel = LogLevel::Info;
bool Logger::s_consoleEnabled = true;
bool Logger::s_initialized = false;

bool Logger::initialize(
    const std::string& logFilePath,
    LogLevel minLevel,
    bool enableConsole
) {
    std::lock_guard<std::recursive_mutex> lock(s_logMutex);
    
    if (s_initialized) {
        return true; // Already initialized
    }
    
    s_minLevel = minLevel;
    s_consoleEnabled = enableConsole;
    
    // Create log directory if it doesn't exist
    try {
        std::filesystem::path logPath(logFilePath);
        auto logDir = logPath.parent_path();
        if (!logDir.empty() && !std::filesystem::exists(logDir)) {
            std::filesystem::create_directories(logDir);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create log directory: " << e.what() << std::endl;
        return false;
    }
    
    // Open log file
    s_logFile = std::make_unique<std::ofstream>(logFilePath, std::ios::app);
    if (!s_logFile->is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        return false;
    }
    
    s_initialized = true;
    
    // Log initialization
    info("Logger initialized - Log file: " + logFilePath);
    
    return true;
}

void Logger::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(s_logMutex);
    
    if (s_initialized && s_logFile) {
        info("Logger shutting down");
        s_logFile->flush();
        s_logFile->close();
        s_logFile.reset();
        s_initialized = false;
    }
}

void Logger::log(
    LogLevel level,
    const std::string& message,
    const char* file,
    int line
) {
    if (level < s_minLevel) {
        return; // Below minimum level
    }
    
    writeLog(level, message, file, line);
}

void Logger::debug(const std::string& message, const char* file, int line) {
    log(LogLevel::Debug, message, file, line);
}

void Logger::info(const std::string& message, const char* file, int line) {
    log(LogLevel::Info, message, file, line);
}

void Logger::warning(const std::string& message, const char* file, int line) {
    log(LogLevel::Warning, message, file, line);
}

void Logger::error(const std::string& message, const char* file, int line) {
    log(LogLevel::Error, message, file, line);
}

void Logger::fatal(const std::string& message, const char* file, int line) {
    log(LogLevel::Fatal, message, file, line);
}

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard<std::recursive_mutex> lock(s_logMutex);
    s_minLevel = level;
}

LogLevel Logger::getMinLevel() {
    std::lock_guard<std::recursive_mutex> lock(s_logMutex);
    return s_minLevel;
}

void Logger::writeLog(
    LogLevel level,
    const std::string& message,
    const char* file,
    int line
) {
    std::lock_guard<std::recursive_mutex> lock(s_logMutex);
    
    if (!s_initialized) {
        // Fallback to console if logger not initialized
        std::cerr << "[FALLBACK] " << levelToString(level) << ": " << message << std::endl;
        return;
    }
    
    std::string timestamp = getTimestamp();
    std::string levelStr = levelToString(level);
    
    // Format: [TIMESTAMP] [LEVEL] [FILE:LINE] MESSAGE
    std::ostringstream logLine;
    logLine << "[" << timestamp << "] [" << levelStr << "]";
    
    if (file) {
        // Extract just filename from path
        std::string filename = file;
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }
        logLine << " [" << filename;
        if (line > 0) {
            logLine << ":" << line;
        }
        logLine << "]";
    }
    
    logLine << " " << message << std::endl;
    
    std::string logEntry = logLine.str();
    
    // Write to file
    if (s_logFile && s_logFile->is_open()) {
        *s_logFile << logEntry;
        s_logFile->flush();
    }
    
    // Write to console
    if (s_consoleEnabled) {
        if (level >= LogLevel::Error) {
            std::cerr << logEntry;
        } else {
            std::cout << logEntry;
        }
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

} // namespace openmeters::common

