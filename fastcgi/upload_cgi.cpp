//#pragma once
#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include "cJSON.h"
#include "print_log.h"
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<mysql/mysql.h>
#include<string.h>
#include<parse_conf.h>
#include "database.h"
#include<sys/time.h>
#include "des.h"
#include "base64.h"
#include "md5.h"
#include<sys/wait.h>
using namespace std;
print_log _log;
char UPLOAD_LOG_MODULE[10]="cgi";
char UPLOAD_LOG_PROC[10] = "upload";

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

//通过文件名file_name， 得到文件后缀字符串, 保存在suffix 如果非法文件后缀,返回"null"
int get_file_suffix(const char *file_name, char *suffix)
{
    const char *p = file_name;
    int len = 0;
    const char *q=NULL;
    const char *k= NULL;
    if (p == NULL)
    {
        return -1;
    }
    q = p;
    //mike.doc.png
    while (*q != '\0')
    {
        q++;
    }
    k = q;
    while (*k != '.' && k != p)
    {
        k--;
    }
    if (*k == '.')
    {
        k++;
        len = q - k;

        if (len != 0)
        {
            strncpy(suffix, k, len);
            suffix[len] = '\0';
        }
        else
        {
            strncpy(suffix, "null", 5);
        }
    }
    else
    {
        strncpy(suffix, "null", 5);
    }

    return 0;
}

int trim_space(char *inbuf)
{
    int i = 0;
    int j = strlen(inbuf) - 1;
    char *str = inbuf;
    int count = 0;
    if (str == NULL ) 
	{        
        return -1;
    }
    while (isspace(str[i]) && str[i] != '\0') 
	{
        i++;
    }

    while (isspace(str[j]) && j > i) 
	{
        j--;
    }
    count = j - i + 1;
    strncpy(inbuf, str + i, count);
    inbuf[count] = '\0';
    return 0;
}
char* memstr(char* full_data, int full_data_len, char* substr) 
{ 
	//异常处理
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) 
	{ 
        return NULL; 
    } 
    if (*substr == '\0')
	{ 
        return NULL; 
    } 
	//匹配子串的长度
    int sublen = strlen(substr); 
    int i; 
    char* cur = full_data; 
    int last_possible = full_data_len - sublen + 1; 
    for (i = 0; i < last_possible; i++) 
	{ 
        if (*cur == *substr) 
		{ 
            if (memcmp(cur, substr, sublen) == 0) 
			{ 
                //found  
                return cur; 
            } 
        }
		
        cur++; 
    } 

    return NULL; 
} 
/* -------------------------------------------*/
//解析上传的post数据 保存到本地临时路径同时得到文件上传者、文件名称、文件大小
/* -------------------------------------------*/
int recv_save_file(long len, char *user, char *filename, char *md5, long *p_size)
{
    int ret = 0;
    char *file_buf = NULL;
    char *begin = NULL;
    char *p, *q, *k;
    char content_text[512] = {0}; //文件头部信息
    char boundary[512] = {0};     //分界线信息

    //开辟存放文件的 内存 
    file_buf = new char[len];
    if (file_buf == NULL)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"malloc error! file size is to big!!!!\n");
        return -1;
    }
    int ret2 = fread(file_buf, 1, len, stdin); 
    if(ret2 == 0)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"fread(file_buf, 1, len, stdin) err\n");
        ret = -1;
        delete file_buf;
        return ret;
    }

    //开始处理前端发送过来的post数据格式
    begin = file_buf;    //内存起点
    p = begin;
    /*
       ------WebKitFormBoundary88asdgewtgewx\r\n
       Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
       Content-Type: application/octet-stream\r\n
       \r\n
       真正的文件内容\r\n
       ------WebKitFormBoundary88asdgewtgewx
    */

    //get boundary 得到分界线, ------WebKitFormBoundary88asdgewtgewx
    p = strstr(begin, "\r\n");
    if (p == NULL)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"wrong no boundary!\n");
        ret = -1;
        delete file_buf;
        return ret;
    }

    //拷贝分界线
    strncpy(boundary, begin, p-begin);
    boundary[p-begin] = '\0';   //字符串结束符
    p += 2;//\r\n
    //已经处理了p-begin的长度
    len -= (p-begin);
    //get content text head
    begin = p;
    //Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    p = strstr(begin, "\r\n");
    if(p == NULL)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"ERROR: get context text error, no filename?\n");
        ret = -1;
        delete file_buf;
        return ret;
    }
    strncpy(content_text, begin, p-begin);
    content_text[p-begin] = '\0';   
    p += 2;//\r\n
    len -= (p-begin);
    //========================================获取文件上传者
    //Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n    
    q = begin;
    q = strstr(begin, "user=");

    //Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n    
    q += strlen("user=");
    q++;    //跳过第一个"

    //Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n    
    k = strchr(q, '"');
    strncpy(user, q, k-q);  //拷贝用户名
    user[k-q] = '\0';

    //去掉一个字符串两边的空白字符
    trim_space(user); 

    //========================================获取文件名字
    //"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    begin = k;
    q = begin;
    q = strstr(begin, "filename=");
    //"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n  
    q += strlen("filename=");
    q++;    //跳过第一个"
    //"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n 
    k = strchr(q, '"');
    strncpy(filename, q, k-q);  //拷贝文件名
    filename[k-q] = '\0';
    trim_space(filename);

    //========================================获取文件MD5码
    //"; md5="xxxx"; size=10240\r\n
    begin = k;
    q = begin;
    q = strstr(begin, "md5=");
    //"; md5="xxxx"; size=10240\r\n
    q += strlen("md5=");
    q++;    //跳过第一个"
    //"; md5="xxxx"; size=10240\r\n
    k = strchr(q, '"');
    strncpy(md5, q, k-q);   //拷贝文件名
    md5[k-q] = '\0';
    trim_space(md5);   

    //========================================获取文件大小
    //"; size=10240\r\n
    begin = k;
    q = begin;
    q = strstr(begin, "size=");
    //"; size=10240\r\n
    q += strlen("size=");
    //"; size=10240\r\n
    k = strstr(q, "\r\n");
    char tmp[256] = {0};
    strncpy(tmp, q, k-q);   //内容
    tmp[k-q] = '\0';
    *p_size = strtol(tmp, NULL, 10);
    begin = p;
    p = strstr(begin, "\r\n");
    p += 4;//\r\n\r\n
    len -= (p-begin);

    //下面才是文件的真正内容
    /*
       ------WebKitFormBoundary88asdgewtgewx\r\n
       Content-Disposition: form-data; user="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
       Content-Type: application/octet-stream\r\n
       \r\n
       真正的文件内容\r\n
       ------WebKitFormBoundary88asdgewtgewx
    */
    begin = p;
    //find file's end
    p = memstr(begin, len, boundary);
    if (p == NULL)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"memstr(begin, len, boundary) error\n");
        ret = -1;
        delete file_buf;
        return ret;
    }
    else
    {
        p = p - 2;//\r\n
    }    
    
    //=将数据写入文件中,其中文件名也是从post数据解析得来
    int fd = 0;
    fd = open(filename, O_CREAT|O_WRONLY, 0644);
    if (fd < 0)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"open %s error\n", filename);
        ret = -1;
        delete file_buf;
        return ret;
    }

    //ftruncate会将参数fd指定的文件大小改为参数length指定的大小
    ftruncate(fd, (p-begin));
    write(fd, begin, (p-begin));
    close(fd);
    delete file_buf;
    return ret;
}

