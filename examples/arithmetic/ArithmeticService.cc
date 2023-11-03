#include "examples/arithmetic/ArithmeticServiceStub.hpp"

using namespace mudong::rpc;

class ArithmeticService : public ArithmeticServiceStub<ArithmeticService> {

public:
    explicit ArithmeticService(RpcServer& server)
            : ArithmeticServiceStub(server), pool_(4)
    {}

    void Add(double lhs, double rhs, const UserDoneCallback& callback) {
        pool_.runTask([=](){
            callback(mudong::json::Value(static_cast<double>(lhs + rhs)));
        });
    }

    void Sub(double lhs, double rhs, const UserDoneCallback& callback) {
        pool_.runTask([=](){
            callback(mudong::json::Value(static_cast<double>(lhs - rhs)));
        });
    }

    void Mul(double lhs, double rhs, const UserDoneCallback& callback) {
        pool_.runTask([=](){
            callback(mudong::json::Value(static_cast<double>(lhs * rhs)));
        });
    }

    void Div(double lhs, double rhs, const UserDoneCallback& callback) {
        pool_.runTask([=](){
            callback(mudong::json::Value(static_cast<double>(lhs / rhs)));
        });
    }

private:
    ThreadPool pool_;
}; // ArithmeticServer

int main() {
    EventLoop loop;
    InetAddress addr(9877);

    RpcServer rpcServer(&loop, addr);
    ArithmeticService service(rpcServer);

    rpcServer.start();
    loop.loop();
}