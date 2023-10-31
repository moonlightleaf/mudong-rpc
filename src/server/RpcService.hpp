#pragma once

#include <mudong-json/include/Value.hpp>

#include "server/Procedure.hpp"

namespace mudong {

namespace rpc {

class RpcService: noncopyable {

public:
    void addProcedureReturn(std::string_view methodName, ProcedureReturn* p) {
        assert(procedureReturn_.find(methodName) == procedureReturn_.end()); //添加新的ProcedureReturn，一定是之前没有的
        procedureReturn_.emplace(methodName, p);
    }

    void addProcedureNotify(std::string_view methodName, ProcedureNotify* p) {
        assert(procedureNotify_.find(methodName) == procedureNotify_.end()); //同上
        procedureNotify_.emplace(methodName, p);
    }

    void callProcedureReturn(std::string_view methodName, mudong::json::Value& request, const RpcDoneCallback& done);
    void callProcedureNotify(std::string_view methodName, mudong::json::Value& request); // notify无需callback，无返回

private:
    using ProcedureReturnPtr = std::unique_ptr<ProcedureReturn>;
    using ProcedureNotifyPtr = std::unique_ptr<ProcedureNotify>;
    using ProcedureReturnList = std::unordered_map<std::string_view, ProcedureReturnPtr>;
    using ProcedureNotifyList = std::unordered_map<std::string_view, ProcedureNotifyPtr>;

    ProcedureReturnList procedureReturn_;
    ProcedureNotifyList procedureNotify_;

}; // class RpcService

} // namespace rpc

} // namespace mudong