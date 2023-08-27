//#pragma once
#ifndef _DATABASE_
#define _DATABASE_
//#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mysql/mysql.h>
#include "print_log.h"
#include <hiredis.h>
using namespace std;
typedef char (*RVALUES)[1024];
class database
{
public:
    /* data */
    int SQL_MAX_LEN = 512;
    //打印错误信息 
    print_log db_log;
    char LOG_MODULE[10] = "database";
    char LOG_PROC[10] = "mysql";
    char REDIS_LOG_PROC[10] = "redis";
    char REDIS_LOG_MODULE[10] = "database";
    
public:          
    //连接数据库
    MYSQL* msql_conn(char *user_name, char* passwd, char *db_name, char* mysql_ip, char* mysql_port);
    //处理数据库查询结果，结果集保存在buf，只处理一条记录，一个字段, 如果buf为NULL，无需保存结果集，只做判断有没有此记录
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    int process_result_one(MYSQL *conn, char *sql_cmd, char *buf);

    //-----redis--------
    redisContext* redis_connect(char* redis_ip, char* redis_port);
    //处理redis命令
    int process_redis(redisContext* redis_conn,char* user, unsigned int time, char* token);
    int get_zcard(redisContext *conn, char *key);
    int del_key(redisContext *conn, char *key);
    int zset_add(redisContext *conn, char* key, long score, char* member);
    int hash_set(redisContext *conn, char *key, char *field, char *value);
    int zset_zrevrange(redisContext *conn, char *key, int from_pos, int end_pos, RVALUES values, int *get_num);
    int hash_get(redisContext *conn, char *key, char *field, char *value);
    int zset_get_score(redisContext *conn, char *key, char *member);
    int zset_exit(redisContext *conn, char *key, char *member);
    int zset_zrem(redisContext *conn, char* key, char* member);
    int hash_del(redisContext *conn, char *key, char *field);
    int zset_increment(redisContext *conn, char* key, char* member);
    int get_string(redisContext *conn, char *key, char *value);
};
#endif