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

char DEALFILE_LOG_MODULE[10] = "cgi";
char DEALFILE_LOG_PROC[10]  = "dealfile";
//mysql 数据库配置信息 用户名， 密码， 数据库名称
char mysql_user[128] = {0};
char mysql_pwd[128] = {0};
char mysql_db[128] = {0};
char mysql_ip[128] = {0};
char mysqlport[128] = {0};
//redis 服务器ip、端口
char redis_ip[30] = {0};
char redis_port[10] = {0};
print_log _log;
//返回前端情况
char* return_status(char *status_num)
{
    char *out = nullptr;
    cJSON *root = cJSON_CreateObject();  
    cJSON_AddStringToObject(root, "code", status_num);// {"code":"000"}
    out = cJSON_Print(root);
    cJSON_Delete(root);
    return out;
}
//解析的json包
int get_json_info(char *buf, char *user, char *token, char *md5, char *filename)
{
    int ret = 0;
    /*json数据如下
    {
    "user": "yoyo",
    "token": "xxxx",
    "md5": "xxx",
    "filename": "xxx"
    }
    */
    //解析json包
    //解析一个json字符串为cJSON对象
    cJSON * root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_Parse err\n");
        ret = -1;
        if(root != nullptr)
        {
            cJSON_Delete(root);//删除json对象
            root = nullptr;
        }
        return ret;
    }

    //返回指定字符串对应的json对象
    //用户
    cJSON *child1 = cJSON_GetObjectItem(root, "user");
    if(child1 == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        if(root != nullptr)
        {
            cJSON_Delete(root);//删除json对象
            root = nullptr;
        }
        return ret;
    }   
   
    //文件md5码
    cJSON *child2 = cJSON_GetObjectItem(root, "md5");
    if(child2 == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        if(root != nullptr)
        {
            cJSON_Delete(root);//删除json对象
            root = nullptr;
        }
        return ret;
    }    

    //文件名字
    cJSON *child3 = cJSON_GetObjectItem(root, "filename");
    if(child3 == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        if(root != nullptr)
        {
            cJSON_Delete(root);//删除json对象
            root = nullptr;
        }
        return ret;
    }    

    //token
    cJSON *child4 = cJSON_GetObjectItem(root, "token");
    if(child4 == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cJSON_GetObjectItem err\n");
        ret = -1;
        if(root != nullptr)
        {
            cJSON_Delete(root);//删除json对象
            root = nullptr;
        }
        return ret;
    }
    //拷贝内容
    strcpy(user, child1->valuestring); 
    strcpy(md5, child2->valuestring); 
    strcpy(filename, child3->valuestring);
    strcpy(token, child4->valuestring); 
    if(root != nullptr)
    {
        cJSON_Delete(root);//删除json对象
        root = nullptr;
    }

    return ret;
}

