#pragma once

#include <mudong-json/include/Value.hpp>

namespace mudong {

namespace rpc {

class StubGenerator {

public:
    explicit StubGenerator(json::Value& proto) {
        parseProto(proto);
    }
    virtual ~StubGenerator() = default;

    virtual std::string genStub() = 0;
    virtual std::string genStubClassName() = 0;

protected:
    struct RpcReturn {
        RpcReturn(const std::string& name_, json::Value& params_, json::Value& returns_)
                : name(name_),
                  params(params_),
                  returns(returns_)
        {}

        std::string name;
        mutable json::Value params;
        mutable json::Value returns;
    };

    struct RpcNotify {
        RpcNotify(const std::string& name_, json::Value& params_)
                : name(name_),
                  params(params_)
        {}

        std::string name;
        mutable json::Value params;
    };

    struct ServiceInfo {
        std::string name;
        std::vector<RpcReturn> rpcReturn;
        std::vector<RpcNotify> rpcNotify;
    };

    ServiceInfo serviceInfo_;

private:
    void parseProto(json::Value& proto);
    void parseRpc(json::Value& rpc);
    void validateParams(json::Value& params);
    void validateReturns(json::Value& returns);

}; // class StubGenerator

// 将str中所有的from字符串替换为to字符串
inline void replaceAll(std::string& str, const std::string& from,  const std::string& to) {
    while (true) {
        size_t i = str.find(from);
        if (i != std::string::npos) str.replace(i, from.size(), to);
        else return;
    }
}

} // namespace rpc

} // namespace mudong