add_library(mudong-rpc STATIC
        utils/RpcError.hpp
        utils/Exception.hpp
        utils/util.hpp
        server/BaseServer.hpp server/BaseServer.cc
        server/RpcServer.hpp server/RpcServer.cc
        server/RpcService.hpp server/RpcService.cc
        server/Procedure.hpp server/Procedure.cc
        client/BaseClient.hpp client/BaseClient.cc)
target_link_libraries(mudong-rpc mudong-json mudong-ev)
install(TARGETS mudong-rpc DESTINATION lib)

set(HEADERS
        # utils/RpcError.hpp
        # utils/Exception.hpp
        utils/util.hpp
        server/BaseServer.hpp
        server/RpcServer.hpp
        server/RpcService.hpp
        server/Procedure.hpp
        client/BaseClient.hpp)
install(FILES ${HEADERS} DESTINATION include)

add_subdirectory(stub)