#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>
#include<errno.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include"mysql/mysql.h"


#define SPORT 7092
#define SADDR "192.168.179.138"
#define MES_SIZE 1024*10
#define SHORT_SIZE 20


#define REG_CHECK 0 //注册检查用户名是否重复
#define REG 1 //注册
#define LOG_IN 2 //登录
#define LOG_OUT 3 //退出登录
#define CHANGE_PWD 4 //改密码申请
#define AFTER_PWD 5 //修改后密码

#define GRO_CHAT 6 //群聊
#define PRI_CHAT 7 //私聊

#define FILE_NAME 8 //发文件名
#define FILE_CONT 9 //发文件内容
#define FILE_DONE 10 //文件发送完毕
#define FILE_EXCEP 11 //文件读取错误

#define KICK 12 //踢人，被踢
#define QUIET 13 //禁言，被禁
#define NO_QUIET 14 //解禁，被解禁

#define ACK 15 //成功，只有服务器发送给客户机，例如注册、登录成功
#define FAIL 16 //失败

#define ONLINE_NUM 17 //在线人数
#define ONLINE_NAME 18 //在线人员名单


struct msg
{
	int type;
	char name[SHORT_SIZE]; //注册或者登陆时的用户名；发送私聊时的对方用户名；群聊时发送方的用户名；踢人、禁言、解禁时的用户名；
	char pwd[SHORT_SIZE]; //注册或者登录时的密码
	char pro[SHORT_SIZE]; //注册时密保问题答案；改密码时的修改后密码
	char cont[MES_SIZE]; //私聊、群聊发送的内容；密保问题答案
	int num; //查看在线人数
};


void open_mysql();
void ser_section(struct msg rm, MYSQL mysql, int fd);
int no_exist(MYSQL_RES *res, struct msg *msgp, int fd);
void send_msg(int fd, struct msg *rm);
void mysql_query_error(MYSQL mysql, char query_str[], int n);
MYSQL_RES *mysql_select_error(MYSQL mysql, char query_str[], int n);
void reg_check(struct msg rm, MYSQL mysql, int fd);
void reg(struct msg rm, MYSQL mysql, int fd);
void login(struct msg rm, MYSQL mysql, int fd);
void logout(struct msg rm, MYSQL mysql, int fd);
void change_pwd(struct msg rm, MYSQL mysql, int fd);
void after_pwd(struct msg rm, MYSQL mysql, int fd);
void group_chat(struct msg rm, MYSQL mysql, int fd);
void private_chat(struct msg rm, MYSQL mysql, int fd);
void send_file_name(struct msg rm, MYSQL mysql, int fd);
void kick(struct msg rm, MYSQL mysql, int fd);
void quiet(struct msg rm, MYSQL mysql, int fd);
void no_quiet(struct msg rm, MYSQL mysql, int fd);
void kick_quiet_common(struct msg rm, MYSQL mysql, int fd, char *query_str);
void online_num(struct msg rm, MYSQL mysql, int fd);
void online_name(struct msg rm, MYSQL mysql, int fd);
