#include "db.hpp"
#include <muduo/base/Logging.h>
 
//configure information of database
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";
 
//initialize database connection
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
 
//release database connection resource
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
 
//connect to database
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        //C and C++ will use ASCII by default
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
    }
 
    return p;
}
 
//upadaet operation
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "update failed!";
        return false;
    }
 
    return true;
}
 
//query operation
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "query failed!";
        return nullptr;
    }
    
    return mysql_use_result(_conn);
}
 
//get connection information
MYSQL* MySQL::getConnection()
{
    return _conn;
}