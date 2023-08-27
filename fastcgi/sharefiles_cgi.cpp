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
char SHAREFILES_LOG_MODULE[10] = "cgi";
char SHAREFILES_LOG_PROC[15] = "sharefiles";
//mysql 数据库配置信息 用户名， 密码， 数据库名称
char mysql_user[128] = {0};
char mysql_pwd[128] = {0};
char mysql_db[128] = {0};
char mysql_ip[128] = {0};
char mysqlport[128] = {0};
//redis 服务器ip、端口
char redis_ip[30] = {0};
char redis_port[10] = {0};

//解析的json包
int get_fileslist_json_info(char *buf, int *p_start, int *p_count)
{
    int ret = 0;
    /*json数据如下
    {
        "start": 0
        "count": 10
    }
    */
    //解析json包    
    cJSON * root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_Parse err\n");
        ret = -1;
        return ret;
    }
    //文件起点
    cJSON *child2 = cJSON_GetObjectItem(root, "start");
    if(child2 == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = NULL;
        return ret;
    }   

    //文件请求个数
    cJSON *child3 = cJSON_GetObjectItem(root, "count");
    if(child3 == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = NULL;
        return ret;
    }
    //拷贝内容
    *p_start = child2->valueint;
    *p_count = child3->valueint;

    if(root != NULL)
    {
        cJSON_Delete(root);//删除json对象
        root = NULL;
    }
    return ret;
}

