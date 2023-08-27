#include "parse_conf.h"

using namespace std;
/*
    ---读取本地的配置文件信息  "./conf/cfg.json"
    返回值：0 成功
          -1 失败
*/
int parse_conf::get_redis_conf(char* redis_ip,char* redis_port){
    int res = 0;
    char* buf = nullptr;
    FILE *fp = nullptr;    
    fp = fopen(path, "rb");
    if(fp == nullptr){
        fprintf(stderr,"open file error!\n");
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fopen error\n");
        res = -1;
        return res;
    }
    fseek(fp, 0, SEEK_END);//光标移动到末尾
    long size = ftell(fp); //获取文件大小
    fseek(fp, 0, SEEK_SET);//光标移动到开头
    buf = new char[size];
    fread(buf,1,size,fp);
    fclose(fp);
    //解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"root error\n");
        res = -1;
        delete buf;
        return res;
    }
    //返回指定字符串对应的json对象
    cJSON* obj_redis = cJSON_GetObjectItem(root, "redis");
    if(obj_redis == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_redis error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_subredis = cJSON_GetObjectItem(obj_redis, "ip");
    if(obj_subredis == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_subredis error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_subredis1 = cJSON_GetObjectItem(obj_redis, "port");
    if(obj_subredis1 == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_subredis1 error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }    
    //赋值    
    strcpy(redis_ip, obj_subredis->valuestring); 
    strcpy(redis_port, obj_subredis1->valuestring);    
    //删除json对象
    cJSON_Delete(root);
    return res;
}

int parse_conf::get_web_server_conf(char* storage_web_server_port){
    int res = 0;
    char* buf = nullptr;
    FILE *fp = nullptr;    
    fp = fopen(path, "rb");
    if(fp == nullptr){
        fprintf(stderr,"open file error!\n");
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fopen error\n");
        res = -1;
        return res;
    }
    fseek(fp, 0, SEEK_END);//光标移动到末尾
    long size = ftell(fp); //获取文件大小
    fseek(fp, 0, SEEK_SET);//光标移动到开头
    buf = new char[size];
    fread(buf,1,size,fp);
    fclose(fp);
    //解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"root error\n");
        res = -1;
        delete buf;
        return res;
    }
    //返回指定字符串对应的json对象
    cJSON* obj_redis = cJSON_GetObjectItem(root, "storage_web_server");
    if(obj_redis == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_redis error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_subredis = cJSON_GetObjectItem(obj_redis, "port");
    if(obj_subredis == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_subredis error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    
    //赋值    
    strcpy(storage_web_server_port, obj_subredis->valuestring);        
    //删除json对象
    cJSON_Delete(root);
    return res;
}

int parse_conf::get_fdfs_client_conf(char* fdfs_cli_conf_path){
    int res = 0;
    char* buf = nullptr;
    FILE *fp = nullptr;    
    fp = fopen(path, "rb");
    if(fp == nullptr){
        fprintf(stderr,"open file error!\n");
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fopen error\n");
        res = -1;
        return res;
    }
    fseek(fp, 0, SEEK_END);//光标移动到末尾
    long size = ftell(fp); //获取文件大小
    fseek(fp, 0, SEEK_SET);//光标移动到开头
    buf = new char[size];
    fread(buf,1,size,fp);
    fclose(fp);
    //解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"root error\n");
        res = -1;
        delete buf;
        return res;
    }
    //返回指定字符串对应的json对象
    cJSON* obj_redis = cJSON_GetObjectItem(root, "dfs_path");
    if(obj_redis == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_redis error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_subredis = cJSON_GetObjectItem(obj_redis, "client");
    if(obj_subredis == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_subredis error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }    
    //赋值  
    strcpy(fdfs_cli_conf_path, obj_subredis->valuestring);    
    //删除json对象
    cJSON_Delete(root);
    return res;
}

/*
    ---读取本地的配置文件信息  "./conf/cfg.json"
    返回值：0 成功
          -1 失败
*/
int parse_conf::get_mysql_conf(char* mysql_user,char* mysql_pwd, char* mysql_db, char* mysql_ip, char* mysql_port){
    int res = 0;
    char* buf = nullptr;
    FILE *fp = NULL;
    if(mysql_user == nullptr || mysql_pwd == nullptr || mysql_db == nullptr)
    {
        return -1;
    }
    fp = fopen(path, "rb");
    if(fp == NULL){
        fprintf(stderr,"open file error!\n");
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fopen error\n");
        res = -1;
        return res;
    }
    fseek(fp, 0, SEEK_END);//光标移动到末尾
    long size = ftell(fp); //获取文件大小
    fseek(fp, 0, SEEK_SET);//光标移动到开头
    buf = new char[size];
    fread(buf,1,size,fp);
    fclose(fp);
    //解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(buf);
    if(root == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"root error\n");
        res = -1;
        delete buf;
        return res;
    }

    //返回指定字符串对应的json对象
    cJSON* obj_mysql = cJSON_GetObjectItem(root, "mysql");
    if(obj_mysql == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_mysql error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_submysql = cJSON_GetObjectItem(obj_mysql, "user");
    if(obj_submysql == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_submysql error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_submysql2 = cJSON_GetObjectItem(obj_mysql, "password");
    if(obj_submysql2 == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_submysql2 error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_submysql3 = cJSON_GetObjectItem(obj_mysql, "database");
    if(obj_submysql3 == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC,(char*)__FILE__, __LINE__, (char*)"obj_submysql3 error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_submysql4 = cJSON_GetObjectItem(obj_mysql, "ip");
    if(obj_submysql4 == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_submysql4 error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    cJSON* obj_submysql5 = cJSON_GetObjectItem(obj_mysql, "port");
    if(obj_submysql5 == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"obj_submysql5 error\n");
        res = -1;
        cJSON_Delete(root);
        delete buf;
        return res;
    }
    //赋值    
    strcpy(mysql_user, obj_submysql->valuestring); 
    strcpy(mysql_pwd, obj_submysql2->valuestring);
    strcpy(mysql_db, obj_submysql3->valuestring);
    strcpy(mysql_ip, obj_submysql4->valuestring);
    strcpy(mysql_port, obj_submysql5->valuestring);
    //删除json对象
    cJSON_Delete(root);
    return res;
}

/**
 *   解析url query 类似 abc=123&bbb=456 字符串
 *          传入一个key,得到相应的value
 *          0 成功, -1 失败
 */
int parse_conf::query_parse_key_value(char *query, char *key, char *value, int *value_len_p)
{
    char *temp = nullptr;
    char *end = nullptr;
    int value_len =0;
    //找到是否有key
    temp = strstr(query, key);
    if (temp == nullptr)
    {
        parse_log.log(CFG_LOG_MODULE, CFG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"Can not find key %s in query\n", key);
        return -1;
    }
    temp += strlen(key);
    temp++;
    //get value
    end = temp;
    while ('\0' != *end && '#' != *end && '&' != *end )
    {
        end++;
    }
    value_len = end-temp;
    strncpy(value, temp, value_len);
    value[value_len] ='\0';
    if (value_len_p != nullptr)
    {
        *value_len_p = value_len;
    }
    return 0;
}