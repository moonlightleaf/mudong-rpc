add_custom_command(
    OUTPUT RAW_HEADER
    COMMAND mudong-rpc-stub
    ARGS -o -i ${CMAKE_CURRENT_SOURCE_DIR}/spec.json
    MAIN_DEPENDENCY spec.json
    DEPENDS mudong-rpc-stub
    COMMENT "Generating Service/Client Stub..."
    VERBATIM
)

set(stub_dir ${PROJECT_BINARY_DIR}/examples/arithmetic)

add_custom_command(
    OUTPUT HEADER
    COMMAND clang-format
    ARGS -i ${stub_dir}/ArithmeticServiceStub.hpp ${stub_dir}/ArithmeticClientStub.hpp
    DEPENDS RAW_HEADER
    COMMENT "clang format stub file..."
    VERBATIM
)

add_executable(arithmetic_server ArithmeticService.cc HEADER)
target_link_libraries(arithmetic_server mudong-rpc)
install(TARGETS arithmetic_server DESTINATION bin)

add_executable(arithmetic_client ArithmeticClient.cc HEADER)
target_link_libraries(arithmetic_client mudong-rpc)
install(TARGETS arithmetic_server DESTINATION bin)