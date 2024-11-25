#ifndef USERMODEL_H
#define USERMODEL_H
 
#include "user.hpp"
 
//class for user table operation
class UserModel {
public:
    //insert user table
    bool insert(User &user);
 
    //query user information by id
    User query(int id);
 
    //update user state information
    bool updateState(User user);
 
    //resest user state information
    void resetState();
};
 
#endif