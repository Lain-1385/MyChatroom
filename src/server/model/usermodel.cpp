#include "usermodel.hpp"
#include "db.hpp"
#include <iostream>
using namespace std;
 
// insert user table information
bool UserModel::insert(User &user)
{
    // 1. write sql statement 
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
 
    MySQL mysql;         // define a mysql object
    if (mysql.connect()) // connect successfully
    {
        if (mysql.update(sql)) // update this sql statement
        {
            // get the id of mysql generated
            // set it as the id of user
            user.setId(mysql_insert_id(mysql.getConnection())); 
            return true;
        }
    }
 
    return false;
}

User UserModel::query(int id)
{
    // 1. write sql statement
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);
 
    MySQL mysql;         // define a mysql object
    if (mysql.connect()) // connect successfully
    {
        MYSQL_RES *res = mysql.query(sql); // query this sql statement
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
 
    return User();
}

bool UserModel::updateState(User user)
{
    // 1. write sql statement
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",
            user.getState().c_str(), user.getId());
 
    MySQL mysql;         // define a mysql object
    if (mysql.connect()) // connect successfully
    {
        if (mysql.update(sql)) // update this sql statement
        {
            return true;
        }
    }
 
    return false;
}

void UserModel::resetState()
{
    // 1. write sql statement
    char sql[1024] = "update user set state = 'offline' where state = 'online'";
 
    MySQL mysql;         
    if (mysql.connect()) 
    {
        mysql.update(sql); 
    }
}