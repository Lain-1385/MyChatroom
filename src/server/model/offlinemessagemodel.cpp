#include "offlinemessagemodel.hpp"
#include "db.hpp"
 
// store user's offline message
void offlineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());
 
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
 
// delete user's offline message
void offlineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);
 
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
 
// query user's offline message
vector<string> offlineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);
 
    vector<string> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // put all offline message into vector and return
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}