#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
 
#include "user.hpp"
#include <vector>
using namespace std;
 
// interface class for friend model
class FriendModel
{
public:
    // add friend relationship
    bool insert(int userid, int friendid);
 
    // return user friend list
    vector<User> query(int userid);
};
 
#endif