//将一个本地文件上传到 后台分布式文件系统中
int upload_to_dstorage(char *filename, char *fileid)
{
    int ret = 0;
    pid_t pid;
    int fd[2];
    //无名管道的创建
    if (pipe(fd) < 0)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"pip error\n");
        ret = -1;
        return ret;
    }
    //创建进程
    pid = fork();
    if (pid < 0)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"fork error\n");
        ret = -1;
        return ret;
    }

    if(pid == 0) //子进程
    {
        //关闭读端
        close(fd[0]);
        //将标准输出 重定向 写管道
        dup2(fd[1], STDOUT_FILENO);      

        //读取fdfs client 配置文件的路径
        char fdfs_cli_conf_path[256] = {0};
        parse_conf p_conf;
        p_conf.get_fdfs_client_conf( fdfs_cli_conf_path);

        //通过execlp执行fdfs_upload_file
        execlp("fdfs_upload_file", "fdfs_upload_file", fdfs_cli_conf_path, filename, NULL);

        //执行失败
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"execlp fdfs_upload_file error\n");

        close(fd[1]);
    }
    else //父进程
    {
        //关闭写端
        close(fd[1]);

        //从管道中去读数据
        read(fd[0], fileid, 512);
        //去掉一个字符串两边的空白字符
        trim_space(fileid);
        if (strlen(fileid) == 0)
        {
            _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"[upload FAILED!]\n");
            ret = -1;
            return ret;
        }

        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"get [%s] succ!\n", fileid);
        wait(NULL); //等待子进程结束，回收其资源
        close(fd[0]);
    }
    return ret;
}

