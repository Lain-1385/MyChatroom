#ifndef PUBLIC_H
#define PUBLIC_H
 

enum EnMsgType
{
    LOGIN_MSG = 1, //
    LOGIN_MSG_ACK, //login message ack
    REG_MSG, //
    REG_MSG_ACK, //register message ack
    ONE_CHAT_MSG,   //chat message
    ADD_FRIEND_MSG, 
    ADD_FRIEND_MSG_ACK,
    CREATE_GROUP_MSG, // 
    CREATE_GROUP_MSG_ACK, // 
    ADD_GROUP_MSG, // 
    ADD_GROUP_MSG_ACK, // 
    GROUP_CHAT_MSG, //
    LOGOUT_MSG, // 
};
 
#endif