//分享文件
int share_file(char *user, char *md5, char *filename)
{
    /*
    a)先判断此文件是否已经分享，判断集合有没有这个文件，如果有，说明别人已经分享此文件，中断操作(redis操作)
    b)如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)
    c)如果mysql有记录，而redis没有记录，说明redis没有保存此文件，redis保存此文件信息后，再中断操作(redis操作)
    d)如果此文件没有被分享，mysql保存一份持久化操作(mysql操作)
    e)redis集合中增加一个元素(redis操作)
    f)redis对应的hash也需要变化 (redis操作)
    
        分享文件：
                成功：{"code":"010"}
                失败：{"code":"011"}
                别人已经分享此文件：{"code", "012"}
    
    */
    int ret = 0;
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    redisContext* redis_conn = nullptr;
    char *out = nullptr;
    char tmp[512] = {0};
    char fileid[1024] = {0};
    int ret2 = 0;
    //连接redis数据库
    database db;
    redis_conn = db.redis_connect(redis_ip, redis_port);
    if (redis_conn == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"redis connected error");
        ret = -1;
        out = nullptr;
        out = return_status((char*)"011");
        printf(out);
        delete out;
        return ret;
    }

    //connect the database
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;
        out = nullptr;
        out = return_status((char*)"011");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");

    //文件标示，md5+文件名
    sprintf(fileid, "%s%s", md5, filename);

    //===1、先判断此文件是否已经分享，判断集合有没有这个文件，如果有，说明别人已经分享此文件，中断操作(redis操作)
    ret2 = db.zset_exit(redis_conn, (char*)"FILE_PUBLIC_ZSET", fileid);
    if(ret2 == 1) //存在
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"别人已经分享此文件\n");
        ret = -2;
        out = nullptr;
        out = return_status((char*)"012");
        printf(out);
        delete out;
        redisFree(redis_conn);
        mysql_close(conn);
        return ret;
    }
    else if(ret2 == 0) //不存在
    {
        //===2、如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)
        //===3、如果mysql有记录，而redis没有记录，说明redis没有保存此文件，redis保存此文件信息后，再中断操作(redis操作)
        //查看此文件别人是否已经分享了
        sprintf(sql_cmd, "select * from share_file_list where md5 = '%s' and filename = '%s'", md5, filename);

        //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
        ret2 = db.process_result_one(conn, sql_cmd, nullptr); 
        if(ret2 == 2) //说明有结果，别人已经分享此文件
        {
            //redis保存此文件信息
            db.zset_add(redis_conn, (char*)"FILE_PUBLIC_ZSET", 0, fileid);
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"别人已经分享此文件\n");
            ret = -2;
            out = nullptr;
            out = return_status((char*)"012");
            printf(out);
            delete out;
            redisFree(redis_conn);
            mysql_close(conn);
            return ret;
        }
    }
    else
    {
        ret = -1;
        out = nullptr;
        out = return_status((char*)"011");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    //===4、如果此文件没有被分享，mysql保存一份持久化操作(mysql操作)
    //sql语句, 更新共享标志字段
    sprintf(sql_cmd, "update user_file_list set shared_status = 1 where user = '%s' and md5 = '%s' and filename = '%s'", user, md5, filename);

    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out = nullptr;
        out = return_status((char*)"011");
        printf(out);
        delete out;
        redisFree(redis_conn);
        mysql_close(conn);
        return ret;
    }

    time_t now;;
    char create_time[25];
    //获取当前时间
    now = time(nullptr);
    strftime(create_time, 25-1, (char*)"%Y-%m-%d %H:%M:%S", localtime(&now));

    //分享文件的信息，额外保存在share_file_list保存列表
    /*
        -- user	文件所属用户
        -- md5 文件md5
        -- createtime 文件共享时间
        -- filename 文件名字
        -- pv 文件下载量，默认值为1，下载一次加1
    */
    sprintf(sql_cmd, "insert into share_file_list (user, md5, createtime, filename, pv) values ('%s', '%s', '%s', '%s', %d)", user, md5, create_time, filename, 0);
    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out = nullptr;
        out = return_status((char*)"011");
        printf(out);
        delete out;
        redisFree(redis_conn);
        mysql_close(conn);
        return ret;
    }

    //查询共享文件数量
    sprintf(sql_cmd, "select count from user_file_count where user = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    int count = 0;
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    ret2 = db.process_result_one(conn, sql_cmd, tmp); 
    if(ret2 == 1) //没有记录
    {
        //插入记录
        sprintf(sql_cmd, "insert into user_file_count (user, count) values('%s', %d)", "xxx_share_xxx_file_xxx_list_xxx_count_xxx", 1);
    }
    else if(ret2 == 0)
    {
        //更新用户文件数量count字段
        count = atoi(tmp);
        sprintf(sql_cmd, "update user_file_count set count = %d where user = '%s'", count+1, "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    }

    if(mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out = nullptr;
        out = return_status((char*)"011");
        printf(out);
        delete out;
        redisFree(redis_conn);
        mysql_close(conn);
        return ret;
    }

    //===5、redis集合中增加一个元素(redis操作)
    db.zset_add(redis_conn, (char*)"FILE_PUBLIC_ZSET", 0, fileid);

    //===6、redis对应的hash也需要变化 (redis操作)
    //     fileid ------>  filename
    db.hash_set(redis_conn, (char*)"FILE_NAME_HASH", fileid, filename);
    
    out = nullptr;
    if(ret == 0)
    {
        out = return_status((char*)"010");
    }
    else if(ret == -1)
    {
        out = return_status((char*)"011");
    }
    else if(ret == -2)
    {
        out = return_status((char*)"012");
    }

    if(out != nullptr)
    {
        printf(out);//给前端反馈信息
        free(out);
    }

    if(redis_conn != nullptr)
    {
        redisFree(redis_conn);
    }


    if(conn != nullptr)
    {
        mysql_close(conn);
    }

    return ret;
}

//从storage删除指定的文件，参数为文件id
int remove_file_from_storage(char *fileid)
{
    int ret = 0;
    parse_conf p_conf;
    //读取fdfs client 配置文件的路径
    char fdfs_cli_conf_path[256] = {0};
    p_conf.get_fdfs_client_conf(fdfs_cli_conf_path);

    char cmd[1024*2] = {0};
    sprintf(cmd, "fdfs_delete_file %s %s", fdfs_cli_conf_path, fileid);

    ret = system(cmd);
    _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"remove_file_from_storage ret = %d\n", ret);

    return ret;
}

//删除文件
int del_file(char *user, char *md5, char *filename)
{
    /*
    a)先判断此文件是否已经分享
    b)判断集合有没有这个文件，如果有，说明别人已经分享此文件(redis操作)
    c)如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)
    d)如果mysql有记录，而redis没有记录，那么删除文件处理只需要处理mysql (mysql操作)
    e)如果redis有记录，mysql和redis都需要处理，删除相关记录    
            删除文件：
                成功：{"code":"013"}
                失败：{"code":"014"}

    */
    int ret = 0;
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    redisContext* redis_conn = nullptr;
    char *out = nullptr;
    char tmp[512] = {0};
    char fileid[1024] = {0};
    int ret2 = 0;
    int count = 0;
    int share = 0;  //共享状态
    int flag = 0; //标志redis是否有记录
    //连接redis数据库
    database db;
    redis_conn = db.redis_connect(redis_ip, redis_port);
    if (redis_conn == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"redis connected error");
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        return ret;
    }

    //connect the database
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    //文件标示，md5+文件名
    sprintf(fileid, "%s%s", md5, filename);

    //===1、先判断此文件是否已经分享，判断集合有没有这个文件，如果有，说明别人已经分享此文件
    ret2 = db.zset_exit(redis_conn, (char*)"FILE_PUBLIC_ZSET", fileid);
    if(ret2 == 1) //存在
    {
        share = 1;  //共享标志
        flag = 1;   //redis有记录
    }
    else if(ret2 == 0) //不存在
    {
        //===2、如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)
        //sql语句
        //查看该文件是否已经分享了
        sprintf(sql_cmd, "select shared_status from user_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user, md5, filename);

        //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
        ret2 = db.process_result_one(conn, sql_cmd, tmp); 
        if(ret2 == 0)
        {
            share = atoi(tmp); //shared_status字段
        }
        else if(ret2 == -1)//失败
        {
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
            ret = -1;
            out = nullptr;
            out = return_status((char*)"014");
            printf(out);
            delete out;
            redisFree(redis_conn);
            return ret;
        }
    }
    else//出错
    {
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    //说明此文件被分享，删除分享列表(share_file_list)的数据
    if(share == 1)
    {
        //===3、如果mysql有记录，删除相关分享记录 (mysql操作)
        //删除在共享列表的数据
        sprintf(sql_cmd, "delete from share_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user, md5, filename);
        if (mysql_query(conn, sql_cmd) != 0)
        {
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            out = nullptr;
            out = return_status((char*)"014");
            printf(out);
            delete out;
            redisFree(redis_conn);
            return ret;
        }

        //共享文件的数量-1
        //查询共享文件数量
        sprintf(sql_cmd, "select count from user_file_count where user = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");

        //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
        ret2 = db.process_result_one(conn, sql_cmd, tmp); 
        if(ret2 != 0)
        {
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
            ret = -1;
            out = nullptr;
            out = return_status((char*)"014");
            printf(out);
            delete out;
            redisFree(redis_conn);
            return ret;
        }
        count = atoi(tmp);
        sprintf(sql_cmd, "update user_file_count set count = %d where user = '%s'", count-1, "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
        if (mysql_query(conn, sql_cmd) != 0)
        {
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            out = nullptr;
            out = return_status((char*)"014");
            printf(out);
            delete out;
            redisFree(redis_conn);
            return ret;
        }

        //===4、如果redis有记录，redis需要处理，删除相关记录
        if(1 == flag)
        {
            //有序集合删除指定成员
            db.zset_zrem(redis_conn, (char*)"FILE_PUBLIC_ZSET", fileid);

            //从hash移除相应记录
            db.hash_del(redis_conn, (char*)"FILE_NAME_HASH", fileid);
        }

    }
    //用户文件数量-1
    //查询用户文件数量
    sprintf(sql_cmd, "select count from user_file_count where user = '%s'", user);
    ret2 = db.process_result_one(conn, sql_cmd, tmp); 
    if(ret2 != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    count = atoi(tmp);
    if(count == 1)
    {
        //删除用户文件数量表对应的数据
        sprintf(sql_cmd, "delete from user_file_count where user = '%s'", user);
    }
    else
    {
        sprintf(sql_cmd, "update user_file_count set count = %d where user = '%s'", count-1, user);
    }
    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    //删除用户文件列表数据
    sprintf(sql_cmd, "delete from user_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user, md5, filename);

    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    //文件信息表(file_info)的文件引用计数count，减去1
    //查看该文件文件引用计数
    sprintf(sql_cmd, "select count from file_info where md5 = '%s'", md5);
    ret2 = db.process_result_one(conn, sql_cmd, tmp); 
    if(ret2 == 0)
    {
        count = atoi(tmp); //count字段
    }
    else
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    count--; //减一
    sprintf(sql_cmd, "update file_info set count=%d where md5 = '%s'", count, md5);
    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        out = nullptr;
        out = return_status((char*)"014");
        printf(out);
        delete out;
        redisFree(redis_conn);
        return ret;
    }

    if(count == 0) //说明没有用户引用此文件，需要在storage删除此文件
    {
        //查询文件的id
        sprintf(sql_cmd, "select file_id from file_info where md5 = '%s'", md5);
        ret2 = db.process_result_one(conn, sql_cmd, tmp); 
        if(ret2 != 0)
        {
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
            ret = -1;
            out = nullptr;
            out = return_status((char*)"014");
            printf(out);
            delete out;
            redisFree(redis_conn);
            return ret;
        }
        //删除文件信息表中该文件的信息
        sprintf(sql_cmd, "delete from file_info where md5 = '%s'", md5);
        if (mysql_query(conn, sql_cmd) != 0)
        {
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
            ret = -1;
            out = nullptr;
            out = return_status((char*)"014");
            printf(out);
            delete out;
            redisFree(redis_conn);
            return ret;
        }

        //从storage服务器删除此文件，参数为为文件id
        ret2 = remove_file_from_storage(tmp);
        if(ret2 != 0)
        {
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"remove_file_from_storage err\n");
            ret = -1;
            out = nullptr;
            out = return_status((char*)"014");
            printf(out);
            delete out;
            redisFree(redis_conn);
            return ret;
        }
    }
    
    out = nullptr;
    if(ret == 0)
    {
        out = return_status((char*)"013");
    }
    else
    {
        out = return_status((char*)"014");
    }

    if(out != nullptr)
    {
        printf(out);//给前端反馈信息
        free(out);
    }

    if(redis_conn != nullptr)
    {
        redisFree(redis_conn);
    }



    if(conn != nullptr)
    {
        mysql_close(conn);
    }

    return ret;
}

//文件下载标志处理
int pv_file(char *user, char *md5, char *filename)
{
    int ret = 0;
    char sql_cmd[512] = {0};
    MYSQL *conn = nullptr;
    char *out = nullptr;
    char tmp[512] = {0};
    int ret2 = 0;

    //connect the database
    database db;
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == nullptr)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;
        out = nullptr;
        out = return_status((char*)"017");
        printf(out);
        delete out;
        return ret;
    }

    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");

    //sql语句
    //查看该文件的pv字段
    sprintf(sql_cmd, "select pv from user_file_list where user = '%s' and md5 = '%s' and filename = '%s'", user, md5, filename);

    ret2 = db.process_result_one(conn, sql_cmd, tmp); 
    int pv = 0;
    if(ret2 == 0)
    {
        pv = atoi(tmp); //pv字段
    }
    else
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败\n", sql_cmd);
        ret = -1;
        printf(out);
        delete out;
        return ret;
    }

    //更新该文件pv字段，+1
    sprintf(sql_cmd, "update user_file_list set pv = %d where user = '%s' and md5 = '%s' and filename = '%s'", pv+1, user, md5, filename);

    if (mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        printf(out);
        delete out;
        return ret;
    }
    /*
    下载文件pv字段处理
        成功：{"code":"016"}
        失败：{"code":"017"}
    */
    out = nullptr;
    if(ret == 0)
    {
        out = return_status((char*)"016");
    }
    else
    {
        out = return_status((char*)"017");
    }

    if(out != nullptr)
    {
        printf(out);//给前端反馈信息
        free(out);
    }


    if(conn != nullptr)
    {
        mysql_close(conn);
    }

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

int main()
{
    char cmd[20];
    char user[128];   //用户名
    char token[128];      //token
    char md5[256];          //文件md5码
    char filename[256]; //文件名字
    //读取数据库配置信息    
    parse_conf p_conf;
    int res = p_conf.get_mysql_conf(mysql_user,mysql_pwd,mysql_db,mysql_ip,mysqlport);
    res = p_conf.get_redis_conf(redis_ip,redis_port);
    //阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {
         // 获取URL地址 "?" 后面的内容
        char *query = getenv("QUERY_STRING");
        //解析命令
        p_conf.query_parse_key_value(query, (char*)"cmd", cmd, nullptr);
        _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"cmd = %s\n", cmd);
        char *content_Length = getenv("CONTENT_LENGTH");
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
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"len = 0, No data from standard input\n");
        }
        else
        {
            char buf[4*1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); 
            if(ret == 0)
            {
                _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fread(buf, 1, len, stdin) err\n");
                continue;
            }

            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"buf = %s\n", buf);
            get_json_info(buf, user, token, md5, filename); //解析客户端json信息
            _log.log(DEALFILE_LOG_MODULE, DEALFILE_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"user = %s, token = %s, md5 = %s, filename = %s\n", user, token, md5, filename);

            //验证登陆token，成功返回0，失败-1
            ret = verify_token(user, token); 
            if(ret != 0)
            {
                char *out = return_status((char*)"111"); //token验证失败错误码
                if(out != nullptr)
                {
                    printf(out); //给前端反馈错误码
                    delete out;
                }
                continue;
            }
            if(strcmp(cmd, "share") == 0) //分享文件
            {
                 share_file(user, md5, filename);
            }
            else if(strcmp(cmd, "del") == 0) //删除文件
            {
                del_file(user, md5, filename);
            }
            else if(strcmp(cmd, "pv") == 0) //文件下载标志处理
            {
                pv_file(user, md5, filename);
            }

        }

    }

    return 0;
}