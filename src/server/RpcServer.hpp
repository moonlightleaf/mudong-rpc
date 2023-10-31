#pragma once

#include <memory>
#include <unordered_map>

#include <mudong-json/include/Value.hpp>

#include "utils/util.hpp"
#include "server/RpcService.hpp"
#include "server/BaseServer.hpp"

namespace mudong {

namespace rpc {

// CRTP设计模式的子类实现
class RpcServer : public BaseServer<RpcServer> {

public:
    RpcServer(EventLoop* loop, const InetAddress& listen)
            : BaseServer(loop, listen)
    {}
    ~RpcServer() = default;

    // called by user stub
    void addService(std::string_view serviceName, RpcService* service);
    // called by connection manager
    void handleRequest(const std::string& json, const RpcDoneCallback& done);

private:
    void handleSingleRequest(mudong::json::Value& request, const RpcDoneCallback& done);
    void handleBatchRequests(mudong::json::Value& request, const RpcDoneCallback& done);
    void handleSingleNotify(mudong::json::Value& request);

    void validateRequest(mudong::json::Value& request);
    void validateNotify(mudong::json::Value& request);

private:
    using RpcServicePtr = std::unique_ptr<RpcService>;
    using ServiceList = std::unordered_map<std::string_view, RpcServicePtr>;

    // RpcServer管理RpcService，RpcService管理Procedure
    ServiceList services_;
}; // class RpcServer

} // namespace rpc

} // namespace mudong