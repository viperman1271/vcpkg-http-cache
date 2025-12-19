#pragma once

#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>

class PersistenceInfo final
{
public:
    PersistenceInfo();
    ~PersistenceInfo();

    uint32_t GetDownloads() const { return m_Downloads; }
    uint32_t GetTotalRequests() const { return m_TotalRequests; }
    uint32_t GetUploads() const { return m_Uploads; }

    void IncreaseDownloads() { ++m_Downloads; UpdateLastWrite(); }
    void IncreaseTotalRequests() { ++m_TotalRequests; UpdateLastWrite(); }
    void IncreaseUploads() { ++m_Uploads; UpdateLastWrite(); }

    void Save() const;
    void Save(nlohmann::json& json) const;

    void Load();
    void Load(const nlohmann::json& json);

    const std::string& GetPersistencePath() const { return m_Path; }
    void SetPersistencePath(const std::string& path) { m_Path = path; }

private:
    void UpdateLastWrite() const;
    void UpdateThread();

private:
    std::atomic<uint32_t> m_Downloads;
    std::atomic<uint32_t> m_TotalRequests;
    std::atomic<uint32_t> m_Uploads;
    std::atomic<bool> m_ShouldContinue;

    std::string m_Path;
    mutable std::chrono::system_clock::time_point m_LastWrite;
    std::thread m_UpdateThread;
};