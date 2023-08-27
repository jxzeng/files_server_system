#include "print_log.h"
#include<stdarg.h>
using namespace std;
/*
    往path中写入数据
*/
int print_log::out_put_file(char* path, char* buf){
    int fd;                                                                                                   
    fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0777);
    if(write(fd, buf, strlen(buf)) != (int)strlen(buf)) {                                      
        fprintf(stderr, "write error!\n");                           
        close(fd);                                                                                        
    } 
    else {                                                                                                        
        close(fd);                                                                                        
    }               

    return 0;
    
}
/*
    创建文件，文件路径————》path
*/
int print_log::make_path(char* path, char* module_name, char* proc_name){
    time_t t;
    struct tm* now = NULL;
    char top_dir[1024] = {"."};
    char second_dir[1024] = {"./logs"};
    char third_dir[1024] = {0};
	char y_dir[1024] = {0};
	char m_dir[1024] = {0};
	char d_dir[1024] = {0}; 
    time(&t);
    now = localtime(&t);
    snprintf(path, 1024, "./logs/%s/%04d/%02d/%s-%02d.log",
             module_name, now -> tm_year + 1900, now -> tm_mon + 1, proc_name, now -> tm_mday);	
	sprintf(third_dir, "%s/%s", second_dir, module_name);
	sprintf(y_dir, "%s/%04d/", third_dir, now -> tm_year + 1900);
	sprintf(m_dir, "%s/%02d/", y_dir, now -> tm_mon + 1);
	sprintf(d_dir,"%s/%02d/", m_dir, now -> tm_mday);
    //创建文件，并修改权限
    if(access(top_dir, 0) == -1) {
		if(mkdir(top_dir, 0777) == -1) {
			fprintf(stderr, "create %s failed!\n", top_dir);	
		} 
        else if(mkdir(second_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, second_dir);
		} 
        else if(mkdir(third_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", top_dir, third_dir);
		} 
        else if(mkdir(y_dir, 0777) == -1) {
            fprintf(stderr, "%s:create %s failed!\n", top_dir, y_dir);                                                     
        } 
        else if(mkdir(m_dir, 0777) == -1) {                                                             
            fprintf(stderr, "%s:create %s failed!\n", top_dir, m_dir);                                                     
        }          	
	} 
    else if(access(second_dir, 0) == -1) {
		if(mkdir(second_dir, 0777) == -1) {
			fprintf(stderr, "create %s failed!\n", second_dir);
		} 
        else if(mkdir(third_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", second_dir, third_dir);
                } 
        else if(mkdir(y_dir, 0777) == -1) {
            fprintf(stderr, "%s:create %s failed!\n", second_dir, y_dir);
        } 
        else if(mkdir(m_dir, 0777) == -1) {
            fprintf(stderr, "%s:create %s failed!\n", second_dir, m_dir);
        }
	} 
    else if(access(third_dir, 0) == -1) {
		if(mkdir(third_dir, 0777) == -1) {
			fprintf(stderr, "create %s failed!\n", third_dir);
		} 
        else if(mkdir(y_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", third_dir, y_dir);
		} 
        else if(mkdir(m_dir, 0777) == -1) {
			fprintf(stderr, "%s:create %s failed!\n", third_dir, m_dir);
		} 
	} 
    else if (access(y_dir, 0) == -1) {
		if(mkdir(y_dir, 0777) == -1) {
			fprintf(stderr, "create %s failed!\n", y_dir);
		} 
        else if(mkdir(m_dir, 0777) == -1) {
            fprintf(stderr, "%s:create %s failed!\n", y_dir, m_dir);
        } 

	} 
    else if (access(m_dir, 0) == -1) {
        if(mkdir(m_dir, 0777)) {
			fprintf(stderr, "create %s failed!\n", m_dir);
		} 
    }
	return 0;
}
int print_log::log(char* module_name, char* proc_name,char* filename, int line ,char* fmt, ...){
    //初始化mutex
    pthread_mutex_init(&_mutex,NULL);
    char mesg[4096]={0};
    char buf[4096]={0};
    char filepath[1024]={0};
    
    //时间
    time_t t=0;
    struct tm* now = nullptr;    
    time(&t);
    now = localtime(&t);
    //可变参数获取
    va_list ap;
    va_start(ap,fmt);
    vsprintf(mesg,fmt,ap);
    va_end(ap);
    //设置打印字符串-->buf
    snprintf(buf, 4096, "[%04d-%02d-%02d %02d:%02d:%02d]--[%s:%d]--%s",
                                now -> tm_year + 1900, now -> tm_mon + 1,                                         
                                now -> tm_mday, now -> tm_hour, now -> tm_min, now -> tm_sec,                     
								filename, line, mesg);
    //创建文件
    make_path(filepath,module_name,proc_name);
    //写入内容，共享资源，使用互斥锁同步
    pthread_mutex_lock(&_mutex);
    out_put_file(filepath,buf);
    pthread_mutex_unlock(&_mutex);
    return 0;
}