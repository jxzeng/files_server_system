//#pragma once
#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include "cJSON.h"
#include "print_log.h"
#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>
#include<parse_conf.h>
#include "database.h"
#include<sys/time.h>
#include "des.h"
#include "base64.h"
#include "md5.h"
using namespace std;
char MYFILES_LOG_MODULE[10] = "cgi";
char MYFILES_LOG_PROC[10] = "myfiles";
//mysql 数据库配置信息 用户名， 密码， 数据库名称
char mysql_user[128] = {0};
char mysql_pwd[128] = {0};
char mysql_db[128] = {0};
char mysql_ip[128] = {0};
char mysqlport[128] = {0};
print_log _log;
//解析的json包, 登陆token
int get_count_json_info(char *buf, char *user, char *token)
{
    int ret = 0;
    /*json数据如下
    {
        "token": "9e894efc0b2a898a82765d0a7f2c94cb",
        user:xxxx
    }
    */
    //解析json包
    //解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_Parse err\n");
        ret = -1;
        return ret;
    }
    //返回指定字符串对应的json对象
    //用户
    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(child1 == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);
        root = nullptr;
        return ret;
    }        
    //登陆token
    cJSON *child2 = cJSON_GetObjectItem(root, "token");
    if(child2 == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*) "cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);
        root = nullptr;
        return ret;
    }
    //拷贝内容
    strcpy(user, child1->valuestring);
    strcpy(token, child2->valuestring); 
    cJSON_Delete(root);
    root = nullptr;
    return ret;
}

//验证登陆token，成功返回0，失败-1
int verify_token(char *user, char *token)
{
    int ret = 0;
    redisContext* redis_conn = nullptr;
    char tmp_token[128] = {0};
    parse_conf p_conf;
    database db;
    //redis 服务器ip、端口
    char redis_ip[30] = {0};
    char redis_port[10] = {0};
    //读取redis配置信息
    p_conf.get_redis_conf(redis_ip,redis_port);    

    //连接redis数据库
    redis_conn = db.redis_connect(redis_ip, redis_port);
    if (redis_conn == nullptr)
    {
        _log.log(db.REDIS_LOG_MODULE, db.REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"redis connected error\n");
        ret = -1;
        return ret;
    }
    //获取user对应的value
    ret = db.get_string(redis_conn, user, tmp_token);
    if(ret == 0)
    {
        if( strcmp(token, tmp_token) != 0 ) //token不相等
        {
            ret = -1;
        }
    }
    redisFree(redis_conn);
    redis_conn = nullptr;
    return ret;
}

//返回前端情况
/*
    return: token = "110"; //成功 --->{"code":"110" , "num":"num" }
            token = "111"; //失败 --->{"code":"111" , "num":"0" }
*/
void return_login_status(long num, int token_flag)
{
    char *out = nullptr;
    char *token;
    char num_buf[128] = {0};
    sprintf(num_buf, "%ld", num);
    cJSON *root = cJSON_CreateObject();
    
    if(token_flag == 0)
    {
        token = (char*)"110"; //成功
        cJSON_AddStringToObject(root, "num", num_buf);
    }
    else
    {
        token = (char*)"111"; //失败
        cJSON_AddStringToObject(root, "num", "0");
    }
    //token
    cJSON_AddStringToObject(root, "code", token);// {"code":"110"}     
    out = cJSON_Print(root);
    cJSON_Delete(root);
    if(out != nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"out = %s\n",out);
        printf(out); //给前端反馈信息
        delete(out); 
    }
}

//获取用户文件个数
void get_user_files_count(char *user, int ret)
{
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    long line = 0;
    database db;    
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");        
        //给前端反馈的信息
        return_login_status(line, ret);
        return;        
    }
    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    sprintf(sql_cmd, (char*)"select count from user_file_count where user=\"%s\"", user);
    char tmp[512] = {0};
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    int ret2 = db.process_result_one(conn, sql_cmd, tmp); 
    if(ret2 != 0)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC,  (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
        mysql_close(conn);
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC,  (char*)__FILE__, __LINE__,(char*)"line = %ld\n", line);
        //给前端反馈的信息
        return_login_status(line, ret);
        return;
    }

    line = atol(tmp);
    if(conn != nullptr){
        mysql_close(conn);
    }     
    _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC,  (char*)__FILE__, __LINE__,(char*)"line = %ld\n", line);
    //给前端反馈的信息
    return_login_status(line, ret);
}

