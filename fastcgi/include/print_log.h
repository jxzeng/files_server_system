//#pragma once
#ifndef _PRINT_LOG_
#define _PRINT_LOG_
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<time.h>
#include<string.h>
using namespace std;
class print_log
{
private:
    /* data */
    pthread_mutex_t _mutex;
public:    
    //写入内容
    int out_put_file(char* path, char* buf);
    //创建目录
    int make_path(char* path, char* module_name, char* proc_name);
    //主函数
    int log(char* module_name, char* proc_name,char* filename, int line ,char* fmt, ...); //,  string filename,int line, const string funcname, string fmt

};

#endif