#include <persistence.hpp>

#include <fstream>
#include <iostream>

PersistenceInfo::PersistenceInfo()
    : m_ShouldContinue(true)
    , m_UpdateThread(std::bind(&PersistenceInfo::UpdateThread, this))
    , m_LastWrite(std::chrono::system_clock::now() + std::chrono::years(1))
{
}

PersistenceInfo::~PersistenceInfo()
{
    m_ShouldContinue = false;
    if (m_UpdateThread.joinable())
    {
        m_UpdateThread.join();
    }
}

void PersistenceInfo::UpdateOrAddApiKey(const ApiKey& apiKey)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    bool foundKey = false;
    for (ApiKey& existingKey : m_ApiKeys)
    {
        if (existingKey.GetKey() == apiKey.GetKey())
        {
            existingKey = apiKey;
            foundKey = true;
            break;
        }
    }

    if (!foundKey)
    {
        m_ApiKeys.push_back(apiKey);
    }

    UpdateLastWrite();
}

void PersistenceInfo::Save(nlohmann::json& json) const
{
    nlohmann::json apiKeys = nlohmann::json::array();
    for (const ApiKey& apiKey : m_ApiKeys)
    {
        nlohmann::json json;
        apiKey.Save(json);
        apiKeys.push_back(json);
    }

    json = nlohmann::json
    {
        { "downloads", m_Downloads.load()},
        { "totalRequests", m_TotalRequests.load()},
        { "uploads", m_Uploads.load() },
        { "apiKeys", apiKeys }
    };
}

void PersistenceInfo::Save() const
{
    nlohmann::json json;
    Save(json);

    std::ofstream file(m_Path);
    if (file.is_open())
    {
        file << json;
        file.close();
    }
}

void PersistenceInfo::Load(const nlohmann::json& json)
{
    try
    {
        if (json.contains("downloads"))
        {
            m_Downloads = json.at("downloads").get<uint32_t>();
        }
        if (json.contains("totalRequests"))
        {
            m_TotalRequests = json.at("totalRequests").get<uint32_t>();
        }
        if (json.contains("uploads"))
        {
            m_Uploads = json.at("uploads").get<uint32_t>();
        }

        if (json.contains("apiKeys"))
        {
            nlohmann::json apiKeys = json.at("apiKeys");
            for (nlohmann::json json : apiKeys)
            {
                ApiKey apiKey(json);
                m_ApiKeys.emplace_back(std::move(apiKey));
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "Persistence loading error: " << e.what() << std::endl;
    }
}

void PersistenceInfo::Load()
{
    std::ifstream file(m_Path); // data.json contains your JSON
    if (file.is_open())
    {
        nlohmann::json json;
        file >> json;
        Load(json);
        file.close();
    }
}

void PersistenceInfo::UpdateLastWrite() const
{
    m_LastWrite = std::chrono::system_clock::now();
}

void PersistenceInfo::UpdateThread()
{
    while (m_ShouldContinue)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (m_LastWrite < std::chrono::system_clock::now() && std::chrono::system_clock::now() - m_LastWrite > std::chrono::seconds(5))
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            Save();

            //Until there is another update that needs to be saved, we'll make sure
            //the last write is in the future
            m_LastWrite = std::chrono::system_clock::now() + std::chrono::years(1);
        }
    }
}