//解析其他命令的json包
int get_fileslist_json_info(char *buf, char *user, char *token, int *p_start, int *p_count)
{
    int ret = 0;
    /*json数据如下
    {
        "user": "yoyo"       用户名
        "token": xxxx        token
        "start": 0           开始文件号
        "count": 10          一次获取数
    }
    */
    //解析json包    
    cJSON * root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_Parse err\n");
        ret = -1;
        return ret;
    }

    //返回指定字符串对应的json对象
    //用户
    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(child1 == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = nullptr;
        return ret;
    }
    
    //token
    cJSON *child2 = cJSON_GetObjectItem(root, "token");
    if(child2 == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = nullptr;
        return ret;
    }    

    //文件起点
    cJSON *child3 = cJSON_GetObjectItem(root, "start");
    if(child3 == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = nullptr;
        return ret;
    }    

    //文件请求个数
    cJSON *child4 = cJSON_GetObjectItem(root, "count");
    if(child4 == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = nullptr;
        return ret;
    }

    //拷贝内容
    strcpy(user, child1->valuestring); 
    strcpy(token, child2->valuestring); 
    *p_start = child3->valueint;
    *p_count = child4->valueint;
    cJSON_Delete(root);//删除json对象
    root = nullptr;
    return ret;
}

//返回前端情况
char * return_status(char *status_num)
{
    char *out = nullptr;
    cJSON *root = cJSON_CreateObject();  
    cJSON_AddStringToObject(root, "code", status_num);// {"code":"000"}
    out = cJSON_Print(root);
    cJSON_Delete(root);
    return out;
}

//获取用户文件列表
int get_user_filelist(char *cmd, char *user, int start, int count)
{
    int ret = 0;
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    cJSON *root = nullptr;
    cJSON *array =nullptr;
    char *out = nullptr;
    char *out2 = nullptr;
    MYSQL_RES *res_set = nullptr;

    //connect the database
    database db;
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;
        out = return_status((char*)"015");
        if(out != nullptr){
            printf(out);
            delete(out);
        }
        return ret;
    }
    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    
    if(strcmp(cmd, "normal") == 0) //获取用户文件信息
    {
        //sql语句
        sprintf(sql_cmd, "select user_file_list.*, file_info.url, file_info.size, file_info.type from file_info, user_file_list where user = '%s' and file_info.md5 = user_file_list.md5 limit %d, %d", user, start, count);
    }
    else if(strcmp(cmd, "pvasc") == 0) //按下载量升序
    {
        //sql语句
        sprintf(sql_cmd, "select user_file_list.*, file_info.url, file_info.size, file_info.type from file_info, user_file_list where user = '%s' and file_info.md5 = user_file_list.md5  order by pv asc limit %d, %d", user, start, count);
    }
    else if(strcmp(cmd, "pvdesc") == 0) //按下载量降序
    {
        //sql语句
        sprintf(sql_cmd, "select user_file_list.*, file_info.url, file_info.size, file_info.type from file_info, user_file_list where user = '%s' and file_info.md5 = user_file_list.md5 order by pv desc limit %d, %d", user, start, count);
    }
    _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC,  (char*)__FILE__, __LINE__,(char*)"%s 在操作\n", sql_cmd);

    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败：%s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out = return_status((char*)"015");
        if(out != nullptr){
            printf(out);
            delete(out);
        }
        mysql_close(conn);
        return ret;
    }
    res_set = mysql_store_result(conn);/*生成结果集*/
    if (res_set == nullptr)
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"smysql_store_result error: %s!\n", mysql_error(conn));
        ret = -1;
        out = return_status((char*)"015");
        if(out != nullptr){
            printf(out);
            delete(out);
        }
        mysql_close(conn);
        return ret;
    }
    ulong line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)//没有结果
    {
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql_num_rows(res_set) failed：%s\n", mysql_error(conn));
        ret = -1;
        out = return_status((char*)"015");
        if(out != nullptr){
            printf(out);
            delete(out);
        }
        mysql_free_result(res_set);
        mysql_close(conn);        
        return ret;
    }
    MYSQL_ROW row;
    root = cJSON_CreateObject();
    array = cJSON_CreateArray();
    // mysql_fetch_row从使用mysql_store_result得到的结果结构中提取一行，并把它放到一个行结构中。
    // 当数据用完或发生错误时返回NULL.
    while ((row = mysql_fetch_row(res_set)) != nullptr)
    {
        cJSON* item = cJSON_CreateObject();        
        /*
        {
        "user": "yoyo",
        "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
        "time": "2023-08-20 21:35:25",
        "filename": "test.mp4",
        "share_status": 0,
        "pv": 0,
        "url": "http://192.168.31.109:80/group1/M00/00/00/wKgfbViy2Z2AJ-FTAaM3As-g3Z0782.mp4",
        "size": 27473666,
         "type": "mp4"
        }

        */
        //-- user	文件所属用户
        if(row[0] != nullptr)
        {
            cJSON_AddStringToObject(item, "user", row[0]);
        }

        //-- md5 文件md5
        if(row[1] != nullptr)
        {
            cJSON_AddStringToObject(item, "md5", row[1]);
        }

        //-- createtime 文件创建时间
        if(row[2] != nullptr)
        {
            cJSON_AddStringToObject(item, "time", row[2]);
        }

        //-- filename 文件名字
        if(row[3] != nullptr)
        {
            cJSON_AddStringToObject(item, "filename", row[3]);
        }

        //-- shared_status 共享状态, 0为没有共享， 1为共享
        if(row[4] != nullptr)
        {
            cJSON_AddNumberToObject(item, "share_status", atoi( row[4] ));
        }

        //-- pv 文件下载量，默认值为0，下载一次加1
        if(row[5] != nullptr)
        {
            cJSON_AddNumberToObject(item, "pv", atol( row[5] ));
        }

        //-- url 文件url
        if(row[6] != nullptr)
        {
            cJSON_AddStringToObject(item, "url", row[6]);
        }

        //-- size 文件大小, 以字节为单位
        if(row[7] != nullptr)
        {
            cJSON_AddNumberToObject(item, "size", atol( row[7] ));
        }

        //-- type 文件类型： png, zip, mp4……
        if(row[8] != nullptr)
        {
            cJSON_AddStringToObject(item, "type", row[8]);
        }

        cJSON_AddItemToArray(array, item);
    }
    cJSON_AddItemToObject(root, "files", array);
    out2 = cJSON_Print(root);
    _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s\n", out2);
    if(ret == 0)
    {
        printf("%s", out2); //给前端反馈信息
    }
    else
    {   //失败
        /*
        获取用户文件列表：
            成功：文件列表json
            失败：{"code": "015"}
        */
        out = nullptr;
        out = return_status((char*)"015");
    }
    if(out != nullptr)
    {
        printf(out); //给前端反馈错误码
        free(out);
    }

    if(res_set != nullptr)
    {
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);
    }
    if(conn != nullptr)
    {
        mysql_close(conn);
    }
    if(root != nullptr)
    {
        cJSON_Delete(root);
    }
    if(out != nullptr)
    {
        free(out);
    }
    return ret;
}


