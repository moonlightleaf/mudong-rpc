#include <random>
#include <iostream>

#include "examples/arithmetic/ArithmeticClientStub.hpp"

using namespace mudong::rpc;

void run(ArithmeticClientStub& client) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution dis(1, 100);

    double lhs = static_cast<double>(dis(gen));
    double rhs = static_cast<double>(dis(gen));

    client.Add(lhs, rhs, [=](mudong::json::Value response, bool isError, bool timeout) {
        if (!isError) {
            std::cout << lhs << "+" << rhs << "=" << response.getDouble() << std::endl;
        }
        else if (timeout) {
            std::cout << "timeout" << std::endl;
        }
        else { // isError && !timeout
            std::cout << "response: " << response["message"].getStringView() << ": " << response["data"].getStringView() << std::endl;
        }
    });

    client.Sub(lhs, rhs, [=](const mudong::json::Value& response, bool isError, bool timeout) {
        if (!isError) {
            if (response.getType() != mudong::json::ValueType::TYPE_DOUBLE) std::cout << static_cast<unsigned int>(response.getType()) << response.getInt32() << std::endl;
            std::cout << lhs << "-" << rhs << "=" << response.getDouble() << std::endl;
        }
        else if (timeout) {
            std::cout << "timeout" << std::endl;
        }
        else { // isError && !timeout
            std::cout << "response: " << response["message"].getStringView() << ": " << response["data"].getStringView() << std::endl;
        }
    });

    client.Mul(lhs, rhs, [=](const mudong::json::Value& response, bool isError, bool timeout) {
        if (!isError) {
            if (response.getType() != mudong::json::ValueType::TYPE_DOUBLE) std::cout << static_cast<unsigned int>(response.getType()) << std::endl;
            std::cout << lhs << "*" << rhs << "=" << response.getDouble() << std::endl;
        }
        else if (timeout) {
            std::cout << "timeout" << std::endl;
        }
        else { // isError && !timeout
            std::cout << "response: " << response["message"].getStringView() << ": " << response["data"].getStringView() << std::endl;
        }
    });

    client.Div(lhs, rhs, [=](const mudong::json::Value& response, bool isError, bool timeout) {
        if (!isError) {
            std::cout << lhs << "/" << rhs << "=" << response.getDouble() << std::endl;
        }
        else if (timeout) {
            std::cout << "timeout" << std::endl;
        }
        else { // isError && !timeout
            std::cout << "response: " << response["message"].getStringView() << ": " << response["data"].getStringView() << std::endl;
        }
    });
}

int main() {
    EventLoop loop;
    InetAddress addr(9877);
    ArithmeticClientStub client(&loop, addr);

    client.setConnectionCallback([&](const TcpConnectionPtr& conn) {
        if (conn->disconnected()) {
            loop.quit();
        }
        else {
            loop.runEvery(1s, [&]{
                run(client);
            });
        }
    });
    client.start();
    loop.loop();
}