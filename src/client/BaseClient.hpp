#pragma once

#include <mudong-json/include/Value.hpp>

#include "utils/util.hpp"

namespace mudong {

namespace rpc {

using ResponseCallback = std::function<void(mudong::json::Value&, bool isError, bool isTimeout)>;

class BaseClient : noncopyable {

public:
    BaseClient(EventLoop* loop, const InetAddress& serverAddress);

    void start();

    void setConnectionCallback(const ConnectionCallback& callback);

    void sendCall(const TcpConnectionPtr& conn, mudong::json::Value& call, const ResponseCallback& callback);

    void sendNotify(const TcpConnectionPtr& conn, mudong::json::Value& notify);

private:
    void onMessage(const TcpConnectionPtr& conn, Buffer& buffer);
    void handleMessage(Buffer& buffer);
    void handleResponse(std::string& json);
    void handleSingleResponse(mudong::json::Value& response);
    void validateResponse(mudong::json::Value& response);
    void sendRequest(const TcpConnectionPtr& conn, mudong::json::Value& request);

private:
    using Callbacks = std::unordered_map<int64_t, ResponseCallback>;
    int64_t id_;
    Callbacks callbacks_;
    TcpClient client_;
}; // class BaseClient

} // namespace rpc

} // namespace mudong