int main(){
    //count 获取用户文件个数
    //display 获取用户文件信息，展示到客户端
    char cmd[20];
    char user[128];
    char token[256];
    //读取数据库配置信息    
    parse_conf p_conf;
    int ret = p_conf.get_mysql_conf(mysql_user,mysql_pwd,mysql_db,mysql_ip,mysqlport);    
    _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql:[user=%s,pwd=%s,database=%s,ip=%s,port=%s]",
                             mysql_user, mysql_pwd, mysql_db,mysql_ip,mysqlport);
    //阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {

        // 获取URL地址 "?" 后面的内容  url:127.0.0.1:80/myfiles?cmd=***
        char *query = getenv("QUERY_STRING");
        //解析命令
        p_conf.query_parse_key_value(query, (char*)"cmd", cmd, nullptr);
        _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cmd = %s\n", cmd);

        char* content_Length = getenv("CONTENT_LENGTH");
        int len;
        printf("Content-type: text/html\r\n\r\n");
        if( content_Length == nullptr )
        {
            len = 0;
        }
        else
        {
            len = atoi(content_Length); 
        }
        if (len <= 0)
        {
            printf("No data from standard input.<p>\n");
            _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"len = 0, No data from standard input\n");
        }
        else
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); 
            if(ret == 0)
            {
                _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fread(buf, 1, len, stdin) err\n");
                continue;
            }
            _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"buf = %s\n", buf);
            //url:127.0.0.1:80/myfiles?cmd=count
            if (strcmp(cmd, "count") == 0) //count 获取用户文件个数
            {
                get_count_json_info(buf, user, token); //通过json包获取用户名, token
                //验证登陆token，成功返回0，失败-1
                ret = verify_token(user, token);                 
                get_user_files_count(user, ret); //获取用户文件个数

            }
            //获取用户文件信息 127.0.0.1:80/myfiles&cmd=normal
            //按下载量升序 127.0.0.1:80/myfiles?cmd=pvasc
            //按下载量降序127.0.0.1:80/myfiles?cmd=pvdesc
            else
            {
                int start; //文件起点
                int count; //文件个数
                get_fileslist_json_info(buf, user, token, &start, &count); //通过json包获取信息
                _log.log(MYFILES_LOG_MODULE, MYFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"user = %s, token = %s, start = %d, count = %d\n", user, token, start, count);

                //验证登陆token，成功返回0，失败-1
                ret = verify_token(user, token);
                if(ret == 0)
                {
                    get_user_filelist(cmd, user, start, count); //获取用户文件列表
                }
                else
                {
                    char *out = return_status((char*)"111"); //token验证失败错误码
                    if(out != nullptr)
                    {
                        printf(out); //给前端反馈错误码
                        delete(out);
                    }
                }

            }

        }

    }
    return 0;
}