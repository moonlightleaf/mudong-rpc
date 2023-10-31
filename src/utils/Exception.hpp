#pragma once

#include <exception>
#include <memory>

#include <mudong-json/include/Value.hpp>

#include "utils/RpcError.hpp"

namespace mudong {

namespace rpc {

class NotifyException: public std::exception {

public:
    NotifyException(RpcError err, const char* detail)
            : err_(err),
              detail_(detail)
    {}

    const char* what() const noexcept {
        return err_.asString();
    }

    RpcError err() const {
        return err_;
    }

    const char* detail() const {
        return detail_;
    }

private:
    RpcError err_;
    const char* detail_;

}; // class NotifyException

class RequestException: public std::exception {

public:
    RequestException(const RpcError& err, const mudong::json::Value& id, const char* detail)
            : err_(err),
              id_(new json::Value(id)),
              detail_(detail)
    {}
    RequestException(const RpcError& err, const char* detail)
            : err_(err),
              id_(new json::Value(json::ValueType::TYPE_NULL)),
              detail_(detail)
    {}

    const char* what() const noexcept {
        return err_.asString();
    }

    RpcError err() const {
        return err_;
    }

    json::Value& id() {
        return *id_;
    }

    const char* detail() const {
        return detail_;
    }

private:
    RpcError err_;
    std::unique_ptr<json::Value> id_;
    const char* detail_;
}; // class RequestException

class ResponseException: public std::exception {

public:
    explicit ResponseException(const char* msg)
            : hasId_(false),
              id_(-1),
              msg_(msg)
    {}
    ResponseException(const char* msg, int32_t id)
            : hasId_(true),
              id_(id),
              msg_(msg)
    {}

    const char* what() const noexcept {
        return msg_;
    }

    bool hasId() const {
        return hasId_;
    }

    int32_t Id() const {
        return id_;
    }

private:
    const bool hasId_;
    const int32_t id_;
    const char* msg_;
}; // class ResponseException

class StubException: std::exception {

public:
    explicit StubException(const char* msg)
            : msg_(msg)
    {}

    const char* what() const noexcept {
        return msg_;
    }

private:
    const char* msg_;
}; // class StubException

} // namespace rpc

} // namespace mudong