//获取共享文件个数
void get_share_files_count()
{
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    database db;
    long line = 0;
    //connect the database
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"line = %ld\n", line);
        printf("%ld", line); //给前端反馈的信息
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    sprintf(sql_cmd, "select count from user_file_count where user=\"%s\"", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    char tmp[512] = {0};
    int ret2 = db.process_result_one(conn, sql_cmd, tmp); //指向sql语句
    if(ret2 != 0)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"line = %ld\n", line);
        printf("%ld", line); //给前端反馈的信息
    }
    line = atol(tmp); //字符串转长整形
    if(conn != NULL)
    {
        mysql_close(conn);
    }
    _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"line = %ld\n", line);
    printf("%ld", line); //给前端反馈的信息
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
//获取共享文件列表
//获取用户文件信息 127.0.0.1:80/sharefiles&cmd=normal
int get_share_filelist(int start, int count)
{
    int ret = 0;
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    cJSON *root = nullptr;
    cJSON *array =nullptr;
    char *out = nullptr;
    char *out2 = nullptr;
    MYSQL_RES *res_set = nullptr;
    database db;
    //connect the database
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        return ret;
    }
    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    //sql语句
    sprintf(sql_cmd, "select share_file_list.*, file_info.url, file_info.size, file_info.type from file_info, share_file_list where file_info.md5 = share_file_list.md5 limit %d, %d", start, count);

    _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 在操作\n", sql_cmd);
    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        return ret;
    }
    res_set = mysql_store_result(conn);/*生成结果集*/
    if (res_set == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"smysql_store_result error!\n");
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        mysql_free_result(res_set);
        mysql_close(conn);
        return ret;
    }
    ulong line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql_num_rows(res_set) failed\n");
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
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
         //array[i]:
        cJSON* item = cJSON_CreateObject();
        /*
        {
        "user": "yoyo",
        "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
        "time": "2023-08-20 21:35:25",
        "filename": "test.mp4",
        "share_status": 1,
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
        cJSON_AddNumberToObject(item, "share_status", 1);


        //-- pv 文件下载量，默认值为0，下载一次加1
        if(row[4] != nullptr)
        {
            cJSON_AddNumberToObject(item, "pv", atol( row[4] ));
        }

        //-- url 文件url
        if(row[5] != nullptr)
        {
            cJSON_AddStringToObject(item, "url", row[5]);
        }

        //-- size 文件大小, 以字节为单位
        if(row[6] != nullptr)
        {
            cJSON_AddNumberToObject(item, "size", atol( row[6] ));
        }

        //-- type 文件类型： png, zip, mp4……
        if(row[7] != nullptr)
        {
            cJSON_AddStringToObject(item, "type", row[7]);
        }

        cJSON_AddItemToArray(array, item);
    }
    cJSON_AddItemToObject(root, "files", array);
    out = cJSON_Print(root);
    _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s\n", out);
    if(ret == 0)
    {
        printf("%s", out); //给前端反馈信息
    }
    else
    {   //失败
        out2 = nullptr;
        out2 = return_status((char*)"015");
    }
    if(out2 != nullptr)
    {
        printf(out2); //给前端反馈错误码
        delete out2;
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
        delete out;
    }
    return ret;
}

//获取共享文件排行版
//按下载量降序127.0.0.1:80/sharefiles?cmd=pvdesc
int get_ranking_filelist(int start, int count)
{
    /*
    a) mysql共享文件数量和redis共享文件数量对比，判断是否相等
    b) 如果不相等，清空redis数据，从mysql中导入数据到redis (mysql和redis交互)
    c) 从redis读取数据，给前端反馈相应信息
    */
    int ret = 0;
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    cJSON *root = nullptr;
    RVALUES value = nullptr;
    cJSON *array =nullptr;
    char *out = nullptr;
    char *out2 = nullptr;
    char tmp[512] = {0};
    int ret2 = 0;
    MYSQL_RES *res_set = nullptr;
    redisContext * redis_conn = nullptr;
    database db;
    //连接redis数据库
    redis_conn = db.redis_connect(redis_ip, redis_port);
    if (redis_conn == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"redis connected error");
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        return ret;
    }
    //connect the database
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        return ret;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    //===1、mysql共享文件数量
    sprintf(sql_cmd, "select count from user_file_count where user=\"%s\"", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    ret2 = db.process_result_one(conn, sql_cmd, tmp); //指向sql语句
    if(ret2 != 0)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        mysql_close(conn);
        redisFree(redis_conn);
        return ret;
    }
    int sql_num = atoi(tmp); //字符串转长整形
    //===2、redis共享文件数量
    int redis_num = db.get_zcard(redis_conn, (char*)"FILE_PUBLIC_ZSET");
    if(redis_num == -1)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"rop_zset_zcard 操作失败\n");
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        mysql_close(conn);
        redisFree(redis_conn);
        return ret;
    }
    _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"sql_num = %d, redis_num = %d\n", sql_num, redis_num);

    //===3、mysql共享文件数量和redis共享文件数量对比，判断是否相等
    if(redis_num != sql_num)
    {
        //===4、如果不相等，清空redis数据，重新从mysql中导入数据到redis (mysql和redis交互)
        //a) 清空redis有序数据
        db.del_key(redis_conn, (char*)"FILE_PUBLIC_ZSET");
        db.del_key(redis_conn, (char*)"FILE_NAME_HASH");
        //b) 从mysql中导入数据到redis
        //sql语句
        strcpy(sql_cmd, "select md5, filename, pv from share_file_list order by pv desc");

        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 在操作\n", sql_cmd);
        if (mysql_query(conn, sql_cmd) != 0)
        {
            _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            out2 = nullptr;
            out2 = return_status((char*)"015");
            if(out2 != nullptr){
                printf(out2);
                delete out2;
            }
            mysql_close(conn);
            redisFree(redis_conn);
            return ret;
        }
        res_set = mysql_store_result(conn);/*生成结果集*/
        if (res_set == nullptr)
        {
            _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"smysql_store_result error!\n");
            ret = -1;
            out2 = nullptr;
            out2 = return_status((char*)"015");
            if(out2 != nullptr){
                printf(out2);
                delete out2;
            }
            mysql_close(conn);
            redisFree(redis_conn);
            return ret;
        }
        ulong line = 0;
        //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
        line = mysql_num_rows(res_set);
        if (line == 0)
        {
            _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql_num_rows(res_set) failed\n");
            ret = -1;
            out2 = nullptr;
            out2 = return_status((char*)"015");
            if(out2 != nullptr){
                printf(out2);
                delete out2;
            }
            mysql_free_result(res_set);
            mysql_close(conn);
            redisFree(redis_conn);
            return ret;
        }
        MYSQL_ROW row;
        // mysql_fetch_row从使用mysql_store_result得到的结果结构中提取一行，并把它放到一个行结构中。        
        while ((row = mysql_fetch_row(res_set)) != nullptr)
        {
            //md5, filename, pv
            if(row[0] == nullptr || row[1] == nullptr || row[2] == nullptr)
            {
                _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql_fetch_row(res_set)) failed\n");
                ret = -1;
                out2 = nullptr;
                out2 = return_status((char*)"015");
                if(out2 != nullptr){
                    printf(out2);
                    delete out2;
                }
                mysql_free_result(res_set);
                mysql_close(conn);
                redisFree(redis_conn);
                return ret;
            }
            char fileid[1024] = {0};
            sprintf(fileid, "%s%s", row[0], row[1]); //文件标示，md5+文件名

            //增加有序集合成员
            db.zset_add(redis_conn, (char*)"FILE_PUBLIC_ZSET", atoi(row[2]), fileid);
            //增加hash记录
            db.hash_set(redis_conn, (char*)"FILE_NAME_HASH", fileid, row[1]);
        }
    }

    //===5、从redis读取数据，给前端反馈相应信息
    value  = (RVALUES)calloc(count, 1024); //堆区请求空间
    if(value == nullptr)
    {
        ret = -1;
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        mysql_close(conn);
        redisFree(redis_conn);
        return ret;
    }

    int n = 0;
    int end = start + count - 1;//加载资源的结束位置
    //降序获取有序集合的元素
    ret = db.zset_zrevrange(redis_conn, (char*)"FILE_PUBLIC_ZSET", start, end, value, &n);
    if(ret != 0)
    {
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"rop_zset_zrevrange 操作失败\n");
        out2 = nullptr;
        out2 = return_status((char*)"015");
        if(out2 != nullptr){
            printf(out2);
            delete out2;
        }
        mysql_close(conn);
        redisFree(redis_conn);
        return ret;
    }

    root = cJSON_CreateObject();
    array = cJSON_CreateArray();
    //遍历元素个数
    for(int i = 0; i < n; ++i)
    {        
        cJSON* item = cJSON_CreateObject();
        /*
        {
            "filename": "test.mp4",
            "pv": 0
        }
        */
        //-- filename 文件名字
        char filename[1024] = {0};
        ret = db.hash_get(redis_conn, (char*)"FILE_NAME_HASH", value[i], filename);
        if(ret != 0)
        {
            _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"rop_hash_get 操作失败\n");
            ret = -1;
            out2 = nullptr;
            out2 = return_status((char*)"015");
            if(out2 != nullptr){
                printf(out2);
                delete out2;
            }
            mysql_close(conn);
            redisFree(redis_conn);
            return ret;
        }
        cJSON_AddStringToObject(item, "filename", filename);
        //-- pv 文件下载量
        int score = db.zset_get_score(redis_conn, (char*)"FILE_PUBLIC_ZSET", value[i]);
        if(score == -1)
        {
            _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"rop_zset_get_score 操作失败\n");
            ret = -1;
            out2 = nullptr;
            out2 = return_status((char*)"015");
            if(out2 != nullptr){
                printf(out2);
                delete out2;
            }
            mysql_close(conn);
            redisFree(redis_conn);
            return ret;
        }
        cJSON_AddNumberToObject(item, "pv", score);
        cJSON_AddItemToArray(array, item);
    }
    cJSON_AddItemToObject(root, "files", array);
    out = cJSON_Print(root);
    _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s\n", out);
    if(ret == 0)
    {
        printf("%s", out); //给前端反馈信息
    }
    else
    {   //失败
        out2 = NULL;
        out2 = return_status((char*)"015");
    }
    if(out2 != NULL)
    {
        printf(out2); //给前端反馈错误码
        free(out2);
    }

    if(res_set != NULL)
    {
        //完成所有对数据的操作后，调用mysql_free_result来善后处理
        mysql_free_result(res_set);
    }

    if(redis_conn != NULL)
    {
        redisFree(redis_conn);
    }

    if(conn != NULL)
    {
        mysql_close(conn);
    }

    if(value != NULL)
    {
        free(value);
    }

    if(root != NULL)
    {
        cJSON_Delete(root);
    }

    if(out != NULL)
    {
        free(out);
    }
    return ret;
}

