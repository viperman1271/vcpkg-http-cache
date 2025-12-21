#include <filters/authfilter.hpp>

#include <nlohmann/json.hpp>

ApiKeyFilter::ApiKeyFilter(std::shared_ptr<PolicyEngine> policyEngine, bool requireAuthForRead, bool requireAuthForWrite, bool requireAuthForStatus)
    : m_PolicyEngine(policyEngine)
    , m_RequireAuthForRead(requireAuthForRead)
    , m_RequireAuthForWrite(requireAuthForWrite)
    , m_RequireAuthForStatus(requireAuthForStatus)
{
}

void ApiKeyFilter::doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& fcb, drogon::FilterChainCallback&& fccb)
{
//     auto api_key = extractApiKey(req);
// 
//     if (!api_key.has_value()) 
//     {
//         auto resp = createUnauthorizedResponse("Missing API key");
//         fcb(resp);
//         return;
//     }
// 
//     // Validate the API key
//     std::string method = drogon::to_string(req->method());
//     auto result = policy_engine_->validateApiKey(api_key.value(), method);
// 
//     if (!result.valid) 
//     {
//         auto resp = createUnauthorizedResponse(result.error_message);
//         fcb(resp);
//         return;
//     }
// 
//     // Add permission info to request attributes for use in handlers
//     req->attributes()->insert("api_key_permission", toString(result.permission));
//     req->attributes()->insert("api_key", api_key.value());

    // Continue to next filter or handler
    fccb();
}

