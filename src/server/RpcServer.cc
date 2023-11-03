#include <mudong-json/include/Document.hpp>

#include "utils/Exception.hpp"
#include "server/RpcService.hpp"
#include "server/RpcServer.hpp"

using namespace mudong::rpc;

namespace {

// 基础版本
template<mudong::json::ValueType dst, mudong::json::ValueType... rest>
void checkValueType(mudong::json::ValueType type) {
    if (dst == type) return;
    if constexpr (sizeof...(rest) > 0) {
        checkValueType<rest...>(type);
    }
    else {
        throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "bad type of at least on one field");
    }
}

// 对基础版本exception包装一层id
template<mudong::json::ValueType... types>
void checkValueType(mudong::json::ValueType type, mudong::json::Value& id) {
    try {
        checkValueType<types...>(type);
    }
    catch(RequestException& e) {
        throw RequestException(e.err(), id, e.detail());
    }
}

template<mudong::json::ValueType... types>
mudong::json::Value& findValue(mudong::json::Value& request, const char* key) {
    static_assert(sizeof...(types) > 0, "expect at least one type");

    auto it = request.findMember(key);
    if (it == request.endMember()) {
        throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "missing at least one field");
    }
    checkValueType<types...>(it->value.getType());
    return it->value;
}

// 包装exception加上id
template<mudong::json::ValueType... types>
mudong::json::Value& findValue(mudong::json::Value& request, mudong::json::Value& id, const char* key) {
    try {
        return findValue<types...>(request, key);
    }
    catch (RequestException& e) {
        throw RequestException(e.err(), id, e.detail());
    }
}

bool hasParams(const mudong::json::Value& request) {
    return request.findMember("params") != request.endMember();
}

// 判断是否为一个notify请求，notify没有id，json-rpc 2.0协议
bool isNotify(const mudong::json::Value& request) {
    return request.findMember("id") == request.endMember();
}

class ThreadSafeBatchResponse {

public:
    explicit ThreadSafeBatchResponse(const RpcDoneCallback& done)
            : data_(std::make_shared<ThreadSafeData>(done))
    {}

    void addResponse(const mudong::json::Value& response) {
        std::lock_guard lock(data_->mutex);
        data_->responses.addValue(response);
    }

private:
    // 给vector<Value>包装一下，添加了mutex和callback对象，使用的时候都lock一下
    struct ThreadSafeData {
        explicit ThreadSafeData(const RpcDoneCallback& done_)
                : responses(mudong::json::ValueType::TYPE_ARRAY),
                  done(done_)
        {}

        ~ThreadSafeData() {
            done(responses); // 最后一个指向data的智能指针也析构了，因此执行callback通知RPC server处理完成了
        }

        std::mutex mutex;
        mudong::json::Value responses; // 这里Value类型的作用就是一个带引用计数的vector<Value>
        RpcDoneCallback done;
    };

    using DataPtr = std::shared_ptr<ThreadSafeData>;
    DataPtr data_;
};

} // anonymous namespace

void RpcServer::addService(std::string_view serviceName, RpcService* service) {
    assert(services_.find(serviceName) == services_.end());
    services_.emplace(serviceName, service);
}

void RpcServer::handleRequest(const std::string& json, const RpcDoneCallback& done) {
    mudong::json::Document request;
    mudong::json::ParseError err = request.parse(json);
    if (err != mudong::json::ParseError::PARSE_OK) {
        throw RequestException(RpcError(ERROR::RPC_PARSE_ERROR), mudong::json::parseErrorStr(err));
    }

    switch (request.getType()) {
        case mudong::json::ValueType::TYPE_OBJECT:
            // Document is a Value，继承关系，里氏替换原则
            if (isNotify(request)) {
                handleSingleNotify(request);
            }
            else {
                handleSingleRequest(request, done);
            }
            break;
        case mudong::json::ValueType::TYPE_ARRAY:
            handleBatchRequests(request, done);
            break;
        default:
            throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "request should be json object or array");
    }
}

