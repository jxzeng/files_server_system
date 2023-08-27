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
print_log _log;
char MD5_LOG_MODULE[10]= "cgi";
char MD5_LOG_PROC[10] = "md5";

//mysql 数据库配置信息 用户名， 密码， 数据库名称
static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};
static char mysql_ip[128] = {0};
static char mysqlport[128] = {0};
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
//解析秒传信息的json包
int get_md5_info(char *buf, char *user, char *token, char *md5, char *filename)
{
    int ret = 0;    
    cJSON * root = cJSON_Parse(buf);
    if(NULL == root)
    {
        _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_Parse err\n");
        ret = -1;        
        return ret;
    }

    //返回指定字符串对应的json对象
    //用户
    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(NULL == child1)
    {
        _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = NULL;
        return ret;
    }
    
    strcpy(user, child1->valuestring); //拷贝内容
    //MD5
    cJSON *child2 = cJSON_GetObjectItem(root, "md5");
    if(NULL == child2)
    {
        _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = NULL;
        return ret;
    }
    strcpy(md5, child2->valuestring); //拷贝内容
    //文件名字
    cJSON *child3 = cJSON_GetObjectItem(root, "fileName");
    if(NULL == child3)
    {
        _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = NULL;
        return ret;
    }
    strcpy(filename, child3->valuestring); //拷贝内容
    //token
    cJSON *child4 = cJSON_GetObjectItem(root, "token");
    if(NULL == child4)
    {
        _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = NULL;
        return ret;
    }
    strcpy(token, child4->valuestring);
    if(root != NULL)
    {
        cJSON_Delete(root);//删除json对象
        root = NULL;
    }

    return ret;
}

//秒传处理
//返回值：0秒传成功，-1出错，-2此用户已拥有此文件， -3秒传失败
/*
    秒传文件：        
        秒传成功：  {"code":"005"}
        秒传失败：  {"code":"006"}
        文件已存在：{"code":"007"}

*/
int deal_md5(char *user, char *md5, char *filename)
{
    int ret = 0;
    MYSQL *conn = NULL;
    int ret2 = 0;
    char tmp[512] = {0};
    char sql_cmd[512] = {0};
    char *out = NULL;
    //connect the database
    database db;
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == NULL)
    {
        _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;
        out = return_status((char*)"006"); 
        printf(out); //给前端反馈信息
        delete out; 
        mysql_close(conn);
        return ret;
    }
    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");      
    //查看数据库是否有此文件的md5
    //如果没有，返回 {"code":"006"}， 代表不能秒传
    //如果有
    //1、修改file_info中的count字段，+1 （count 文件引用计数）
    //   update file_info set count = 2 where md5 = "bae488ee63cef72efb6a3f1f311b3743";
    //2、user_file_list插入一条数据
    //sql 语句，获取此md5值文件的文件计数器 count
    sprintf(sql_cmd, "select count from file_info where md5 = '%s'", md5);

    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    ret2 = db.process_result_one(conn, sql_cmd, tmp); 
    if(ret2 == 0) //有结果，说明服务器已经有此文件
    {
        int count = atoi(tmp);
        //查看此用户是否已经有此文件，如果存在说明此文件已上传，无需再上传
        sprintf(sql_cmd, "select * from user_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user, md5, filename);

        //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
        ret2 = db.process_result_one(conn, sql_cmd, NULL); 
        if(ret2 == 2) //如果有结果，说明此用户已经保存此文件
        {
            _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s[filename:%s, md5:%s]已存在\n", user, filename, md5);
            ret = -2; //0秒传成功，-1出错，-2此用户已拥有此文件， -3秒传失败
            out = return_status((char*)"007"); 
            printf(out); //给前端反馈信息
            delete out; 
            mysql_close(conn);
            return ret;
        }

        //1、修改file_info中的count字段，+1 （count 文件引用计数）
        sprintf(sql_cmd, "update file_info set count = %d where md5 = '%s'", ++count, md5);
        if(mysql_query(conn, sql_cmd) != 0)
        {
            _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败： %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            out = return_status((char*)"006"); 
            printf(out); //给前端反馈信息
            delete out; 
            mysql_close(conn);
            return ret;
        }

        //2、user_file_list, 用户文件列表插入一条数据
        //当前时间戳
        struct timeval tv;
        struct tm* ptm;
        char time_str[128];

        //使用函数gettimeofday()函数来得到时间。它的精度可以达到微妙
        gettimeofday(&tv, NULL);
        ptm = localtime(&tv.tv_sec);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);        
        sprintf(sql_cmd, "insert into user_file_list(user, md5, createtime, filename, shared_status, pv) values ('%s', '%s', '%s', '%s', %d, %d)", user, md5, time_str, filename, 0, 0);
        if(mysql_query(conn, sql_cmd) != 0)
        {
            _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败： %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            out = return_status((char*)"006"); 
            printf(out); //给前端反馈信息
            delete out; 
            mysql_close(conn);
            return ret;
        }

        //查询用户文件数量
        sprintf(sql_cmd, "select count from user_file_count where user = '%s'", user);
        count = 0;

        //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
        ret2 = db.process_result_one(conn, sql_cmd, tmp); 
        if(ret2 == 1) //没有记录
        {
            //数据库插入此记录
            sprintf(sql_cmd, " insert into user_file_count (user, count) values('%s', %d)", user, 1);
        }
        else if(ret2 == 0)
        {
            //更新用户文件数量count字段
            count = atoi(tmp);
            sprintf(sql_cmd, "update user_file_count set count = %d where user = '%s'", count+1, user);
        }


        if(mysql_query(conn, sql_cmd) != 0)
        {
            _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败： %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            out = return_status((char*)"006"); 
            printf(out); //给前端反馈信息
            delete out; 
            mysql_close(conn);
            return ret;
        }

    }
    else if(1 == ret2)//没有结果，秒传失败
    {
       ret = -3;//0秒传成功，-1出错，-2此用户已拥有此文件， -3秒传失败
       out = return_status((char*)"006"); 
       printf(out); //给前端反馈信息
       delete out; 
       mysql_close(conn);
       return ret;
    }
    //ret的值：0秒传成功，-1出错，-2此用户已拥有此文件， -3秒传失败
    /*
    秒传文件：
        文件已存在：{"code":"007"}
        秒传成功：  {"code":"005"}
        秒传失败：  {"code":"006"}

    */
    //返回前端情况
    if(ret == 0)
    {
        out = return_status((char*)"005"); 
    }
    else if(ret == -2)
    {
        out = return_status((char*)"007"); 
    }
    else
    {
        out = return_status((char*)"006"); 
    }

    if(out != NULL)
    {
        printf(out); //给前端反馈信息
        delete out; 
    }


    if(conn != NULL)
    {
        mysql_close(conn); //断开数据库连接
    }

    return ret;
}

