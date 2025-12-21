#include <filters/authfilter.hpp>

#include <policyengine.hpp>

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
    const std::optional<std::string> apiKey = ExtractApiKey(req);

    drogon::HttpResponsePtr resp;
    if (apiKey && !m_PolicyEngine->ValidateApiKey(apiKey.value()))
    {
        resp = CreateUnauthorizedResponse("Invalid API Key");
    }
    else if (apiKey && m_PolicyEngine->IsExpired(apiKey.value()))
    {
        resp = CreateUnauthorizedResponse("API Key is expired");
    }
    else if (req->getMethod() == drogon::HttpMethod::Get)
    {
        if ((m_RequireAuthForStatus && req->getPath() == "/status") || m_RequireAuthForRead)
        {
            if (!apiKey || !m_PolicyEngine->ValidateApiKey(apiKey.value(), AccessPermission::READ))
            {
                resp = CreateForbiddenResponse("Invalid permissions for API Key (READ required)");
            }
        }
    }
    else if (req->getMethod() == drogon::HttpMethod::Post)
    {
        if (m_RequireAuthForWrite && (!apiKey || !m_PolicyEngine->ValidateApiKey(apiKey.value(), AccessPermission::WRITE)))
        {
            resp = CreateForbiddenResponse("Invalid permissions for API Key (WRITE required)");
        }
    }

    // Continue to next filter or handler
    const bool allowRequest = !resp;
    if (allowRequest)
    {
        fccb();
    }
    else if(resp)
    {
        fcb(resp);
    }
    else
    {
        nlohmann::json response =
        {
            { "error", "Internal Server Error" },
            { "message" , "Authorization issue that is not correclty handled on the server."},
            { "status", 500 }
        };

        resp = drogon::HttpResponse::newHttpJsonResponse(response.dump(4));
        resp->setStatusCode(drogon::k500InternalServerError);

        fcb(resp);
    }
}

drogon::HttpResponsePtr ApiKeyFilter::CreateUnauthorizedResponse(const std::string& message) const
{
    nlohmann::json response =
    {
        { "error", "Unauthorized" },
        { "message" , message },
        { "status", 401 }
    };

    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k401Unauthorized);
    resp->setBody(std::move(response.dump(4)));
    resp->addHeader("WWW-Authenticate", "ApiKey");
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    return resp;
}

drogon::HttpResponsePtr ApiKeyFilter::CreateForbiddenResponse(const std::string& message) const
{
    nlohmann::json response =
    {
        { "error", "Forbidden" },
        { "message" , message },
        { "status", 403 }
    };

    drogon::HttpResponsePtr resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k403Forbidden);
    resp->setBody(std::move(response.dump(4)));
    resp->addHeader("WWW-Authenticate", "ApiKey");
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

    return resp;
}

std::optional<std::string> ApiKeyFilter::ExtractApiKey(const drogon::HttpRequestPtr& req) const
{
    // Try X-API-Key header first (most common for API keys)
    const std::string apiKeyHeader = req->getHeader("X-API-Key");
    if (!apiKeyHeader.empty()) 
    {
        return apiKeyHeader;
    }

    // Try Authorization header
    const std::string authHeader = req->getHeader("Authorization");
    if (!authHeader.empty()) 
    {
        // Support "Bearer <key>" format
        if (authHeader.find("Bearer ") == 0) 
        {
            return authHeader.substr(7); // Skip "Bearer "
        }

        // Support "ApiKey <key>" format
        if (authHeader.find("ApiKey ") == 0) 
        {
            return authHeader.substr(7); // Skip "ApiKey "
        }

        // Support raw key in Authorization header
        return authHeader;
    }

    return std::nullopt;
}