//封装文件存储在分布式系统中的 完整 url
int make_file_url(char *fileid, char *fdfs_file_url)
{
    int ret = 0;
    char *p = NULL;
    char *q = NULL;
    char *k = NULL;
    char fdfs_file_stat_buf[512] = {0};
    char fdfs_file_host_name[30] = {0};  //storage所在服务器ip地址
    pid_t pid;
    int fd[2];
    //无名管道的创建
    if (pipe(fd) < 0)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"pip error\n");
        ret = -1;
        return ret;
    }

    //创建进程
    pid = fork();
    if (pid < 0)//进程创建失败
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"fork error\n");
        ret = -1;
        return ret;
    }

    if(pid == 0) //子进程
    {
        //关闭读端
        close(fd[0]);
        //将标准输出 重定向 写管道
        dup2(fd[1], STDOUT_FILENO);
        //读取fdfs client 配置文件的路径
        char fdfs_cli_conf_path[256] = {0};
        parse_conf p_conf;
        p_conf.get_fdfs_client_conf(fdfs_cli_conf_path);
        execlp("fdfs_file_info", "fdfs_file_info", fdfs_cli_conf_path, fileid, NULL);

        //执行失败
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"execlp fdfs_file_info error\n");
        close(fd[1]);
    }
    else //父进程
    {
        //关闭写端
        close(fd[1]);

        //从管道中去读数据
        read(fd[0], fdfs_file_stat_buf, 512);
        //_log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "get file_ip [%s] succ\n", fdfs_file_stat_buf);

        wait(NULL); //等待子进程结束，回收其资源
        close(fd[0]);

        //拼接上传文件的完整url地址--->http://host_name/group1/M00/00/00/D12313123232312.png
        p = strstr(fdfs_file_stat_buf, "source ip address: ");
        q = p + strlen("source ip address: ");
        k = strstr(q, "\n");

        strncpy(fdfs_file_host_name, q, k-q);
        fdfs_file_host_name[k-q] = '\0';       
        //读取storage_web_server服务器的端口
        char storage_web_server_port[20] = {0};
        parse_conf p_conf;
        p_conf.get_web_server_conf(storage_web_server_port);
        strcat(fdfs_file_url, "http://");
        strcat(fdfs_file_url, fdfs_file_host_name);
        strcat(fdfs_file_url, ":");
        strcat(fdfs_file_url, storage_web_server_port);
        strcat(fdfs_file_url, "/");
        strcat(fdfs_file_url, fileid);

        //printf("[%s]\n", fdfs_file_url);
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"file url is: %s\n", fdfs_file_url);
    }
    return ret;
}


int store_fileinfo_to_mysql(char *user, char *filename, char *md5, long size, char *fileid, char *fdfs_file_url)
{
    int ret = 0;
    MYSQL *conn = NULL; 
    time_t now;;
    char create_time[25];
    char suffix[8];
    char sql_cmd[512] = {0};

    //连接 mysql 数据库
    database db;
    conn = db.msql_conn(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);
    if (conn == NULL)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"msql_conn connect err\n");
        ret = -1;        
        return ret;
    }

    //设置数据库编码
    mysql_query(conn, "set names utf8");
    //得到文件后缀字符串 如果非法文件后缀,返回"null"
    get_file_suffix(filename, suffix);     
    sprintf(sql_cmd, "insert into file_info (md5, file_id, url, size, type,filename, count) values ('%s', '%s', '%s', '%ld', '%s','%s', %d)",
            md5, fileid, fdfs_file_url, size, suffix,filename, 1);
    if (mysql_query(conn, sql_cmd) != 0) 
    {       
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 插入失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        mysql_close(conn);
        return ret;
    }

    _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s 文件信息插入成功\n", sql_cmd);
    //获取当前时间
    now = time(NULL);
    strftime(create_time, 25-1, "%Y-%m-%d %H:%M:%S", localtime(&now));
    //sql语句
    sprintf(sql_cmd, "insert into user_file_list(user, md5, createtime, filename, shared_status, pv) values ('%s', '%s', '%s', '%s', %d, %d)", user, md5, create_time, filename, 0, 0);
    if(mysql_query(conn, sql_cmd) != 0)
    {
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*) "%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        mysql_close(conn);
        return ret;
    }

    //查询用户文件数量
    sprintf(sql_cmd, "select count from user_file_count where user = '%s'", user);
    int ret2 = 0;
    char tmp[512] = {0};
    int count = 0;
    //返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
    ret2 = db.process_result_one(conn, sql_cmd, tmp); 
    if(ret2 == 1) //没有记录
    {
        //插入记录
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
        _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,(char*)__FILE__, __LINE__,(char*) "%s 操作失败: %s\n", sql_cmd, mysql_error(conn));
        ret = -1;
        mysql_close(conn);
        return ret;
    }

    if (conn != NULL)
    {
        mysql_close(conn); //断开数据库连接
    }

    return ret;
}


