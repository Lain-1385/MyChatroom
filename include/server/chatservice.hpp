#ifndef CHATSERVICE_H
#define CHATSERVICE_H
 
#include <muduo/net/TcpConnection.h>
#include <unordered_map>//one message id corresponds to one handler
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "groupmodel.hpp"
#include "usermodel.hpp"
#include "json.hpp"
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"

using json = nlohmann::json;
 
//define callback function class for MsgHandler, which is used to process event
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;
 

class ChatService
{
public:
    //get the singleton object
    static ChatService *instance();
    //handle login business
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //handle registration business
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //handle one-to-one chat business
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // add friend business
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);


    // create group business
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // add user to group business
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // chat in group business
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //handle client close exception
    void clientCloseException(const TcpConnectionPtr &conn);

    //reset user information when exception occurs
    void reset();

    //get corresponding Handle for message
    MsgHandler getHandler(int msgid);
private:
    // private constructor, using singleton model
    ChatService();
 
    //store ID of message and corresponding MsgHandler in an unordered_map
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //store the connection information of online users
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //define a mutex to protect the _userConnMap thread safety
    mutex _connMutex;

    //object to modify data
    UserModel _userModel;
    offlineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

};
 
#endif