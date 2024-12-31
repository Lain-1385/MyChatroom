#include "chatservice.hpp"
#include "public.hpp"

#include <iostream>
#include <muduo/base/Logging.h>//log of muduo 
using namespace std;
using namespace muduo;
 
//interfa
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
 
//constructor, register message id and corresponding handler
ChatService::ChatService()
{
    //register msg and corresponding handler for user
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});


    // register group message and corresponding handler
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::clientCloseException, this, _1)});

    if(_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubsribleMessage, this, _1, _2));
    }
}
 
 
//get the corresponding handler according to the message id
MsgHandler ChatService::getHandler(int msgid)
{
    //
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())//if not found
    {
        //return a default empty handler, which does nothing [=] means capture all local variables by value
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) 
        {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";//muduo log will add endl automatically
        };
    }
    else//if found
    {
        return _msgHandlerMap[msgid];//return the corresponding handler
    }
}
 
//handle log in business,  id  pwd   pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // login repeatedly is not allowed
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "repeat login is not allowed";
            conn->send(response.dump()); // callback, return json string
        }
        else
        {
            {
                // login successfully, save the user connection information
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert(make_pair(id, conn));
            }

            //redis channel equals id, every online user has a unique redis channel
            _redis.subscribe(id);
            
            // login successfully, update the user state
            user.setState("online");
            _userModel.updateState(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["errmsg"] = "login success!";
            response["id"] = user.getId();
            response["name"] = user.getName();
            // check if there are offline messages
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // after sending offline messages, delete them from the offline message table
                _offlineMsgModel.remove(id);
            }
            
            //query friend information and return
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump()); // callback, return json string
        }
    }
    else
    {
        // user name or password is wrong
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "user name or password is wrong!";
        conn->send(response.dump()); // callback, return json string
    }
}

 
//handle register business,  name  password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];    // get name
    string pwd = js["password"]; // get password
 
    User user; // create a user object
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user); // insert new user
    if (state)                            
    {
        // insert successfully
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump()); // callback, return json string
    }
    else 
    {
        // insert failed
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump()); // callback, return json string
    }
}

// handle client close exception 
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                // delete the current user from the userconnmap table
                _userConnMap.erase(it);
                break;
            }
        }
    }
    _redis.unsubscribe(user.getId());
    // update the user state information
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// handle one-to-one chat business
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int to_id = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(to_id);
        if (it != _userConnMap.end())
        {
            // if the target user is online on same server, send the message directly
            it->second->send(js.dump());
            return;
        }
    }

    User user = _userModel.query(to_id);
    if(user.getState() == "online")
    {
        // if the target user is online on other server, send the message directly
        _redis.publish(to_id, js.dump());
        return;
    }
    // if the target user is offline, store the message in the offline message table
    _offlineMsgModel.insert(to_id, js.dump());
}



// reset user information when exception occurs
void ChatService::reset()
{
 
    //reset user state to offline
    _userModel.resetState();
}

// add friend business     {"msgid":6,"id":*,"name":"*","friendid":*}
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();               //get user id
    int friendid = js["friendid"].get<int>();       //get friend id
 
    // s
    bool state = _friendModel.insert(userid, friendid);
    if(state){
        //add friend successfully
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["error"] = 0;
        response["errmsg"]="add friend success!";
        response["id"] = userid;
        response["friendid"] = friendid;
        conn->send(response.dump());
    }else{
        //add friend failed
        json response;
        response["msgid"] = ADD_FRIEND_MSG_ACK;
        response["error"] = 1;
        response["errmsg"]="add friend failed!";
        conn->send(response.dump());
    }
}


 
// create a group             {"msgid":8,"id":22,"groupname":"Happy Group","groupdesc":"Just for fun!"}
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // userid of creator of this new group
    string name = js["groupname"];    // group name
    string desc = js["groupdesc"];    // group description
 
    // create a group object, and insert into the database
    Group group(-1, name, desc);
    bool state = _groupModel.createGroup(group);
    if (state)
    {
        // create group successfully
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["error"] = 0;
        response["errmsg"] = "Group creation succeeded!";
        response["groupid"] = group.getId();
        response["groupname"] = group.getName();
        response["groupdesc"] = group.getDesc();
 
        // store the creator of the group into the groupuser table
        bool pstate = _groupModel.addGroup(userid, group.getId(), "creator");
        if (pstate)
        {
            response["creategroup_userid"] = userid;
        }
        conn->send(response.dump());
    }
    else
    {
        // create group failed
        json response;
        response["msgid"] = CREATE_GROUP_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "Group creation failure!";
        conn->send(response.dump());
    }
}

// add user into group     {"msgid":10,"id":15,"groupid":2}
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();       // userid of the user to be added
    int groupid = js["groupid"].get<int>(); // target groupid
    bool pstate = _groupModel.addGroup(userid, groupid, "normal");
    if (pstate)
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["error"] = 0;
        response["errmsg"] = "Successfully join a group!";
        response["userid"] = userid;
        response["groupid"] = groupid;
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = ADD_GROUP_MSG_ACK;
        response["error"] = 1;
        response["errmsg"] = "Failed to join a group!";
        conn->send(response.dump());
    }
}

// chat in group         {"msgid":12,"id":13,"groupid":2,"groupmsg":"Oh.The weather is good today."}
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid); // query all group users except the current user
 
    lock_guard<mutex> lock(_connMutex); // dont allow other threads to access the _userConnMap
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) // if use is online on the same server
        {
            // send group message
            it->second->send(js.dump());
        }
        else 
        {
            User user = _userModel.query(id);
            if (user.getState() == "online" ) // if user is online on other server
            {
                // send group message
                _redis.publish(id, js.dump());
            }
            else
            {
                // store the message in the offline message table
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}   


//get message from subscribed channel
void ChatService::handleRedisSubsribleMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    _offlineMsgModel.insert(userid, msg);
}