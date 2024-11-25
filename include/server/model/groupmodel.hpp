#ifndef GROUPMODEL_H
#define GROUPMODEL_H
 
#include "group.hpp"
#include <string>
#include <vector>
using namespace std;
 
//public funciton for managing group information
class GroupModel
{
public:
    // create group
    bool createGroup(Group &group);
    // add user to group
    bool addGroup(int userid, int groupid, string role);
    //query user's group information
    vector<Group> queryGroups(int userid);
    //query other group users 
    vector<int> queryGroupUsers(int userid, int groupid);
};
 
#endif