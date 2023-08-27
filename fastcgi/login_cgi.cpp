/*
    登陆的处理cgi程序
*/
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
char LOGIN_LOG_MODULE[10] = "cgi";
char LOGIN_LOG_PROC[10] = "login";
print_log _log;
/*
    组装响应json报文
*/
char* package_JSON(char* buf, char* token){    
    char* out =nullptr;
    //创建json对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"code",buf);
    cJSON_AddStringToObject(root,"token",token);
    out = cJSON_Print(root);
    cJSON_Delete(root);
    return out;
}
/*
    查询数据库
    返回值 0 成功 -1 失败
*/
int user_check(char* user,char* pwd){    
    database db;
    MYSQL* conn = nullptr;
    char mysql_user[256] = {0};
    char mysql_pwd[256] = {0};
    char mysql_db[256] = {0};
    char mysql_ip[256] = {0};
    char mysql_port[256] = {0};
    parse_conf p_conf;
    int ret = p_conf.get_mysql_conf(mysql_user,mysql_pwd,mysql_db,mysql_ip,mysql_port);
    if(ret != 0){
        return -1;
    }
    _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql_user = %s, mysql_pwd = %s, mysql_db = %s\n",
             mysql_user, mysql_pwd, mysql_db);
    //连接数据库
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db,mysql_ip,mysql_port);
    if (conn == nullptr)
    {
        _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*) __FILE__, __LINE__,(char*)"msql_conn err\n");
        ret = -1;        
        return ret;
    }
    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    char sql_cmd[512] = {0};
    sprintf(sql_cmd, "select password from user where name=\"%s\"", user);
    char tmp[256] = {0};
    //执行sql
    db.process_result_one(conn, sql_cmd, tmp); 
    if(strcmp(tmp, pwd) == 0)
    {
        ret = 0;
    }
    else
    {
        ret = -1;
    }
    mysql_close(conn);      
    return ret;
}
/*
    生成token令牌
*/
int set_token(char* user, char* token)
{
    int ret = 0;
    database db;
    redisContext* redis_conn = nullptr;
    //redis 服务器ip、端口
    char redis_ip[30] = {0};
    char redis_port[10] = {0};
    //读取redis配置信息
    parse_conf p_conf;
    p_conf.get_redis_conf(redis_ip,redis_port);    
    _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*)__FILE__, __LINE__,(char*) "redis:[ip=%s,port=%s]\n", redis_ip, redis_port);
    //连接redis数据库
    redis_conn = db.redis_connect(redis_ip, redis_port);
    if (redis_conn == nullptr)
    {
        _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"redis connected error\n");
        ret = -1;
        redisFree(redis_conn);
        return ret;
    }    
    //产生4个1000以内的随机数
    int rand_num[4] = {0};
    int i = 0;
    //设置随机种子
    srand((unsigned int)time(NULL));
    for(i = 0; i < 4; ++i)
    {
        rand_num[i] = rand()%1000;//随机数
    }
    char tmp[1024] = {0};
    sprintf(tmp, "%s%d%d%d%d", user, rand_num[0], rand_num[1], rand_num[2], rand_num[3]);
    _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"tmp = %s\n", tmp);
    //加密
    char enc_tmp[1024*2] = {0};
    int enc_len = 0;
    ret = DesEnc((unsigned char *)tmp, strlen(tmp), (unsigned char *)enc_tmp, &enc_len);
    if(ret != 0)
    {
        _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"DesEnc error\n");
        ret = -1;
        redisFree(redis_conn);
        return ret;
    }
    //to base64
    char base64[1024*3] = {0};
    base64_encode((const unsigned char*)enc_tmp, enc_len, base64); //base64编码
    _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"base64 = %s\n", base64);

    //to md5
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char decrypt[16];
    MD5Update(&md5, (unsigned char *)base64, strlen(base64) );
    MD5Final(&md5, decrypt);
    char str[100] = { 0 };
    for (i = 0; i < 16; i++)
    {
        sprintf(str, "%02x", decrypt[i]);
        strcat(token, str);
    }
    // redis保存此字符串，用户名：token, 有效时间为12小时
    ret = db.process_redis(redis_conn, user, 43200, token);  
    redisFree(redis_conn);
    return ret;
}
/*
    解析用户登陆信息的json包

*/
int get_login_info(char* login_buf, char* user, char* pwd)
{
    int ret = 0;
    //解析json包
    //解析一个json字符串为cJSON对象
    cJSON* root = cJSON_Parse(login_buf);
    if(root == nullptr)
    {
        _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*)__FILE__, __LINE__, (char*) "cJSON_Parse err\n");
        ret = -1;
        return ret;
    }
    //返回指定字符串对应的json对象
    //用户
    cJSON* obj_user = cJSON_GetObjectItem(root, "user");
    if(obj_user == nullptr)
    {
        _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*)__FILE__, __LINE__, (char*) "cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = nullptr;
        return ret;
    }    
    strcpy(user, obj_user->valuestring); 
    //密码
    cJSON* obj_pwd = cJSON_GetObjectItem(root, "pwd");
    if(obj_pwd == nullptr)
    {
        _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*)__FILE__, __LINE__, (char*) "cJSON_GetObjectItem err\n");
        ret = -1;
        cJSON_Delete(root);//删除json对象
        root = nullptr;
        return ret;
    }    
    strcpy(pwd, obj_pwd->valuestring);
    return ret;
}
int main(){
    //阻塞等待用户连接
    while (FCGI_Accept() >= 0)
    {
        char* content_len = getenv("CONTENT_LENGTH");
        int len;
        //token,返回客户端的身份令牌
        char token[128] = {0};
        printf("Content-type: text/html\r\n\r\n");
        if( content_len == nullptr ){
            len = 0;
        }
        else{
            len = atoi(content_len); 
        }
        //没有登陆用户信息
        if (len <= 0)
        {
            printf("no data input.<p>\n");
            _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"len = 0, No data from standard input\n");
        }
        else //获取登陆用户信息
        {
            char buf[4096] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin); 
            if(ret == 0)
            {
                _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*)__FILE__, __LINE__,(char*) "fread(buf, 1, len, stdin) err\n");
                continue;
            }
            _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC,(char*)__FILE__, __LINE__,(char*) "buf = %s\n", buf);
            //解析客户端数据
            char user[512] = {0};
            char pwd[512] = {0};
            get_login_info(buf, user, pwd);
            _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"user = %s, pwd = %s\n", user, pwd);
            //查询数据库
            ret = user_check( user, pwd );
            if (ret == 0) //登陆成功
            {
                //生成token字符串
                memset(token, 0, sizeof(token));
                ret = set_token(user, token);
                _log.log(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"token = %s\n", token);

            }
            char* out = nullptr;
            if(ret == 0)
            {
                //返回前端登陆情况， 000代表成功
                out = package_JSON((char*)"000", token);

            }
            else
            {
                //返回前端登陆情况， 001代表失败
                out = package_JSON((char*)"001", (char*)"fail");
            }
            printf("%s",out); //给前端反馈信息
            delete(out);
        }
    }


    return 0;
}