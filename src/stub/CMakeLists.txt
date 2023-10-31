add_executable(mudong-rpc-stub
        StubGenerator.hpp StubGenerator.cc
        ServiceStubGenerator.hpp ServiceStubGenerator.cc
        ClientStubGenerator.hpp ClientStubGenerator.cc
        main.cc)

target_link_libraries(mudong-rpc-stub mudong-json)
install(TARGETS mudong-rpc-stub DESTINATION ${PROJECT_BINARY_DIR}/bin)