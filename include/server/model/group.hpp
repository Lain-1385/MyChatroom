#ifndef GROUP_H
#define GROUP_H
 
#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;
 
// ORM class for group table
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }
 
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }
 
    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    vector<GroupUser> &getUsers() { return this->users; }
 
private:
    int id;                  // group id
    string name;             // group name
    string desc;             // group description
    vector<GroupUser> users; // group user list
};
 
#endif