int main()
{
    //读取数据库配置信息
    parse_conf p_conf;
    p_conf.get_mysql_conf(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);

    //阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {
        char *contentLength = getenv("CONTENT_LENGTH");
        int len;
        printf("Content-type: text/html\r\n\r\n");
        if( contentLength == NULL )        {
            len = 0;
        }
        else
        {
            len = atoi(contentLength);
        }
        if (len <= 0)
        {
            printf("No data from standard input.<p>\n");
            _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"len = 0, No data from standard input\n");
        }
        else //获取登陆用户信息
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin);  
            if(ret == 0)
            {
                _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fread(buf, 1, len, stdin) err\n");
                continue;
            }

            _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"buf = %s\n", buf);

            //解析json中信息
            /*
             * {
                    "fileName": "新建文本文档.txt",
                    "md5": "c63204cc106231695480c8a3f847ea54",
                    "token": "30819680cb549ac04443cb7a7e8f6b08",
                    "user": "zeng123"
               }
             */
            char user[128] = {0};
            char md5[256] = {0};
            char token[256] = {0};
            char filename[128] = {0};
            ret = get_md5_info(buf, user, token, md5, filename);
            if(ret != 0)
            {
                _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"get_md5_info() err\n");
                continue;
            }
            _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"user = %s, token = %s, md5 = %s, filename = %s\n", user, token, md5, filename);

            //验证登陆token，成功返回0，失败-1
            ret = verify_token(user, token); 
            if(ret == 0)
            {
                _log.log(MD5_LOG_MODULE, MD5_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"进入妙传处理\n");
                deal_md5(user, md5, filename); //秒传处理
            }
            else
            {
                char *out = return_status((char*)"111"); //token验证失败错误码
                if(out != NULL)
                {
                    printf(out); //给前端反馈错误码
                    delete out;
                }
            }
        }

    }

    return 0;
}