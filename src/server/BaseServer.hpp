#pragma once

#include <mudong-json/include/Value.hpp>

#include "utils/RpcError.hpp"
#include "utils/util.hpp"

namespace mudong {

namespace rpc {

class RequestException;

// 采用CRTP设计模式，此为共有基类
template<typename ProtocolServer>
class BaseServer : noncopyable {

public:
    void setNumThread(size_t n) {
        server_.setNumThread(n);
    }

    void start() {
        server_.start();
    }

protected:
    // CRTP常用权限控制，参考std::enable_shared_from_this源码
    BaseServer(EventLoop* loop, const InetAddress& listen);
    ~BaseServer() = default;

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer& buffer);
    void onHighWaterMark(const TcpConnectionPtr& conn, size_t mark);
    void onWriteComplete(const TcpConnectionPtr& conn);

    void handleMessage(const TcpConnectionPtr& conn, Buffer& buffer);

    void sendResponse(const TcpConnectionPtr& conn, const json::Value& response);

    ProtocolServer& convert(); // 将Base转换为子类对象类型，CRTP
    const ProtocolServer& convert() const;

protected:
    mudong::json::Value wrapException(RequestException& e);

private:
    TcpServer server_;
}; // class BaseServer

} // namespace rpc

} // namespace mudong