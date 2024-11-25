#include "friendmodel.hpp"
#include "db.hpp"
 
// add friend relationship
bool FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);    // insert into friend table
 
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        return true;
    }
    return false;
}
 
// return user friend list
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    
    //multiple table joint query
    sprintf(sql, "select a.id,a.name,a.state from user a \
    inner join friend b on b.friendid = a.id where b.userid=%d", userid);   
 
    //define a vector to store userid friend information
    vector<User> Friendvec;             
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                Friendvec.push_back(user);
            }
            mysql_free_result(res);
            return Friendvec;
        }
    }
    return Friendvec;
}