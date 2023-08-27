#include "database.h"
using namespace std;

/*
    数据库连接
*/
MYSQL* database::msql_conn(char *user_name, char* passwd, char *db_name, char* mysql_ip, char* mysql_port){
    //MYSQL对象句柄
    MYSQL *conn = nullptr;              
	//初始化
    conn = mysql_init(nullptr);
    if (conn == nullptr) 
	{
        fprintf(stderr, "mysql 初始化失败\n");
        return nullptr;
    }
	//mysql_real_connect()尝试与运行在主机上的MySQL数据库引擎建立连接
    //conn: 是已有MYSQL结构的地址。调用mysql_real_connect()之前，必须调用mysql_init()来初始化MYSQL结构。
	//NULL: 值必须是主机名或IP地址。如果值是NULL或字符串"localhost"，连接将被视为与本地主机的连接。
	//user_name: 用户的MySQL登录ID
	//passwd: 参数包含用户的密码
	if ( mysql_real_connect(conn, mysql_ip, user_name, passwd, db_name, atoi(mysql_port), nullptr, 0) == nullptr)
	{
        fprintf(stderr, "mysql_conn 失败:Error %u(%s)\n", mysql_errno(conn), mysql_error(conn));
		db_log.log(LOG_MODULE, LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql connect error\n");
        mysql_close(conn);
        return NULL;
    }

    return conn;
}
//处理数据库查询结果，结果集保存在buf，只处理一条记录，一个字段, 如果buf为NULL，无需保存结果集，只做判断有没有此记录
//返回值： 0成功并保存记录集，1没有记录集，2有记录集但是没有保存，-1失败
int database::process_result_one(MYSQL *conn, char *sql_cmd, char *buf){
    int res = 0;
    MYSQL_RES *res_set = nullptr;  //结果集结构的指针
    if (mysql_query(conn, sql_cmd)!= 0) //执行sql语句，执行成功返回0
    {        
        db_log.log(LOG_MODULE, LOG_PROC, (char*)__FILE__, __LINE__,(char*)"mysql_query error\n");
        res = -1;
        return res;
    }
    res_set = mysql_store_result(conn);//生成结果集
    if (res_set == nullptr)
    {        
        res = -1;
        return res;
    }
    MYSQL_ROW row;
    unsigned long line = 0;
    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (line == 0)
    {
        res = 1; //1没有记录集
        mysql_free_result(res_set);
        return res;
    }
    else if(line > 0 && buf == nullptr) //如果buf为NULL，无需保存结果集，只做判断有没有此记录
    {
        res = 2; //2有记录集但是没有保存
        mysql_free_result(res_set);
        return res;
    }

    // mysql_fetch_row从结果结构中提取一行，并把它放到一个行结构中。当数据用完或发生错误时返回NULL.
    if (( row = mysql_fetch_row(res_set) ) != nullptr)
    {
        if (row[0] != nullptr)
        {
            strcpy(buf, row[0]);
        }
    }
    mysql_free_result(res_set);
    return res;
}
/*
    redis数据库连接
*/
redisContext* database::redis_connect(char* redis_ip, char* redis_port){
    redisContext* conn = nullptr;
	uint16_t port = atoi(redis_port);
	conn = redisConnect(redis_ip, port);
	if (conn  == nullptr) {
		db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]Connect %s:%d Error:Can't allocate redis context!\n", redis_ip, port);		
		return conn;
	}
	if (conn->err) {
		db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]Connect %s:%d Error:%s\n", redis_ip, port, conn->errstr);	
		redisFree(conn);
		conn = NULL;
		return conn;
	}	
	db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC,(char*)__FILE__, __LINE__,(char*)"[+][GMS_REDIS]Connect %s:%d SUCCESS!\n", redis_ip, port);
    return conn;
}
int database::process_redis(redisContext* redis_conn,char* user, unsigned int time, char* token){
    int retn = 0;
    redisReply *reply = nullptr;
    reply = (redisReply*)redisCommand(redis_conn, "setex %s %u %s", user, time, token);    
    if (strcmp(reply->str, "OK") != 0) {
        retn = -1;
        freeReplyObject(reply);
        return retn;
    }
}

