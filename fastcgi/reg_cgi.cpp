// #include<stdio.h>
// #include<stdlib.h>
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
using namespace std;
char REG_LOG_MODULE[10] = "cgi";
char REG_LOG_PROC[10] = "reg";
#if 1
print_log _log;
/*
    解析客户端注册数据（json报文）
    报文格式如下
    {
        userName:xxxx,
        nickName:xxx,
        firstPwd:xxx,
        phone:xxx,
        email:xxx
    }    
    返回值：-1 解析失败
            0 成功
           
*/
int user_info_parse(char* buf,char* user,char* nickname,char* pwd,char* tel,char* email){
    int res = 0;
    cJSON* root = cJSON_Parse(buf);
    if(root == nullptr){
        _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"cJSON_Parse error\n");
        res = -1;        
        return res;
    }    
    //userName
    cJSON* obj_userName = cJSON_GetObjectItem(root,"userName");
    if(obj_userName == nullptr){
        _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"get userName error\n");
        res = -1;  
        cJSON_Delete(root);      
        return res;
    }
    //nickName
    cJSON* obj_nickName = cJSON_GetObjectItem(root,"nickName");
    if(obj_nickName == nullptr){
        _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"get nickName error\n");
        res = -1;  
        cJSON_Delete(root);      
        return res;
    }
    //firstPwd
    cJSON* obj_firstPwd = cJSON_GetObjectItem(root,"firstPwd");
    if(obj_firstPwd == nullptr){
        _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"get firstPwd error\n");
        res = -1;  
        cJSON_Delete(root);      
        return res;
    }
    //phone
    cJSON* obj_phone = cJSON_GetObjectItem(root,"phone");
    if(obj_phone == nullptr){
        _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"get phone error\n");
        res = -1;  
        cJSON_Delete(root);      
        return res;
    }
    //email
    cJSON* obj_email = cJSON_GetObjectItem(root,"email");
    if(obj_email == nullptr){
        _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"get email error\n");
        res = -1;  
        cJSON_Delete(root);      
        return res;
    }
    //拷贝数据
    strcpy(user,obj_userName->valuestring);
    strcpy(nickname,obj_nickName->valuestring);
    strcpy(pwd,obj_firstPwd->valuestring);
    strcpy(tel,obj_phone->valuestring);
    strcpy(email,obj_email->valuestring);    
    return res;

}
/*
    组装响应json报文
*/
char* package_JSON(char* buf){
    _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"%s\n",buf);
    char* out =nullptr;
    //创建json对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"code",buf);
    out = cJSON_Print(root);
    cJSON_Delete(root);
    return out;
}
/*
    数据库查询
    返回值：
        0  成功
        -1 失败
        -2 用户已存在
*/
int user_check(char* user,char* nickname,char* pwd,char* tel,char* email){
    int res = 0;
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
    _log.log(REG_LOG_MODULE, REG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql_user = %s, mysql_pwd = %s, mysql_db = %s\n",
             mysql_user, mysql_pwd, mysql_db);
    //连接数据库
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db,mysql_ip,mysql_port);
    if (conn == nullptr)
    {
        _log.log(REG_LOG_MODULE, REG_LOG_PROC,(char*) __FILE__, __LINE__,(char*)"msql_conn err\n");
        res = -1;
        mysql_close(conn); //断开数据库连接
        return res;
    }
    //设置数据库编码，主要处理中文编码问题
    mysql_query(conn, "set names utf8");
    char sql_cmd[512] = {0};
    sprintf(sql_cmd, "select * from user where name = '%s'", user);
    //查看该用户是否存在
    int res2 = 0;
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    res2 = db.process_result_one(conn, sql_cmd, NULL); //指向sql查询语句
    if(res2 == 2) //如果存在
    {
        _log.log(REG_LOG_MODULE, REG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"【%s】该用户已存在\n",user);
        res2 = -2;
        mysql_close(conn); //断开数据库连接
        return res2;
    }
    //当前时间戳
    struct timeval tv;
    struct tm* ptm;
    char time_str[128];
    //使用函数gettimeofday()函数来得到时间。它的精度可以达到微妙
    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec);//把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间    
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);
    //sql 语句, 插入注册信息
    sprintf(sql_cmd, "insert into user (name, nickname, password, phone, createtime, email) values ('%s', '%s', '%s', '%s', '%s', '%s')", user, nickname, pwd, tel, time_str ,email);
    if(mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(REG_LOG_MODULE, REG_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 插入失败：%s\n", sql_cmd, mysql_error(conn));
        res = -1;
        mysql_close(conn); //断开数据库连接
        return res;
    }
    return res;
}
#endif

int main(){
    //阻塞等待客户连接
    while(FCGI_Accept()>=0){
        char* content_len = getenv("CONTENT_LENGTH");
        int len;        
        printf("Content-type: text/html\r\n\r\n");
        if(content_len == nullptr){
            len = 0;
        }
        else{
            len = atoi(content_len);
        }
        if(len <= 0){
            //出错  
            printf(" no data input! <p>\n");
            _log.log(REG_LOG_MODULE, REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"len = 0, no data input\n");
        }
        else{
            //解析客户端数据
            /*
                响应客户端报文格式：json 
                响应状态值：{"code":"002"} -->用户创建成功
                          {"code":"003"} -->用户已存在
                          {"code":"004"} -->失败
            */
            char buf[4096] = {0};            
            char* out = nullptr;
            //读取客户端数据
            int ret = fread(buf,1,len,stdin);
            if(ret == 0){
                _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"fread(buf,1,len,stdin) error\n");                
            }
            _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"buf = %s\n",buf);
            //解析客户端数据            
            char user[128];
            char nickname[128];
            char pwd[128];
            char tel[128];
            char email[128];
            int state = user_info_parse(buf,user,nickname,pwd,tel,email);            
            if(state != 0){
                _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"json parse error\n");
            }
            _log.log(REG_LOG_MODULE, REG_LOG_PROC,(char*)__FILE__, __LINE__, (char*)"user = %s, nick_name = %s, pwd = %s, tel = %s, email = %s\n", 
                        user, nickname, pwd, tel, email);
            //从数据库查询用户信息
            state = user_check(user,nickname,pwd,tel,email);
            if(state == 0){
                //成功
                //组装json报文                
                out = package_JSON((char*)"002");
            }
            else if(state == -1)
            {
                //004代表失败                
                out = package_JSON((char*)"004");
            }
            else if(state == -2)
            {
                //用户已经存在                
                out = package_JSON((char*)"003");
            }

            if(out != NULL)
            {
                _log.log(REG_LOG_MODULE,REG_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"%s error\n",out);
                printf("%s",out); //给前端反馈信息
                delete(out); 
            }           

        }                     
        
    }


    return 0;
}


