#ifndef DB_H
#define DB_H
 
#include <mysql/mysql.h>//mysql的头文件 
#include <string>
using namespace std;
 
//class for database operation
class MySQL
{
public:
    //initialize database connection
    MySQL();
    //release database connection resource
    ~MySQL();
    //connect to database
    bool connect();
    //update operation
    bool update(string sql);
    // query operation
    MYSQL_RES *query(string sql);
    //get connection information
    MYSQL* getConnection();
private:
    MYSQL *_conn;
};
 
#endif