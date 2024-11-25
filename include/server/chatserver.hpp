#ifndef CHATSERVER_H
#define CHATSERBER_H
 
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
 
using namespace muduo;
using namespace muduo::net;

//chat server class
class ChatServer
{
public:
    // initialize the server
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);
    // start the server
    void start();

 
private:
    //callback function for connection
    void onConnection(const TcpConnectionPtr &);
    //callback function for message
    void onMessage(const TcpConnectionPtr &,
                   Buffer *,
                   Timestamp);
    TcpServer _server;      //TCP server object
    EventLoop *_loop;        //eventloop object pointer
};
 
#endif