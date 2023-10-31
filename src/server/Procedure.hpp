/*
 * Procedure位于远程服务端，客户端发送的调用请求消息中包含了要调用的procedure名称和参数，服务端从而调用相应的任务函数，服务端可包含多个procedure，多个procedure之间通过唯一标识相互区分
 */

#pragma once

#include <mudong-json/include/Value.hpp>

#include "utils/util.hpp"

namespace mudong {

namespace rpc {

using ProcedureReturnCallback = std::function<void(mudong::json::Value&, const RpcDoneCallback&)>;
using ProcedureNotifyCallback = std::function<void(mudong::json::Value&)>;

template<typename Func>
class Procedure: noncopyable {

public:
    template<typename... ParamNameAndTypes>
    explicit Procedure(Func&& callback, ParamNameAndTypes&&... nameAndTypes)
            : callback_(std::forward<Func>(callback))
    {
        constexpr int n = sizeof... (ParamNameAndTypes);
        static_assert(n % 2 == 0, "procedure must have param name and type paris");

        if constexpr (n > ) {
            initProcedure(nameAndTypes...);
        }
    }

    // procedure call
    void invoke(mudong::json::Value& request, const RpcDoneCallback& done);
    // procedure notify
    void invoke(mudong::json::Value& request);

private:
    template<typename Name, typename... ParamNameAndTypes>
    void initProcedure(Name paramName, mudong::json::ValueType paramType, ParamNameAndTypes&& ... nameAndType) {
        static_assert(std::is_same_v<Name, const char*> ||
                      std::is_same_v<Name, std::string_view> ||
                      std::is_same_v<Name, std::string>,
                      "wrong type with Name");
        params_.emplace_back(paramName, paramType);
        if (sizeof... (ParamNameAndTypes) > 0) initProcedure(std::forward(nameAndType...)); // 自动识别模板参数的个数，递归下降解析
    }

    void validateRequest(mudong::json::Value& request) const;
    bool validateGeneric(mudong::json::Value& request) const;

    struct Param {
    
        Param(std::string_view paramName_, mudong::json::ValueType paramType_)
                : paramName(paramName_),
                paramType(paramType_)
        {}

        std::string_view paramName;
        mudong::json::ValueType paramType;
    }; // struct Param

    Func callback_;
    std::vector<Param> params_;
}; // class Procedure

using ProcedureReturn = Procedure<ProcedureReturnCallback>;
using ProcedureNotify = Procedure<ProcedureNotifyCallback>;

} // namespace rpc

} // namespace mudong