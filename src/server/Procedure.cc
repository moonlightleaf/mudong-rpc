#include "utils/Exception.hpp"
#include "server/Procedure.hpp"

using namespace mudong::rpc;

// 模板特化声明
template class Procedure<ProcedureReturnCallback>;
template class Procedure<ProcedureNotifyCallback>;

// ProcedureRequest和ProcedureNotify的validateRequest分别实现，是因为异常类型的不同；之所以要区分两种异常，是因为notify没有返回值，因此没有id，这是唯一区别
template<>
void Procedure<ProcedureReturnCallback>::validateRequest(mudong::json::Value& request) const {
    switch (request.getType()) {
        case mudong::json::ValueType::TYPE_OBJECT:
        case mudong::json::ValueType::TYPE_ARRAY:
            if (!validateGeneric(request)) {
                throw RequestException(RpcError(ERROR::RPC_INVALID_PARAMS), request["id"], "params name or type mismatch");
            }
            break;
        default:
            throw RequestException(RpcError(ERROR::RPC_INVALID_REQUEST), request["id"], "params type must be object or array");
    }
}

template <>
void Procedure<ProcedureNotifyCallback>::validateRequest(json::Value& request) const {
    switch (request.getType()) {
        case mudong::json::ValueType::TYPE_OBJECT:
        case mudong::json::ValueType::TYPE_ARRAY:
            if (!validateGeneric(request))
                throw NotifyException(RpcError(ERROR::RPC_INVALID_PARAMS), "params name or type mismatch");
            break;
        default:
            throw NotifyException(RpcError(ERROR::RPC_INVALID_REQUEST), "params type must be object or array");
    }
}

template <typename Func>
bool Procedure<Func>::validateGeneric(json::Value& request) const {
    auto mIter = request.findMember("params");
    if (mIter == request.endMember()) {
        return params_.empty();
    }

    auto& params = mIter->value;
    if (params.getSize() == 0 || params.getSize() != params_.size()) { // 不允许"params": []有参数字段，但为空的情况
        return false;
    }

    switch (params.getType()) {
        case mudong::json::ValueType::TYPE_ARRAY:
            for (size_t i = 0; i < params_.size(); i++) {
                if (params[i].getType() != params_[i].paramType) {
                    return false;
                }
            }
            break;
        case mudong::json::ValueType::TYPE_OBJECT:
            for (auto& p : params_) {
                auto it = params.findMember(p.paramName);
                if (it == params.endMember()) {
                    return false;
                }
                if (it->value.getType() != p.paramType) {
                    return false;
                }
            }
            break;
        default:
            return false;
    }
    return true;
}

// ProcedureReturn只会调用此invoke，因此只需对该两形参的invoke模板函数进行实现
template <>
void Procedure<ProcedureReturnCallback>::invoke(json::Value& request, const RpcDoneCallback& done) {
    validateRequest(request);
    callback_(request, done);
}

// ProcedureNotify只会调用此invoke，因此只需对该单形参的invoke模板函数进行实现
template <>
void Procedure<ProcedureNotifyCallback>::invoke(json::Value& request) {
    validateRequest(request);
    callback_(request);
}