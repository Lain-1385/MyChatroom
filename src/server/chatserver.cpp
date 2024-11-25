#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
 
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;
 
//initial chat server object
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    //register connection callback
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));//
 
    //register message callback
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));//
 
    //set thread num of server, 1 I/O thread and 3 worker thread
    _server.setThreadNum(4);
}
 
//start the server
void ChatServer::start()
{
    _server.start();
}

 
//implement the callback function for connection
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //if client is disconnected
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);//handle client close exception
        conn->shutdown();//Close file descriptor
    }
}
 
// implement the callback function for message
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();//
 
    //testing, print the message received
    cout << buf << endl;
 
    //data deserialization
    json js = json::parse(buf);
    //
    //get Handler for message
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());//
    //execute the corresponding Handler
    msgHandler(conn, js, time);
}