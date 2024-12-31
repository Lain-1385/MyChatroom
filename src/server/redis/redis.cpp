// synchronization
// redis channel equals id, every online user has a unique redis channel
#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis(): _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subscribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    //start a new thread to receive message, and notify the chatservice
    thread t([&](){
        observer_channel_message();
    });

    t.detach();

    cout << "connect redis server success!" << endl;

    return true;
}

bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply*)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish message failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

//SUBSCRIBE command will cause thread block, so here we just subscribe not receive message
//here we avoid thread blocking
bool Redis::subscribe(int channel)
{
    //redisAppendCommand: command is cached in buffer locally
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferwrite can send buffer looply until buffer is empty(done is set to 1)
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    cerr << "subscribe channel" << channel << " success!" << endl;
    // redisGetReply will bock thread
    return true;
}

bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    //loop, block thread, that's why we need a new thread
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void**)&reply))
    {
        //print reply->element length
        /*
        if (reply != nullptr)
        {
            cout << "observer_channel_message: " << reply->elements << endl;
        }
        error: elements is 0
        */
        //handle message
        //element[2] is message, element[1] is channel

        if (reply != nullptr && reply->elements != 0 && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            //notify message handler
            
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << "observer_channel_message quit" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}
