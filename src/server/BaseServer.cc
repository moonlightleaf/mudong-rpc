#include <mudong-json/include/Document.hpp>
#include <mudong-json/include/Writer.hpp>
#include <mudong-json/include/StringWriteStream.hpp>

#include "utils/Exception.hpp"
#include "server/BaseServer.hpp"

using namespace mudong::rpc;

namespace {

const size_t kHighWaterMark = 65536;
const size_t kMaxMessageLen = 100 * 1024 * 1024;

} // anonymous namespace

template<typename ProtocolServer>
BaseServer<ProtocolServer>::BaseServer(EventLoop* loop, const InetAddress& listen)
        : server_(loop, listen)
{
    server_.setConnectionCallback(std::bind(&BaseServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&BaseServer::onMessage, this, _1, _2));
}

template<typename ProtocolServer>
void BaseServer<ProtocolServer>::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        DEBUG("connection {} is [up]", conn->peer().toIpPort());
        conn->setHighWaterMarkCallback(std::bind(&BaseServer:onHighWaterMark, this, _1, _2), kHighWaterMark);
    }
    else {
        DEBUG("connection {} is [down]", conn->peer().toIpPort());
    }
}

template<typename ProtocolServer>
void BaseServer<ProtocolServer>::onMessage(const TcpConnectionPtr& conn, Buffer& buffer) {
    // 尝试处理message
    try {
        handleMessage(conn, buffer);
    }
    // 失败则返回异常信息打包成Value发送给对方，并断开连接
    catch (RequestException& e) {
        mudong::json::Value response = wrapException(e);
        sendRespense(conn, response);
        conn->shutdown();

        WARN("BaseServer::onMessage() {} request error: {}", conn->peer().toIpPort(), e.what());
    }
}

template<typename ProtocolServer>
void BaseServer<ProtocolServer>::onHighWaterMark(const TcpConnectionPtr& conn, size_t mark) {
    DEBUG("connection {} high watermark {}", conn->peer().toIpPort(), mark);

    conn->setWriteCompleteCallback(std::bind(&BaseServer::onWriteComplete, this, _1));
    conn->stopRead();
}

template<typename ProtocolServer>
void BaseServer<ProtocolServer>::onWriteComplete(const TcpConnectionPtr& conn) {
    DEBUG("connection {} write complete", conn->peer().toIpPort());
    conn->startRead();
}

template<typename ProtocolServer>
void BaseServer<ProtocolServer>::handleMessage(const TcpConnectionPtr& conn, Buffer& buffer) {
    // 消息体格式可参看sendResponse代码，按约定好的格式进行解析
    while (true) {
        const char* crlf = buffer.findCRLF(); // 找到下一个回车换行
        if (crlf == nullptr) break; // 无了，就跳出循环
        if (crlf == buffer.peek()) {
            buffer.retrieve(2);
            break;
        }

        size_t headerLen = crlf - buffer.peek() + 2; // +2为包含了crlf的长度

        mudong::json::Document header;
        auto err = header.parse(buffer.peek(), headerLen); // Doc风格解析
        // 必须是一个int32的正数
        if (err != mudong::json::ParseError::PARSE_OK || !header.isInt32() || header.getInt32() <= 0) {
            throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "invalid message length");
        }

        auto jsonLen = static_cast<uint32_t>(header.getInt32());
        if (jsonLen > kMaxMessageLen) {
            throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "message is too long");
        }

        if (buffer.readableBytes() < headerLen + jsonLen) break; // 校验完整性

        buffer.retrieve(headerLen); // 认为header没问题，已解析完故丢弃
        auto json = buffer.retrieveAsString(jsonLen); // 提取buffer中json部分
        // 调用子类类型对象中的handleRequest，CRTP
        convert().handleRequest(json, [conn, this](const mudong::json::Value& response){
            if (!response.isNull()) {
                sendRespense(conn, response);
                TRACE("BaseServer::handleMessage() {} request success", conn->peer().toIpPort());
            }
            else {
                TRACE("BaseServer::handleMessage() {} notify sucess", conn->peer().toIpPort()); // notify是没有response的，按协议无需发送应答给客户端
            }
        });
    }
}

/* exception消息体结构
{
    "jsonrpc":"2.0",
    "error": {
        "code":...,
        "message":...,
        "data":...
    },
    "id":...
}
 */

template<typename ProtocolServer>
mudong::json::Value BaseServer<ProtocolServer>::wrapException(RequestException& e) {
    mudong::json::Value response(mudong::json::ValueType::TYPE_OBJECT);
    response.addMember("jsonrpc", "2.0");
    auto& value = response.addMember("error", static_cast<int>(mudong::json::ValueType::TYPE_OBJECT));
    value.addMember("code", e.err().asCode());
    value.addMember("message", e.err().asString());
    value.addMember("data", e.detail());
    response.addMember("id", e.id());
    return response;
}

template<typename ProtocolServer>
void BaseServer<ProtocolServer>::sendResponse(const TcpConnectionPtr& conn, const mudong::json::Value& response) {
    mudong::json::StringWriteStream os;
    mudong::json::Writer writer(os); // 由writer操作向输出流os写
    response.writeTo(writer); // 函数内部递归下降式调用writeTo，将response通过writer写入os

    auto message = std::to_string(os.getStringView().length() + 2).append("\r\n").append(os.getStringView()).append("\r\n");
    /* message有header和body两部分组成
    header: body的长度
    body: response + crlf分隔符

    内存分布：header + "\r\n" + body + "\r\n"
    */
   conn->send(message);
}

template<typename ProtocolServer>
ProtocolServer& BaseServer<ProtocolServer>::convert() {
    return static_cast<ProtocolServer&>(*this);
}

template<typename ProtocolServer>
const ProtocolServer& BaseServer<ProtocolServer>::convert() const {
    return static_cast<const ProtocolServer&>(*this);
}