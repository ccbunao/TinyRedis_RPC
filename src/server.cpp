#include "RedisServer.h"
#include "buttonrpc.hpp"

int main() {
    buttonrpc server;  // 创建一个buttonrpc服务器实例
    server.as_server(5555);  // 将服务器设置为监听端口5555
    //server.bind("redis_command", redis_command);  // 绑定一个名为"redis_command"的函数到服务器，该函数未在代码中定义
    RedisServer::getInstance()->start();  // 启动Redis服务器实例
    server.bind("redis_command", &RedisServer::handleClient, RedisServer::getInstance());  // 绑定一个名为"redis_command"的函数到服务器，该函数是RedisServer类的成员函数，用于处理客户端请求
   // std::cout << "run rpc server on: " << 5555 << std::endl;  // 打印服务器运行信息，但此行被注释掉
    server.run();  // 运行服务器，等待客户端连接和请求

    RedisServer::getInstance()->start();  // 再次启动Redis服务器实例，可能是为了确保服务器正常运行
}
