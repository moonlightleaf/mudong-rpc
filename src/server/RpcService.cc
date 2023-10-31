#include "utils/Exception.hpp"
#include "server/RpcService.hpp"

using namespace mudong::rpc;

void RpcService::callProcedureReturn(std::string_view methodName, mudong::json::Value& request, const RpcDoneCallback& done) {
    auto it = procedureReturn_.find(methodName);
    // 调用的方法名不存在，抛异常。更鲁棒的做法可能是给调用方返回约定好的字段以表示此情况，而非异常
    if (it == procedureReturn_.end()) {
        throw RequestException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), request["id"], "method not found"); 
    }
    it->second->invoke(request, done);
}

void RpcService::callProcedureNotify(std::string_view methodName, mudong::json::Value& request) {
    auto it = procedureNotify_.find(methodName);
    if (it == procedureNotify_.end()) {
        throw NotifyException(RpcError(ERROR::RPC_METHOD_NOT_FOUND), "method not found"); // 同上
    }
    it->second->invoke(request);
}