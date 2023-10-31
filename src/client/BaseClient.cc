#include <mudong-json/include/Document.hpp>
#include <mudong-json/include/StringWriteStream.hpp>
#include <mudong-json/include/Writer.hpp>

#include "client/BaseClient.hpp"
#include "utils/Exception.hpp"

using namespace mudong::rpc;

namespace {

const size_t kMaxMessageLen = 65536;

mudong::json::Value& findValue(mudong::json::Value& value, const char* key, mudong::json::ValueType type) {
    auto it = value.findMember(key);
    if (it == value.endMember()) {
        throw ResponseException("missing field");
    }
    if (it->value.getType() != type) {
        throw ResponseException("bad type");
    }
    return it->value;
}

// 给responseException包装上id
mudong::json::Value& findValue(mudong::json::Value& value, const char* key, mudong::json::ValueType type, int32_t id) {
    try {
        return findValue(value, key, type);
    }
    catch (ResponseException& e) {
        throw ResponseException(e.what(), id);
    }
}

} // anonymous namespace

BaseClient::BaseClient(EventLoop* loop, const InetAddress& serverAddress)
        : id_(0),
          client_(loop, serverAddress)
{
    client_.setMessageCallback(std::bind(&BaseClient::onMessage, this, _1, _2));
}

void BaseClient::start() {
    client_.start();
}

void BaseClient::setConnectionCallback(const ConnectionCallback& callback) {
    client_.setConnectionCallback(callback);
}

//  带回调处理函数的request发送
void BaseClient::sendCall(const TcpConnectionPtr& conn, mudong::json::Value& call, const ResponseCallback& callback) {
    // 收到response时调用callback，因此先将id号和对应callback存档
    call.addMember("id", id_);
    callbacks_[id_] = callback;
    ++id_;

    sendRequest(conn, call);
}

void BaseClient::sendNotify(const TcpConnectionPtr& conn, mudong::json::Value& notify) {
    sendRequest(conn, notify);
}

void BaseClient::sendRequest(const TcpConnectionPtr& conn, mudong::json::Value& request) {
    mudong::json::StringWriteStream os;
    mudong::json::Writer writer(os);
    request.writeTo(writer); // json格式的请求序列化

    auto message = std::to_string(os.getStringView().length() + 2)
            .append("\r\n")
            .append(os.getStringView())
            .append("\r\n");
    
    /* message有header和body两部分组成
    header: body的长度
    body: response + crlf分隔符

    内存分布：header + "\r\n" + body + "\r\n"
    */

    conn->send(message); // 将序列化的消息发送给serverAddress
}

void BaseClient::onMessage(const TcpConnectionPtr& conn, Buffer& buffer) {
    try {
        handleMessage(buffer);
    }
    catch (ResponseException& e) {
        // 如果handleMessage发生异常，那么就抛弃本次请求，并清理对callback的存档，退化为notify
        std::string errMsg;
        errMsg.append("response error: ").append(e.what()).append(", and this request will be abandoned.");
        if (e.hasId()) {
            errMsg += " id: " + std::to_string(e.Id());
            callbacks_.erase(e.Id());
        }
        ERROR(errMsg);
    }
}

void BaseClient::handleMessage(Buffer& buffer) {
    while (true) {
        auto crlf = buffer.findCRLF();
        if (crlf == nullptr) break;

        size_t headerLen = crlf - buffer.peek() + 2; // bodyLength + "\r\n"

        mudong::json::Document header;
        auto err = header.parse(buffer.peek(), headerLen); // 反序列化header
        if (err != mudong::json::ParseError::PARSE_OK || !header.isInt32() || header.getInt32() <= 0) {
            buffer.retrieve(headerLen);
            throw ResponseException("invalid message length in header");
        }

        auto bodyLen = static_cast<uint32_t>(header.getInt32());
        if (bodyLen >= kMaxMessageLen) {
            throw ResponseException("message is too long");
        }

        if (buffer.readableBytes() < headerLen + bodyLen) break; // message实际长度小于header中记录的长度，直接丢弃，做报废处理
        buffer.retrieve(headerLen);
        auto json = buffer.retrieveAsString(bodyLen);
        handleResponse(json); // body交由下层继续处理，这里只负责拆包逻辑
        // message处理完就跳出循环
    }
}

void BaseClient::handleResponse(std::string& json) {
    mudong::json::Document response;
    auto err = response.parse(json);
    if (err != mudong::json::ParseError::PARSE_OK) {
        throw ResponseException(mudong::json::parseErrorStr(err));
    }

    switch (response.getType()) {
        // single response
        case  mudong::json::ValueType::TYPE_OBJECT:
            handleSingleResponse(response);
            break;
        // batch response
        case mudong::json::ValueType::TYPE_ARRAY: {
            size_t n = response.getSize();
            if (n == 0) {
                throw ResponseException("batch response is empty");
            }
            for (size_t i = 0; i < n; ++i) {
                handleSingleResponse(response[i]);
            }
            break;
        }
        default:
            throw ResponseException("response should be json object or array");
    }
}

void BaseClient::handleSingleResponse(mudong::json::Value& response) {
    validateResponse(response);
    auto id = response["id"].getInt32();

    auto it = callbacks_.find(id);
    if (it == callbacks_.end()) {
        WARN("response {} not found in stub", id);
        return;
    }

    auto result = response.findMember("result");
    if (result != response.endMember()) {
        it->second(result->value, false, false); // 后两个bool标志位 isError, isTimeout
    }
    else {
        auto error = response.findMember("error");
        assert(error != response.endMember()); // 本不该不为error，因此debug模式下加此断言
        if (error != response.endMember()) {
            it->second(error->value, true, false); // 对于release版本，实在是错误，那么就抛弃此response，request退化为notify
        }
        else {
            ERROR("response error, this response will be abandoned, id: {}", id);
        }
    }
    callbacks_.erase(it);
}

// 检查response的字段是否都合法且符合预期
void BaseClient::validateResponse(mudong::json::Value& response) {
    if (response.getSize() != 3) {
        throw ResponseException("response should have exactly 3 fields: (jsonrpc, error/result, id)");
    }

    auto id = findValue(response, "id", mudong::json::ValueType::TYPE_INT32).getInt32();

    auto version = findValue(response, "jsonrpc", mudong::json::ValueType::TYPE_STRING, id).getStringView();

    if (version != "2.0") {
        throw ResponseException("unknown json rpc version", id);
    }

    if (response.findMember("result") != response.endMember()) return;

    findValue(response, "error", mudong::json::ValueType::TYPE_OBJECT, id);
}