int main()
{
    char cmd[20];
    //读取数据库配置信息
    parse_conf p_conf;
    int ret = p_conf.get_mysql_conf(mysql_user,mysql_pwd,mysql_db,mysql_ip,mysqlport);
    ret = p_conf.get_redis_conf(redis_ip,redis_port);

    //阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {
        // 获取URL地址 "?" 后面的内容
        char *query = getenv("QUERY_STRING");
        //解析命令
        p_conf.query_parse_key_value(query, (char*)"cmd", cmd, nullptr);
        _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cmd = %s\n", cmd);

        printf("Content-type: text/html\r\n\r\n");
        if (strcmp(cmd, "count") == 0) //count 获取用户文件个数
        {
            get_share_files_count(); //获取共享文件个数
        }
        else
        {
            char *content_Length = getenv("CONTENT_LENGTH");
            int len;
            if( content_Length == nullptr )
            {
                len = 0;
            }
            else
            {
                len = atoi(content_Length); //字符串转整型
            }

            if (len <= 0)
            {
                printf("No data from standard input.<p>\n");
                _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"len = 0, No data from standard input\n");
            }
            else
            {
                char buf[4*1024] = {0};
                int ret = 0;
                ret = fread(buf, 1, len, stdin); //从标准输入(web服务器)读取内容
                if(ret == 0)
                {
                    _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fread(buf, 1, len, stdin) err\n");
                    continue;
                }
                _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"buf = %s\n", buf);

                //获取共享文件信息 127.0.0.1:80/sharefiles&cmd=normal
                //按下载量升序 127.0.0.1:80/sharefiles?cmd=pvasc
                //按下载量降序127.0.0.1:80/sharefiles?cmd=pvdesc
                int start; //文件起点
                int count; //文件个数
                get_fileslist_json_info(buf, &start, &count); //通过json包获取信息
                _log.log(SHAREFILES_LOG_MODULE, SHAREFILES_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"start = %d, count = %d\n", start, count);
                if (strcmp(cmd, "normal") == 0)
                {
                    get_share_filelist(start, count); //获取共享文件列表
                }
                else if(strcmp(cmd, "pvdesc") == 0)
                {
                    get_ranking_filelist(start, count);//获取共享文件排行版
                }


            }
        }

    }

    return 0;
}