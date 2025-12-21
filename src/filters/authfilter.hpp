#pragma once

#include <drogon/HttpFilter.h>

class PolicyEngine;

/**
 * @brief Drogon HTTP filter for API key authentication and authorization
 *
 * This middleware intercepts HTTP requests and validates API keys from headers.
 * It should be applied to protected endpoints that require authentication.
 */
class ApiKeyFilter : public drogon::HttpFilter<ApiKeyFilter, false> 
{
public:
    /**
     * @brief Construct a new Api Key Filter
     *
     * @param policy_engine Shared pointer to the policy engine
     */
    explicit ApiKeyFilter(std::shared_ptr<PolicyEngine> policyEngine, bool requireAuthForRead = false, bool requireAuthForWrite = false, bool requireAuthForStatus = false);

    /**
     * @brief Filter method called before request handling
     *
     * @param req The HTTP request
     * @param fcb Callback to continue processing if authorized
     * @param fecb Callback to send error response if unauthorized
     */
    void doFilter(const drogon::HttpRequestPtr& req, drogon::FilterCallback&& fcb, drogon::FilterChainCallback&& fecb) override;

private:
    drogon::HttpResponsePtr CreateUnauthorizedResponse(const std::string& message) const;
    drogon::HttpResponsePtr CreateForbiddenResponse(const std::string& message) const;

    std::optional<std::string> ExtractApiKey(const drogon::HttpRequestPtr& req) const;

private:
    std::shared_ptr<PolicyEngine> m_PolicyEngine;

    const bool m_RequireAuthForRead;
    const bool m_RequireAuthForWrite;
    const bool m_RequireAuthForStatus;
};