#include <persistence.hpp>

#include <fstream>

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

void PersistenceInfo::Save(nlohmann::json& json) const
{
    json = nlohmann::json
    {
        { "downloads", m_Downloads.load()},
        { "totalRequests", m_TotalRequests.load()},
        { "uploads", m_Uploads.load() }
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
    m_Downloads = json.at("downloads").get<uint32_t>();
    m_TotalRequests = json.at("totalRequests").get<uint32_t>();
    m_Uploads = json.at("uploads").get<uint32_t>();
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
            Save();

            //Until there is another update that needs to be saved, we'll make sure
            //the last write is in the future
            m_LastWrite = std::chrono::system_clock::now() + std::chrono::years(1);
        }
    }
}