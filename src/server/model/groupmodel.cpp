#include "groupmodel.hpp"
#include "db.hpp"
#include <vector>
 
// create group
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    // insert into allgroup table
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str()); 
 
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // set the id of group as the id of mysql generated
            group.setId(mysql_insert_id(mysql.getConnection())); 
            return true;
        }
    }
 
    return false;
}
 
// add user to group
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')", groupid, userid, role.c_str());
 
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        return true;
    }
    return false;
}

// query user's group information 
vector<Group> GroupModel::queryGroups(int userid)
{
    /*
    1. query all groups that the user belongs to
    2. according to the groupid, query all users in each group 
    */
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
         groupuser b on a.id = b.groupid where b.userid=%d",
            userid); // joint query, get all group information that the user belongs to
 
    vector<Group> groupVec; // vector to store group information
 
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            // get all group information that the user belongs to
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
 
    // query user information in each group
    for (Group &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a \
            inner join groupuser b on b.userid = a.id where b.groupid=%d",
                group.getId()); // joint query, get all user information in the each group
 
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user); // get group user vector object
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
 
// all group users except the current user in the certain group
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid); // get all group users except the current user 
 
    vector<int> idVec; // define a vector to store user id
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}