int main()
{
    char filename[256] = {0}; //文件名
    char user[256] = {0};   //文件上传者
    char md5[256] = {0};    //文件md5码
    long size;  //文件大小
    char fileid[512] = {0};    //文件上传到fastDFS后的文件id
    char fdfs_file_url[512] = {0}; //文件所存放storage的host_name

    //读取数据库配置信息
    parse_conf p_conf;
    p_conf.get_mysql_conf(mysql_user, mysql_pwd, mysql_db, mysql_ip, mysqlport);

    while (FCGI_Accept() >= 0)
    {
        char *contentLength = getenv("CONTENT_LENGTH");
        long len;
        int ret = 0;
        printf("Content-type: text/html\r\n\r\n");
        if (contentLength != NULL)
        {
            len = strtol(contentLength, NULL, 10); 
        }
        else
        {
            len = 0;
        }

        if (len <= 0)
        {
            printf("No data from standard input\n");
            _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"len = 0, No data from standard input\n");
            ret = -1;
        }
        else
        {
            //得到上传文件
            if (recv_save_file(len, user, filename, md5, &size) < 0)
            {
                ret = -1;                
                memset(filename, 0, 256);
                memset(user, 0, 128);
                memset(md5, 0, 256);
                memset(fileid, 0, 512);
                memset(fdfs_file_url, 0, 512);
                char *out = NULL;
                out = return_status((char*)"009");
                printf(out); //给前端反馈信息
                delete out; 
                
            }
            _log.log(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"%s成功上传[%s, 大小：%ld, md5码：%s]到本地\n", user, filename, size, md5);

            // 将该文件存入fastDFS中,并得到文件的file_id 
            if (upload_to_dstorage(filename, fileid) < 0)
            {
                ret = -1;
                ret = -1;
                memset(filename, 0, 256);
                memset(user, 0, 128);
                memset(md5, 0, 256);
                memset(fileid, 0, 512);
                memset(fdfs_file_url, 0, 512);
                char *out = NULL;
                out = return_status((char*)"009");
                printf(out); //给前端反馈信息
                delete out; 
                
            }
            //删除本地临时存放的上传文件
            unlink(filename);
            //得到文件所存放storage的host_name
            if (make_file_url(fileid, fdfs_file_url) < 0)
            {
                ret = -1;
                ret = -1;
                memset(filename, 0, 256);
                memset(user, 0, 128);
                memset(md5, 0, 256);
                memset(fileid, 0, 512);
                memset(fdfs_file_url, 0, 512);
                char *out = NULL;
                out = return_status((char*)"009");
                printf(out); //给前端反馈信息
                delete out; 
                
            }
            //将该文件的FastDFS相关信息存入mysql中
            if (store_fileinfo_to_mysql(user, filename, md5, size, fileid, fdfs_file_url) < 0)
            {
                ret = -1;
                memset(filename, 0, 256);
                memset(user, 0, 128);
                memset(md5, 0, 256);
                memset(fileid, 0, 512);
                memset(fdfs_file_url, 0, 512);
                char *out = NULL;
                out = return_status((char*)"009");
                printf(out); //给前端反馈信息
                delete out; 
                
            }

            memset(filename, 0, 256);
            memset(user, 0, 128);
            memset(md5, 0, 256);
            memset(fileid, 0, 512);
            memset(fdfs_file_url, 0, 512);
            char *out = NULL;
            //给前端返回，上传情况
            /*
               上传文件：
               成功：{"code":"008"}
               失败：{"code":"009"}
               */
            if(ret == 0) //成功上传
            {
                out = return_status((char*)"008");
            }
            else//上传失败
            {
                out = return_status((char*)"009");
            }
            if(out != NULL)
            {
                printf(out); //给前端反馈信息
                delete out;  
            }

        }

    } /* while */

    return 0;
}