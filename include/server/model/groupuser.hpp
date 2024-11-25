#ifndef GROUPUSER_H
#define GROUPUSER_H
 
#include "user.hpp"

//group user, with an additional role information, directly inherited from the User class, reusing other information of User
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }
 
private:
    string role;            //role in the group, creator or normal
};
 
#endif