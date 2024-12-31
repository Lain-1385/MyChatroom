#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis 
{
    public:
        Redis();
        ~Redis();
        //connect to redis server
        bool connect();

        //publish messgae to redis channel
        bool publish(int channel, string message);

        //subscribe to redis channel
        bool subscribe(int channel);

        //unsubscribe to redis channel
        bool unsubscribe(int channel);

        //get message from redis server in a independent thread
        void observer_channel_message();

        //init callback handler for notification 
        void init_notify_handler(function<void(int, string)> fn);
    private:
        //redis context for publish
        redisContext *_publish_context;
        //redis context for subscribe
        redisContext *_subscribe_context;

        //callback function for notification 
        function<void(int, string)> _notify_message_handler;
};
