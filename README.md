# Mudong-RPC : A JSON-RPC 2.0 framework implemented in C++20

## 项目简介

mudong-rpc是一款基于C++20开发的使用于Linux的RPC（Remote Procedure Call）框架，使用JSON数据格式作为序列化/反序列化方案，实现了JSON-RPC 2.0协议。客户端支持异步RPC调用，也可以通过线程同步达到同步RPC调用的效果。服务端支持基于线程池的多线程RPC，以提高IO线程的响应速度和处理能力上限。服务采用service.method的命名方式，一个TCP端口可以对外提供多个service，每个service中可以含有多个method。

## 开发环境

| Tool | Version |
| :---- | :------: |
| Ubuntu(WSL2) | 20.04 |
| GCC |  13.1.0 |
| CMake | 3.23.0 |
| Git | 2.25.1 |
| clang-format | 10.0.0 |

## 项目架构

<div style="text-align:center">
    <img src="https://cdn.statically.io/gh/moonlightleaf/pics@master/项目记录/rpc说明图白底.6zmiam02ggk0.webp" alt="图片描述" width="50%" height="50%">
</div>

本项目编译生成的文件中包含有一个stub generator，可以根据spec.json中对一组rpc request和response的完整描述，自动生成client端和server端的stub代码。使用时client端（RPC调用方）只需包含生成的ClientStub头文件即可获取到相应的RPC服务，server端（RPC被调用方）除了要包含生成的ServiceStub头文件外，还需实现所提供的RPC服务的具体函数逻辑。

Mudong-RPC中的JSON序列化与反序列化模块基于[Mudong-JSON](https://github.com/moonlightleaf/mudong-json)实现，网络模块基于[Mudong-EV](https://github.com/moonlightleaf/mudong-ev)实现。Mudong-JSON的测试依赖于googletest和benchmark，除此之外无其他依赖。

## 使用示例

每个rpc服务都由一个spec.json文件来描述，arithmetic服务的spec.json文件如下，其中定义了method name、params list和返回值类型：

```json
{
  "name": "Arithmetic",
  "rpc": [
    {
      "name": "Add",
      "params": {"lhs": 1.0, "rhs": 1.0},
      "returns": 2.0
    },
    {
      "name": "Sub",
      "params": {"lhs": 1.0, "rhs": 1.0},
      "returns": 0.0
    },
    {
      "name": "Mul",
      "params": {"lhs": 2.0, "rhs": 3.0},
      "returns": 6.0
    },
    {
      "name": "Div",
      "params": {"lhs": 6.0, "rhs": 2.0},
      "returns": 3.0
    }
  ]
}
```

使用`mudong-rpc-stub`，输入spec.json，将生成client stub和service stub头文件。使用示例如下：

```shell
mudong-rpc-stub -i spec.json -o -c -s
```

`-i`参数表示输入json文件路径，`-o`表示以文件格式输出，`-c`和`-s`分别表示生成客户端和服务端的stub头文件，二者都缺省时表示二者都生成。

对生成的代码format一下，方便阅读：

```
clang-format -i ArithmeticClient.hpp ArithmeticService.hpp
```

项目代码仓库中的**examples**文件夹下，有**arithmetic**示例，给出了客户端和服务端的代码。若添加CMake选项`-DCMAKE_BUILD_EXAMPLES=1`，将会编译该示例。由于CMakeLists文件中以写明了若编译examples则自动执行stub generator程序生成客户端和服务端的stub，并将其作为编译依赖，直接可以得到客户端和服务端的可执行程序。

首先启动arithmetic_server程序，服务端打印日志：

```shell
# server log
20231103 13:14:23.128 [ 8166] [  INFO] create TcpServer 0.0.0.0:9877 - TcpServer.cc:16
20231103 13:14:23.130 [ 8166] [  INFO] TcpServer::start() 0.0.0.0:9877 with 1 eventLoop thread(s) - TcpServer.cc:62
```

随后启动arithmetic_client程序，服务端和客户端打印日志：

```shell
# client log
20231103 13:14:34.516 [ 8328] [  INFO] connected - ArithmeticClientStub.hpp:24

# server log
20231103 13:14:34.516 [ 8166] [ DEBUG] connection 127.0.0.1:48846 is [up] - BaseServer.cc:31
```

客户端连接成功后，将随机生成两个double值，并调用RPC的Add、Sub、Mul、Div服务进行加减乘除：

```shell
# client log
98+62=160
98-62=36
98*62=6076
98/62=1.58065
54+59=113
54-59=-5
54*59=3186
54/59=0.915254
36+39=75
36-39=-3
36*39=1404
36/39=0.923077
```

随后关闭客户端，服务端打印日志：

```shell
# server log
20231103 13:14:43.321 [ 8166] [ DEBUG] connection 127.0.0.1:48846 is [down] - BaseServer.cc:35
```

RPC调用完毕，返回成功。

## 编译&&安装

```shell
$ git clone https://github.com/moonlightleaf/mudong-rpc.git
$ cd mudong-rpc
$ mkdir build && cd build
$ cmake [-DCMAKE_BUILD_EXAMPLES=1] ..
$ make install
```

可以通过选择是否添加`-DCMAKE_BUILD_EXAMPLES=1`选项，来决定是否要对`examples`目录下的文件进行编译。

## 参考

- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)
- [json-rpc-cxx](https://github.com/jsonrpcx/json-rpc-cxx)
- [Mudong-JSON](https://github.com/moonlightleaf/mudong-json): A header-only JSON parser/generator in C++17
- [Mudong-EV](https://github.com/moonlightleaf/mudong-ev): A multi-threaded network library in C++20