void RpcServer::handleSingleRequest(mudong::json::Value& request, const RpcDoneCallback& done) {
    validateRequest(request);

    auto& id = request["id"];
    auto methodName = request["method"].getStringView();
    // 格式为"method":"serviceName.methodName"
    auto pos = methodName.find('.');
    if (pos == std::string_view::npos) {
        throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id, "missing service name in method");
    }

    auto serviceName = methodName.substr(0, pos);
    auto it = services_.find(serviceName);
    if (it == services_.end()) {
        throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id, "service not found");
    }

    methodName.remove_prefix(pos + 1); //移除service name和'.'
    if (methodName.length() == 0) {
        throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id, "missing method name in method field");
    }

    // 只需要确保service name正确存在即可，对methodName的检查将交由对应的service执行，此处不进行检查
    auto& service = it->second;
    service->callProcedureReturn(methodName, request, done);
}

// batch requests就是一个array类型的Value，其中可能包含request，也可能是notify，需要分类处理
void RpcServer::handleBatchRequests(mudong::json::Value& requests, const RpcDoneCallback& done) {
    size_t num = requests.getSize();
    if (num == 0) {
        throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "batch request is empty");
    }

    ThreadSafeBatchResponse responses(done); // 可能存在竞态的点，因此用线程安全的自定义数据类型来存储结果responses的集合

    try {
        size_t n = requests.getSize();
        for (size_t i = 0; i < n; ++i) {
            auto& request = requests[i];

            if (!request.isObject()) {
                throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), "request should be json object");
            }

            if (isNotify(request)) {
                handleSingleNotify(request);
            }
            else {
                handleSingleRequest(request, [&](mudong::json::Value response){ responses.addResponse(response); }); // 执行完之后通过done将结果response添加进结果集当中，thread safe
            }
        }
    }
    catch (RequestException& e) {
        auto response = wrapException(e);
        responses.addResponse(response);
    }
    catch (NotifyException& e) {
        // notify失败是无需给用户返回信息的，因此notify成功与否，用户都应该能接受其结果，用户逻辑不应依赖于notify的成功
        WARN("notify error, code:{}, message:{}, data:{}", e.err().asCode(), e.err().asString(), e.detail());
    }
}

void RpcServer::handleSingleNotify(mudong::json::Value& request) {
    validateNotify(request);

    // 找到匹配的service.method
    auto methodName = request["method"].getStringView();
    auto pos = methodName.find(".");
    if (pos == std::string_view::npos || pos == 0) {
        throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST), "missing service name in method field");
    }

    auto serviceName = methodName.substr(0, pos);
    auto it = services_.find(serviceName);
    if (it == services_.end()) {
        throw NotifyException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), "service not found");
    }

    methodName.remove_prefix(pos + 1);
    if (methodName.size() == 0) {
        throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST), "missing method name in method field");
    }

    auto& service = it->second;
    service->callProcedureNotify(methodName, request);
}

// 确认request合法
void RpcServer::validateRequest(mudong::json::Value& request) {
    auto& id = findValue<mudong::json::ValueType::TYPE_STRING,
                         // mudong::json::ValueType::TYPE_NULL, 认为id null为非法
                         mudong::json::ValueType::TYPE_INT32,
                         mudong::json::ValueType::TYPE_INT64>(request, "id");

    auto& version = findValue<mudong::json::ValueType::TYPE_STRING>(request, id, "jsonrpc");
    if (version.getStringView() != "2.0") {
        throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), id, "jsonrpc version must be 2.0");
    }

    auto& method = findValue<mudong::json::ValueType::TYPE_STRING>(request, id, "method");
    if (method.getStringView() == "rpc.") {
        throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), id, "method name is internal use");
    }

    size_t nMembers = 3u + hasParams(request);

    if (request.getSize() != nMembers) {
        throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), id, "unexpected field");
    }
}

// 确认notify合法
void RpcServer::validateNotify(mudong::json::Value& request) {
    auto& version = findValue<mudong::json::ValueType::TYPE_STRING>(request, "jsonrpc");
    if (version.getStringView() != "2.0") {
        throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST), "jsonrpc version must be 2.0");
    }

    auto& method = findValue<mudong::json::ValueType::TYPE_STRING>(request, "method");
    if (method.getStringView() == "rpc.") {
        throw NotifyException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), "method name is internal use");
    }

    size_t nMembers = 2u + hasParams(request);

    if (request.getSize() != nMembers) {
        throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST), "unexpected field");
    }
}