/* -------------------------------------------*/
/**
 *   得到集合中元素的个数
 *			>=0 个数
 *			-1 fail 
/* -------------------------------------------*/
int database::get_zcard(redisContext *conn, char *key)
{
    int cnt = 0;
    redisReply *reply = nullptr;
    reply = (redisReply *)redisCommand(conn, "ZCARD %s", key);
    if (reply->type != REDIS_REPLY_INTEGER)
    {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]ZCARD %s error %s\n", key, conn->errstr);
        cnt = -1;
        freeReplyObject(reply);
        return cnt;
    }
    cnt = reply->integer;
    freeReplyObject(reply);
    return cnt;
}

//删除一个key
int database::del_key(redisContext *conn, char *key)
{
	int retn = 0;
	redisReply *reply = nullptr;
	reply = (redisReply*)redisCommand(conn, "DEL %s", key);
	if (reply->type != REDIS_REPLY_INTEGER) {
		fprintf(stderr, "[-][GMS_REDIS] DEL key %s ERROR\n", key);
		db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS] DEL key %s ERROR %s\n", key, conn->errstr);
		retn = -1;
		freeReplyObject(reply);
	return retn;
	}

	if (reply->integer > 0) {
		retn = 0;	
	}
	else {
		retn = -1;
	}
	freeReplyObject(reply);
	return retn;
}

int database::zset_add(redisContext *conn, char* key, long score, char* member)
{
    int retn = 0;
    redisReply *reply = nullptr;
    //执行命令, reply->integer成功返回1，reply->integer失败返回0
    reply =(redisReply*) redisCommand(conn, "ZADD %s %ld %s", key, score, member);
    //rop_test_reply_type(reply);
	
    if (reply->integer != 1)
    {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]ZADD: %s,member: %s Error:%s,%s\n", key, member,reply->str, conn->errstr);
        retn = -1;
        freeReplyObject(reply);
        return retn;
    }
    freeReplyObject(reply);
    return retn;

}

int database::hash_set(redisContext *conn, char *key, char *field, char *value)
{
    int retn = 0;
    redisReply *reply = nullptr;

    reply =  (redisReply*)redisCommand(conn, "hset %s %s %s", key, field, value);
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]hset %s %s %s error %s\n", key, field, value,conn->errstr);
        retn =  -1;
        freeReplyObject(reply);
        return retn;
    }
    freeReplyObject(reply);
    return retn;
}

int database::zset_zrevrange(redisContext *conn, char *key, int from_pos, int end_pos, RVALUES values, int *get_num)
{
    int retn = 0;
    int i = 0;
    redisReply *reply = nullptr;
    int max_count = 0;
    int count = end_pos - from_pos + 1; //请求元素个数
    //降序获取有序集合的元素
    reply = (redisReply*)redisCommand(conn, "ZREVRANGE %s %d %d", key, from_pos, end_pos);
    if (reply->type != REDIS_REPLY_ARRAY) //如果返回不是数组
    {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]ZREVRANGE %s  error!%s\n", key, conn->errstr);
        retn = -1;
        if(reply != nullptr)
        {
            freeReplyObject(reply);
        }
        return retn;
    }
    //返回一个数组，查看elements的值(数组个数)    
    max_count = (reply->elements > count) ? count: reply->elements;
    *get_num = max_count; //得到结果value的个数
    for (i = 0; i < max_count; ++i)
    {
        strncpy(values[i], reply->element[i]->str, 1024-1);
        values[i][1024-1] = 0; //结束符
    }
    if(reply != NULL)
    {
        freeReplyObject(reply);
    }

    return retn;
}

