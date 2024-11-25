#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
 
 
#include <string>
#include <vector>
using namespace std;
 
//provide interface for offline message table operation
class offlineMsgModel{
    public:
        //store offline message of user
        void insert(int userid,string msg);
 
        //delete offline message of user
        void remove(int userid);
 
        //query offline message of user
        vector<string> query(int userid);
};
 
#endif