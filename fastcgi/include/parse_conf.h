//#pragma once
#ifndef _PARSE_CONF_
#define _PARSE_CONF_
//#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<print_log.h>
#include<cJSON.h>
using namespace std;
class parse_conf{
public:
    //获取mysql配置文件信息
    int get_mysql_conf(char* mysql_user,char* mysql_pwd, char* mysql_db,char* mysql_ip, char* mysql_port);
    //获取redis配置文件信息
    int get_redis_conf(char* redis_ip,char* redis_port);
    int get_fdfs_client_conf(char* fdfs_cli_conf_path);
    int get_web_server_conf(char* storage_web_server_port);
    //解析url
    int query_parse_key_value(char *query, char *key, char *value, int *value_len_p);
private:
    char path[20] = "./conf/cfg.json";
    char CFG_LOG_MODULE[5] = "cgi";
    char CFG_LOG_PROC[5] = "cfg";
    print_log parse_log;
};
#endif