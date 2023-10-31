#pragma once

#include <string>

#include <mudong-json/include/Value.hpp>

#include <mudong-ev/src/EventLoop.hpp>
#include <mudong-ev/src/TcpConnection.hpp>
#include <mudong-ev/src/TcpServer.hpp>
#include <mudong-ev/src/TcpClient.hpp>
#include <mudong-ev/src/InetAddress.hpp>
#include <mudong-ev/src/Buffer.hpp>
#include <mudong-ev/src/Logger.hpp>
#include <mudong-ev/src/Callbacks.hpp>
#include <mudong-ev/src/Timestamp.hpp>
#include <mudong-ev/src/ThreadPool.hpp>
#include <mudong-ev/src/CountDownLatch.hpp>

namespace mudong {

namespace rpc {

using namespace std::literals::string_view_literals;
using namespace std::literals::chrono_literals;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

using ev::EventLoop;
using ev::TcpConnection;
using ev::TcpServer;
using ev::TcpClient;
using ev::InetAddress;
using ev::TcpConnectionPtr;
using ev::noncopyable;
using ev::Buffer;
using ev::ConnectionCallback;
using ev::ThreadPool;
using ev::CountDownLatch;

using RpcDoneCallback = std::function<void(json::Value response)>;

class UserDoneCallback {

public:
    UserDoneCallback(json::Value &request, const RpcDoneCallback &callback)
            : request_(request),
              callback_(callback)
    {}


    void operator()(json::Value &&result) const {
        json::Value response(json::ValueType::TYPE_OBJECT);
        response.addMember("jsonrpc", "2.0");
        response.addMember("id", request_["id"]);
        response.addMember("result", result);
        callback_(response);
    }

private:
    mutable json::Value request_;
    RpcDoneCallback callback_;

}; // class UserDoneCallback

} // namespace rpc

} // namespace mudong