
#include "json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"
using json = nlohmann::json;
 

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
 
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>
 
// global variable
User g_currentUser;                   // record the current login user
vector<User> g_currentUserFriendList; // record the current login user's friend list information
vector<Group> g_currentUserGroupList; // record the current login user's group list information
bool isMainMenuRunning = false;       // main menu control flag
 
// signal
sem_t rwsem;                         // read and write thread communication semaphore
 
/*Atomic operations are operations that are completed without the possibility of interference from other threads. 
This is crucial in multithreaded programming to avoid race conditions.*/
 
atomic_bool g_isLoginSuccess{false}; // record login status           
 
// get system time to add to chat information
string getCurrentTime();
 
// show current user information
void showCurrentUserData();
 
// main menu function
void mainMenu(int);
 
// recieve thread 
void readTaskHandler(int clientfd);
 
// implement chat client
// main thread used for sending, sub thread used for recieving
int main(int argc, char **argv) // argc: number of parameters, argv: parameter list
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl; // cerr: standard error output
        exit(-1);
    }
 
    // parse ip and port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
 
    // create socket for client
    int clientfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET:IPv4  SOCK_STREAM:TCP
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }
 
    // write server ip and port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in)); // memset set to 0
 
    server.sin_family = AF_INET;            // IPv4
    server.sin_port = htons(port);          // port
    server.sin_addr.s_addr = inet_addr(ip); // ip address
 
    // connect client to server
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }
 
    // initial semaphore for read and write thread
    sem_init(&rwsem, 0, 0);
 
    // start sub thread to read
    std::thread readTask(readTaskHandler, clientfd); // pthread_create
    readTask.detach();                               // pthread_detach
 
    // main thread used for recieving user input, send data
    for (;;)
    {
        // show main menu
        cout << "========================" << endl;
        cout << "======  1. login  ======" << endl;
        cout << "======  2. register  ===" << endl;
        cout << "======  3. quit  =======" << endl;
        cout << "========================" << endl;
        cout << "Please input your choice:";
        int choice = 0;
        if (cin >> choice) // read function option
        {
            cin.get(); // read the remaining newline character in the buffer
        }
        else
        {
            cerr << "invalid input!" << endl;
            cin.clear(); // clear error state
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // ignore the remaining characters in the buffer
            continue;
        }
        switch (choice)
        {
        case 1: // login business 
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "user id:";
            cin >> id;
            cin.get(); // read the remaining newline character in the buffer
            cout << "user password:";
            cin.getline(pwd, 50);
 
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();
 
            g_isLoginSuccess = false;
 
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0); // send to server
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }
 
            sem_wait(&rwsem); // wait for semaphore, notify here after the sub thread processes the login response message
 
            if (g_isLoginSuccess)
            {
                // enter chat menu
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: // register busniess
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "user name:";
            cin.getline(name, 50);
            cout << "user password:";
            cin.getline(pwd, 50);
 
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();
 
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0); 
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
 
            sem_wait(&rwsem); // wait for semaphore, notify here after the sub thread processes the registration response message
            
        }
        break;
        case 3: // exit 
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
 
    return 0;
}
 
// show current user information
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user -> id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << endl;
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
            cout << endl;
        }
    }
    cout << "======================================================" << endl;
}
 
// get current time （chat message need current time stamp）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}
 
// process register response logic
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // register failed
    {
        cerr << "name is already exist, register error!" << endl;
    }
    else // register success
    {
        cout << "name register success, userid is " << responsejs["id"]
             << ", do not forget it!" << endl;
    }
}
 
// process login response logic
void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // login failed
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else // login success
    {
        // record current login user information id and name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);
 
        // record current login user's friend list information
        if (responsejs.contains("friends"))
        {
            // initialize
            g_currentUserFriendList.clear();
 
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str); // deserialize
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }
 
        // record current login user's group list information
        if (responsejs.contains("groups"))
        {
            // initialize
            g_currentUserGroupList.clear();
 
            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr); 
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);
 
                vector<string> vec2 = grpjs["users"]; // users in group
                for (string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
 
                g_currentUserGroupList.push_back(group);
            }
        }
 
        
        showCurrentUserData();

        //show offline message of current user  personal chat information or group chat information
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);                 
                if (ONE_CHAT_MSG == js["msgid"].get<int>()) // chat p to p
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
                else // group chat
                {
                    cout << "groupmessage[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }
 
        g_isLoginSuccess = true;
    }
}
 
//sub thread used for recieving
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // block
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }
 
        // recieve data from server and deserialize it
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>(); // get msgid
        if (ONE_CHAT_MSG == msgtype)          // chat p to p
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
 
        if (GROUP_CHAT_MSG == msgtype) // group chat
        {
            cout << "groupmessage[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
 
        if (LOGIN_MSG_ACK == msgtype) // login business
        {
            doLoginResponse(js); // process login response logic
            sem_post(&rwsem);    // notify main thread, login result processed
            continue;
        }
 
        if (REG_MSG_ACK == msgtype) // register business
        {
            doRegResponse(js); //   process register response logic
            sem_post(&rwsem);  //   notify main thread, register result processed
            continue;
        }
    }
}

// client command list
unordered_map<string, string> CommandMap{
    {"help", "show commands, format: help"},
    {"chat", "one-to-one chat, format: chat:friendid:message"},
    {"addfriend", "add a friend, format: addfriend:friendid"},
    {"creategroup", "create a group, format: creategroup:groupname:groupdesc"},
    {"addgroup", "add users into a group, format: addgroup:groupid"},
    {"groupchat", "group chat, format: groupchat:groupid:message"},
    {"logout", "logout, format: logout"},
};


// help command Handler
void help(int fd = 0, string = "");
// chat command Handler
void chat(int, string);
// addfriend command Handler
void addfriend(int, string);
// creategroup command Handler
void creategroup(int, string);
// addgroup command Handler
void addgroup(int, string);
// groupchat command Handler
void groupchat(int, string);
// logout command Handler
void logout(int, string);


// client command Handler
unordered_map<string, function<void(int, string)>> CommandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout},
};

// main menu
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);      // get command
        string commandbuf(buffer);      // convert char * to string
        string command;                 // store command
        int idx = commandbuf.find(":"); // judge whether the command is help or logout
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx); // truncation
        }
        auto it = CommandHandlerMap.find(command);
        if (it == CommandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
 
        // call corresponding event callback，mainMenu is closed for modification，it needs no modification when adding new commands
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); 
    }
}

// help command Handler
void help(int, string)
{
    cout << "--------------show command list-------------" << endl;
    for (auto &p : CommandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// chat command Handler              "chat:friendid:message"
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
 
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
 
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error ->" << buffer << endl;
    }
}
 
// addfriend command Handler         "addfriend:friendid"
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
 
    string buffer = js.dump(); // 序列化发出
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error ->" << buffer << endl;
    }
}
 
// creategroup command Handler       “creategroup:groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);
 
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();
 
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error ->" << buffer << endl;
    }
}
 
// addgroup command Handler              "addgroup:groupid"
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
 
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error ->" << buffer << endl;
    }
}
// groupchat command Handler         "groupchat:groupid:message"
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "groupchat command invalid1" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
 
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
 
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error ->" << buffer << endl;
    }
}
// logout command Handler
void logout(int clientfd, string str){
    json js;
    js["msgid"] =LOGOUT_MSG ;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send logout msg error ->" << buffer << endl;
    }else{
        isMainMenuRunning = false;
    }
}