int database::hash_get(redisContext *conn, char *key, char *field, char *value)
{
    int retn = 0;
    int len = 0;
    redisReply *reply = nullptr;
    reply =  (redisReply*)redisCommand(conn, "hget %s %s", key, field);
    if (reply == nullptr || reply->type != REDIS_REPLY_STRING) {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]hget %s %s  error %s\n", key, field, conn->errstr);
        retn =  -1;
        freeReplyObject(reply);
        return retn;
    }
    len = reply->len > 1024? 1024:reply->len ;
    strncpy(value, reply->str, len);
    value[len] = '\0';
    freeReplyObject(reply);


    return retn;
}

int database::zset_get_score(redisContext *conn, char *key, char *member)
{
    int score = 0;
    redisReply *reply = nullptr;
    reply = (redisReply*)redisCommand(conn, "ZSCORE %s %s", key, member);  
    if (reply->type != REDIS_REPLY_STRING) {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]ZSCORE %s %s error %s\n", key, member,conn->errstr);
        score = -1;
        freeReplyObject(reply);
        return score;
    }

    score = atoi(reply->str);
    freeReplyObject(reply);
    return score;
}

int database::zset_increment(redisContext *conn, char* key, char* member)
{
    int retn = 0;
    redisReply *reply = nullptr;
    reply = (redisReply*)redisCommand(conn, "ZINCRBY %s 1 %s", key, member);
    if (strcmp(reply->str, "OK") != 0)
    {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]Add or increment table: %s,member: %s Error:%s,%s\n", key, member,reply->str, conn->errstr);

        retn = -1;
        freeReplyObject(reply);
        return retn;
    }
    freeReplyObject(reply);
    return retn;
}

int database::hash_del(redisContext *conn, char *key, char *field)
{
    int retn = 0;
    redisReply *reply = nullptr;

    reply =  (redisReply*)redisCommand(conn, "hdel %s %s", key, field);
    if (reply->integer != 1)
    {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]hdel %s %s %s error %s\n", key, field,conn->errstr);
        retn =  -1;
        freeReplyObject(reply);
        return retn;
    }
    freeReplyObject(reply);
    return retn;

}

int database::zset_zrem(redisContext *conn, char* key, char* member)
{
    int retn = 0;
    redisReply *reply = nullptr;
    //执行命令, reply->integer成功返回1，reply->integer失败返回0
    reply = (redisReply*)redisCommand(conn, "ZREM %s %s", key, member);   
    if (reply->integer != 1)
    {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]ZREM: %s,member: %s Error:%s,%s\n", key, member,reply->str, conn->errstr);
        retn = -1;
        freeReplyObject(reply);
        return retn;
    }

    freeReplyObject(reply);
    return retn;

}

int database::zset_exit(redisContext *conn, char *key, char *member)
{
    int retn = 0;
    redisReply *reply = nullptr;
    //执行命令
    reply = (redisReply*)redisCommand(conn, "zlexcount %s [%s [%s", key, member, member);

    if (reply->type != REDIS_REPLY_INTEGER)
    {
        db_log.log(REDIS_LOG_MODULE, REDIS_LOG_PROC, (char*)__FILE__, __LINE__,(char*)"[-][GMS_REDIS]zlexcount: %s,member: %s Error:%s,%s\n", key, member,reply->str, conn->errstr);
        retn = -1;
        freeReplyObject(reply);
    return retn;
    }

    retn = reply->integer;
    freeReplyObject(reply);
    return retn;
}

int database::get_string(redisContext *conn, char *key, char *value)
{
    int retn = 0;
	redisReply *reply = nullptr;
	reply = (redisReply*)redisCommand(conn, "get %s", key);    
    if (reply->type != REDIS_REPLY_STRING) {
        retn = -1;
        freeReplyObject(reply);
        return retn;
    }
	strncpy(value, reply->str, reply->len);
	value[reply->len] = '\0'; //字符串结束符
	freeReplyObject(reply);
    return retn;
}