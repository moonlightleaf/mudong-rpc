#pragma once

#include "stub/StubGenerator.hpp"

namespace mudong {

namespace rpc {

class ClientStubGenerator: public StubGenerator {

public:
    explicit ClientStubGenerator(json::Value& proto)
            : StubGenerator(proto)
    {}

    std::string genStub() override;
    std::string genStubClassName() override;

private:
    std::string genMacroName();
    std::string genProcedureDefinitions();
    std::string genNotifyDefinitions();

    template<typename Rpc>
    std::string genGenericArgs(const Rpc& r, bool appendCommand);
    template<typename Rpc>
    std::string genGenericParamMembers(const Rpc& r);

}; // class ClientStubGenerator

} // namespace rpc

} // namesspace mudong