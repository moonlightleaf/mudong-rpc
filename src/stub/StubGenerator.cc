#include <unordered_set>

#include "stub/StubGenerator.hpp"
#include "utils/Exception.hpp"

using namespace mudong::rpc;

namespace {

void expect(bool result, const char* errMsg) {
    if (!result) throw StubException(errMsg);
}

} // anonymous namespace

void StubGenerator::parseProto(json::Value& proto) {
    expect(proto.isObject(), "expect object");
    expect(proto.getSize() == 2, "expect 'name' and 'rpc' fields in object");

    auto nameIter = proto.findMember("name");

    expect(nameIter != proto.endMember(), "missing service name");
    expect(nameIter->value.isString(), "type of service name must be string");
    serviceInfo_.name = nameIter->value.getStringView();

    auto rpcIter = proto.findMember("rpc");
    expect(rpcIter != proto.endMember(), "missing service rpc definition");
    expect(rpcIter->value.isArray(), "rpc field must be array");

    size_t n = rpcIter->value.getSize();
    for (size_t i = 0; i < n; ++i) {
        parseRpc(rpcIter->value[i]);
    }
}

void StubGenerator::parseRpc(json::Value& rpc) {
    expect(rpc.isObject(), "rpc definition must be object");

    auto nameIter = rpc.findMember("name");
    expect(nameIter != rpc.endMember(), "missing name in rpc definition");
    expect(nameIter->value.isString(), "rpc name must be string");

    auto paramsIter = rpc.findMember("params");
    bool hasParams = paramsIter != rpc.endMember();
    if (hasParams) {
        validateParams(paramsIter->value);
    }

    auto returnsIter = rpc.findMember("returns");
    bool hasReturns = returnsIter != rpc.endMember();
    if (hasReturns) {
        validateReturns(returnsIter->value);
    }

    auto paramsValue = hasParams ? paramsIter->value : json::Value(json::ValueType::TYPE_OBJECT); // 如果没有参数传入那就构造一个Object类型的空Value

    if (hasReturns) {
        RpcReturn rr(nameIter->value.getString(), paramsValue, returnsIter->value);
        serviceInfo_.rpcReturn.push_back(rr);
    }
    else {
        RpcNotify rn(nameIter->value.getString(), paramsValue);
        serviceInfo_.rpcNotify.push_back(rn);
    }
}

// 检测参数合法性
void StubGenerator::validateParams(json::Value& params) {
    std::unordered_set<std::string_view> ust; // 用于判断参数名是否重复

    for (auto& p : params.getObject()) {
        auto key = p.key.getStringView();
        auto unique = ust.insert(key).second; // insert第一个返回值为插入后的迭代器，第二个返回值为bool类型，如果元素已经存在则返回false，成功插入就返回true
        expect(unique, "duplicate param name");

        switch (p.value.getType()) {
        case json::ValueType::TYPE_NULL:
            expect(false, "bad param type");
            break;
        default:
            break;
        }
    }
}

// 检测返回值合法性
void StubGenerator::validateParams(json::Value& returns) {
    switch (returns.getType()) {
    case json::ValueType::TYPE_NULL:
    case json::ValueType::TYPE_ARRAY:
        expect(false, "bad returns type");
        break;
    case json::ValueType::TYPE_OBJECT:
        validateReturns(returns);
        break;
    default:
        break;
    }
}

