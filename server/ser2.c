#include"./head.h"

int main()
{
    int sockfd = 0;
    int count = 0;
    int maxfd = 0;
    int fd_arr[1024];
    int i;
    struct sockaddr_in seraddr;
	struct msg rm;
	MYSQL mysql;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("socket");
		exit(1);
	}

	bzero(&seraddr, sizeof(seraddr));
	seraddr.sin_family = AF_INET;
	seraddr.sin_addr.s_addr = inet_addr(SADDR);
	seraddr.sin_port = htons(SPORT);

	if(bind(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	if(listen(sockfd, 50) < 0)
	{
		perror("listen");
		exit(1);
	}

    fd_set fds; //创建文件描述符集合
    FD_ZERO(&fds); //将文件描述符清空
    FD_SET(sockfd, &fds); //将套接字描述符加入文件描述符集合之中，用来监控
    maxfd = sockfd; //此时文件描述符的最大个数是sockfd
    fd_arr[count++] = sockfd; //将sockfd放入文件描述符表当中

    fd_set readfds;
    while(1)
    {
        readfds = fds; //读就绪的文件描述符集合 = 文件描述符集合
        int ret = select(maxfd+1, &readfds, NULL, NULL, NULL);
        if(ret < 0)
        {
            perror("select");
            exit(1);
            continue;
        }
        else if(ret == 0)
            printf("select timeout");
        else
        {
            //判断文件描述符sockfd是否在readfds文件描述符集合中，sockfd就绪，说明有新客户端连接
            if(FD_ISSET(sockfd, &readfds))
            {
                int newfd = accept(sockfd, NULL, 0);
                if(newfd < 0)
                {
                    perror("accept");
                    exit(1);
                }
                else
                {
                    printf("有新连接,fd = %d\n",newfd);
                    FD_SET(newfd, &fds);
                    if(newfd > maxfd)
                        maxfd = newfd;
                    fd_arr[count++] = newfd;
                }
            }
            else
            {
                maxfd = 0;
                for(i = 0; i < count; ++i)
                {
                    //除sockfd外的其他fd就绪，说明可以进行io操作
                    if(FD_ISSET(fd_arr[i], &readfds))
                    {

                        bzero(&rm, sizeof(struct msg));
                        //读取的字节数小于等于0，说明客户端断开了
                        if(recv(fd_arr[i], &rm, sizeof(rm), 0) <= 0)
                        {
                            printf("client%d offline.\n", fd_arr[i]);

                            //此处需要考虑客户机非正常退出时，需要将数据库内关于这个套接字的在线标志置为0
                            open_mysql(&mysql);
                            char query_str[100]={0};
                            sprintf(query_str, "update user_info set isonline=0,sockfd=-1 where sockfd=%d", fd_arr[i]);
                            mysql_query_error(mysql, query_str, strlen(query_str));
                            mysql_close(&mysql);

                            close(fd_arr[i]);
                            FD_CLR(fd_arr[i], &fds);
                            int j;
                            for(j = i; j < count-1; ++j)
                                fd_arr[j] = fd_arr[j+1];
                            --count;
                        }
                        else
                        {
                            //打开数据库
                            open_mysql(&mysql);

                            //根据type宏值不同，进行不同的操作
                            ser_section(rm, mysql, fd_arr[i]);

                            mysql_close(&mysql);
                        }
                    } //end if FD_ISSET

					if(fd_arr[i] > maxfd)
				       	maxfd = fd_arr[i];
                } // end for i

            } // end else
        }// end if FD_ISSET
    } // end while

    close(sockfd);
    return 0;
}


/*打开数据库；
 *没有入参；
 *返回值：mysql；
 */
void open_mysql(MYSQL *mysql)
{
    if(NULL == mysql_init(mysql))
    {
        perror("mysql_init");
        exit(-1);
    }
    if(NULL == mysql_real_connect(mysql, NULL, "root", "1604021026", "user", 0, NULL, 0))
    {
        perror("mysql_real_connect");
        exit(1);
    }
}


/*根据type宏值不同，进行不同的操作；
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void ser_section(struct msg rm, MYSQL mysql, int fd)
{
    switch(rm.type)
    {
	case REG_CHECK: //注册检查用户名是否重复
        reg_check(rm, mysql, fd);
        break;
    case REG: //注册
        reg(rm, mysql, fd);
        break;
    case LOG_IN: //登录
        login(rm, mysql, fd);
        break;
    case LOG_OUT: //退出登录
        logout(rm, mysql, fd);
        break;
    case CHANGE_PWD: //改密码
        change_pwd(rm, mysql, fd);
        break;
    case AFTER_PWD: //修改后密码
        after_pwd(rm, mysql, fd);
        break;

    case GRO_CHAT: //群聊
        group_chat(rm, mysql, fd);
        break;
    case PRI_CHAT: //私聊
		private_chat(rm, mysql, fd);
        break;
    case FILE_NAME: //发文件
		send_file_name(rm, mysql, fd);
        break;

    case KICK: //踢人
        kick(rm, mysql, fd);
        break;
    case QUIET: //禁言
        quiet(rm, mysql, fd);
        break;
    case NO_QUIET: //解禁
        no_quiet(rm, mysql, fd);
        break;
    case ONLINE_NUM: //在线人数
        online_num(rm, mysql, fd);
        break;
    case ONLINE_NAME: //在线人员名单
        online_name(rm, mysql, fd);
    }
}


/*如果要进行操作的用户不存在，发送失败信号FAIL
 *入参：MYSQL_RES *res, struct msg *msgp, int fd
 *返回值：无;
 */
int no_exist(MYSQL_RES *res, struct msg *msgp, int fd)
{
    if(mysql_num_rows(res) == 0)
    {
        msgp->type = FAIL;
        send_msg(fd, msgp);
        return 1;
    }
    return 0;
}


/*发送消息
 *入参：int fd， struct msg rm;
 *返回值：无
 */
void send_msg(int fd, struct msg *msgp)
{
    if(send(fd, (void *)msgp, sizeof(struct msg), 0) < 0)
    {
        perror("send");
        exit(1);
    }
}


/*对mysql_real_query函数的返回值进行判断
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 *初步检测过，函数没问题
 */
void mysql_query_error(MYSQL mysql, char query_str[], int n)
{
    int rc = mysql_real_query(&mysql, query_str, n);
    if(rc != 0)
    {
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));
        exit(1);
    }
}


/*对mysql_real_query和mysql_store_result的返回值进行判断，并返回res
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：MYSQL_RES *res;
 *初步检测过，函数没问题
 */
MYSQL_RES *mysql_select_error(MYSQL mysql, char query_str[], int n)
{
    MYSQL_RES *res = NULL;
    int rc = mysql_real_query(&mysql, query_str, n);
    if(rc != 0)
    {
        printf("mysql_real_query(): %s\n", mysql_error(&mysql));
        exit(1);
    }

    res = mysql_store_result(&mysql);
    if(res == NULL)
    {
        printf("mysql_restore_result(): %s\n", mysql_error(&mysql));
        exit(1);
    }
    return res;
}


/*查看用户名是否已经被注册，如果已经被注册，发送失败信号
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void reg_check(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
	char query_str[100]={0};

	//查看用户名是否已经被注册
	sprintf(query_str, "select name from user_info where name='%s'", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
    if(mysql_num_rows(res) != 0)
        rm.type = FAIL;

	//发送给客户机, 让客户机自己循环判断，直到用户名没有被注册过，方可进行下一步注册
	send_msg(fd, &rm);

	//释放res内存
    mysql_free_result(res);
}



/*注册, 将注册信息插入数据库的user_info表中
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void reg(struct msg rm, MYSQL mysql, int fd)
{
	char query_str[100]={0};

    sprintf(query_str, "insert into user_info values('%s', '%s', '%s', '%s', 1, 1, 0, %d, 0)", rm.name, rm.pwd, rm.pro, rm.cont, fd);
    mysql_query_error(mysql, query_str, strlen(query_str));
}


/*登录，成功原样发送，失败发送FAIL，理由同注册
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void login(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query_str[100]={0};

	//查看用户是否存在，密码是否正确，如果不存在这样的用户，发送失败信号
	sprintf(query_str, "select name from user_info where name='%s' and pwd='%s'", rm.name, rm.pwd); //注意字符串还要加单引号，mysql里面的字符串需要单引号或者双引号
    res = mysql_select_error(mysql, query_str, strlen(query_str));

    //如果要进行操作的用户不存在
    if(no_exist(res, &rm, fd) == 1)
        return;

    //查看是否有群主
    memset(query_str, '\0', sizeof(query_str));
    sprintf(query_str, "select * from user_info where ismaster=1");
    res = mysql_select_error(mysql, query_str, strlen(query_str));
    memset(query_str, '\0', sizeof(query_str));

    //如果没有群主，就让这人当群主，并且显示在线
    if(mysql_num_rows(res) == 0)
        sprintf(query_str, "update user_info set sockfd=%d, isonline=1, ismaster=1 where name='%s'", fd, rm.name);
    else
        sprintf(query_str, "update user_info set sockfd=%d, isonline=1 where name='%s'", fd, rm.name);
    mysql_query_error(mysql, query_str, strlen(query_str));

    //登录成功发送消息给客户机，并且把是否是群主的消息放在rm.name里
	sprintf(query_str, "select name from user_info where ismaster=1");
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	row = mysql_fetch_row(res);
    if(strcmp(rm.name, row[0]) == 0)
		strcpy(rm.name, "yes");
	else
		strcpy(rm.name, "no");
    send_msg(fd, &rm);

    //释放res内存
    mysql_free_result(res);
}


/*退出登录，原样发送回去
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void logout(struct msg rm, MYSQL mysql, int fd)
{
    char query_str[100] = {0};
    sprintf(query_str, "update user_info set isonline=0 where sockfd=%d", fd);
    mysql_query_error(mysql, query_str, strlen(query_str));
    send_msg(fd, &rm);
}



/*修改密码1，成功发送原来的type，失败发送FAIL，理由同注册
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void change_pwd(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query_str[100]={0};

	sprintf(query_str, "select pro,ans from user_info where name='%s'", rm.name);
	res = mysql_select_error(mysql, query_str, strlen(query_str));

	//用户名不存在，发送失败信号，客户端提示用户名错误，需要重新提出修改密码申请
    int flag = no_exist(res, &rm, fd);
    if(flag == 1)
        return;

    //将密保问题和答案发送给客户机自己验证是否正确
	row = mysql_fetch_row(res);
	strcpy(rm.pro, row[0]);
	strcpy(rm.cont, row[1]);
	send_msg(fd, &rm);

	//释放res内存
    mysql_free_result(res);
}


/*修改密码2，成功发送ACK，理由同注册
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void after_pwd(struct msg rm, MYSQL mysql, int fd)
{
	char query_str[100]={0};

	sprintf(query_str, "update user_info set pwd='%s' where name='%s'", rm.pwd, rm.name);
	mysql_query_error(mysql, query_str, strlen(query_str));
}


/*群聊，发送方发送GRO_CHAT,如果收到QUIET，说明被禁言或者踢群，如果成功发送所有人会接收到GRO_CHAT
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无;
 */
void group_chat(struct msg rm, MYSQL mysql, int fd)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query_str[100]={0};
	int retu;

	//查看此人是否被禁言或者踢群
    sprintf(query_str, "select name,isquiet,ingroup from user_info where sockfd=%d",fd);
    res = mysql_select_error(mysql, query_str, strlen(query_str));

    //如果被踢群，则发送踢群信号
    row = mysql_fetch_row(res);
    if(atoi(row[2]) == 0)
    {
        rm.type = KICK;
        send_msg(fd, &rm);
        return;
    }
    //如果被禁言，则发送禁言信号
    if(atoi(row[1]) == 0)
    {
        rm.type = QUIET;
        send_msg(fd, &rm);
        return;
    }

    strcpy(rm.name, row[0]);

    //查找所有在线并且在群里的人，包括发消息人自己
    memset(query_str, '\0', sizeof(query_str));
    sprintf(query_str, "select sockfd from user_info where ingroup=1 and isonline=1");
    res = mysql_select_error(mysql, query_str, strlen(query_str));
    while((row = mysql_fetch_row(res)))
    {
        send_msg(atoi(row[0]), &rm);
    }

    //释放res内存
    mysql_free_result(res);
}


/*私聊
*入参：struct msg rm, MYSQL mysql, int fd;
*返回值：无;
*/
void private_chat(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
	char query_str[100]={0};
    int newfd;

    //查看用户是否存在
    sprintf(query_str, "select name from user_info where name='%s'", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	if(mysql_num_rows(res) == 0)
	{
		strcpy(rm.cont, "该用户不存在，请重新确认是否输错用户名!");
		rm.type = FAIL;
		send_msg(fd, &rm);
		return;
	}

    //如果要进行操作的用户对方处于下线状态
	memset(query_str, '\0', sizeof(query_str));
	sprintf(query_str, "select sockfd from user_info where name='%s' and isonline=1", rm.name);
	res = mysql_select_error(mysql, query_str, strlen(query_str));
	if(mysql_num_rows(res) == 0)
	{
		strcpy(rm.cont, "该用户处于下线状态，无法发送消息!");
		rm.type = FAIL;
		send_msg(fd, &rm);
        return;
	}

    //用户存在，得到接收方的套接字
    row = mysql_fetch_row(res);
    newfd = atoi(row[0]);

    //通过发送方套接字找到发送方姓名，放到rm.name里面
    memset(query_str, '\0', sizeof(query_str));
	sprintf(query_str, "select name from user_info where sockfd=%d", fd);
	res = mysql_select_error(mysql, query_str, strlen(query_str));
	row = mysql_fetch_row(res);
	strcpy(rm.name, row[0]);

	send_msg(newfd, &rm);

    //释放res空间
	mysql_free_result(res);
}


/*发文件
*入参：struct msg rm, MYSQL mysql, int fd;
*返回值：无;
*/
void send_file_name(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
	char query_str[100]={0};
    int newfd;

    //查看用户是否存在
    sprintf(query_str, "select name from user_info where name='%s'", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	if(mysql_num_rows(res) == 0)
	{
		strcpy(rm.cont, "该用户不存在，请重新确认是否输错用户名!");
		rm.type = FAIL;
		send_msg(fd, &rm);
		return;
	}

    //如果要进行操作的用户对方处于下线状态
	memset(query_str, '\0', sizeof(query_str));
	sprintf(query_str, "select sockfd from user_info where name='%s' and isonline=1", rm.name);
	res = mysql_select_error(mysql, query_str, strlen(query_str));
	if(mysql_num_rows(res) == 0)
	{
		strcpy(rm.cont, "该用户处于下线状态，无法接收文件!");
		rm.type = FAIL;
		send_msg(fd, &rm);
        return;
	}

	row = mysql_fetch_row(res);
    newfd = atoi(row[0]);

	//通过发送方套接字找到发送方姓名，放到rm.name里面
	memset(query_str, '\0', sizeof(query_str));
	sprintf(query_str, "select name from user_info where sockfd=%d", fd);
	res = mysql_select_error(mysql, query_str, strlen(query_str));
	row = mysql_fetch_row(res);
	strcpy(rm.name, row[0]);

	send_msg(newfd, &rm);
	while(1)
	{
		if(recv(fd, &rm, sizeof(struct msg), 0) <= 0)
		{
			perror("recv");
			exit(-1);
		}
		strcpy(rm.name, row[0]);
		send_msg(newfd, &rm);
		if(rm.type == FILE_DONE || rm.type == FILE_EXCEP)
			break;
	}

    //释放res空间
	mysql_free_result(res);
}



/*判断用户是否存在
 *入参：struct msg *rm, MYSQL mysql, int fd;
 *返回值：无;
 */
int user_exist(struct msg *rm, MYSQL mysql, int fd)
{
	MYSQL_RES *res = NULL;
    char query_str[100]={0};

	sprintf(query_str, "select name from user_info where name='%s'", rm->name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	strcpy(rm->cont, "该用户不存在，请重新确认是否输错用户名!");
	if(no_exist(res, rm, fd) == 1)
		return 1;
	else
		return 0;
}


/*踢人，如果用户不存在或者不在群里，发送失败信号FAIL
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无;
 */
void kick(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char query_str[100]={0};

    //查看用户是否存在
    if(user_exist(&rm, mysql, fd) == 1)
		return;

	//查看要踢的用户是否在群里
	sprintf(query_str, "select isonline, sockfd from user_info where name='%s' and ingroup=1", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	strcpy(rm.cont, "该用户不在群内!");
    if(no_exist(res, &rm, fd) == 1)
        return;

    //如果被踢的人在线，发消息告诉他已经被群主禁言，否则不发送
    row = mysql_fetch_row(res);
    if(atoi(row[0]) == 1)
        send_msg(atoi(row[1]), &rm);

    //将ingroup的值改为0
    memset(query_str, '\0', sizeof(query_str));
    sprintf(query_str, "update user_info set ingroup=0 where name='%s'", rm.name);
    mysql_query_error(mysql, query_str, strlen(query_str));

    //释放res空间
    mysql_free_result(res);
}


/*禁言，如果用户不存在或者已经被禁言，发送失败信号FAIL
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void quiet(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char query_str[100]={0};

	//查看用户是否存在
    if(user_exist(&rm, mysql, fd) == 1)
		return;

	//查看要踢的用户是否在群里
	sprintf(query_str, "select name from user_info where name='%s' and ingroup=1", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	strcpy(rm.cont, "该用户不在群内!");
    if(no_exist(res, &rm, fd) == 1)
        return;

    //如果用户已经被禁言
	memset(query_str, '\0', sizeof(query_str));
    sprintf(query_str, "select isonline, sockfd from user_info where name='%s' and isquiet=1", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	strcpy(rm.cont, "该用户已经被禁言!");
    if(no_exist(res, &rm, fd) == 1)
        return;

    //如果被禁言的人在线，发消息告诉他已经被群主禁言，否则不发送
    row = mysql_fetch_row(res);
    if(atoi(row[0]) == 1)
        send_msg(atoi(row[1]), &rm);

    //将isquiet改为0
    memset(query_str, '\0', sizeof(query_str));
    sprintf(query_str, "update user_info set isquiet=0 where name='%s'", rm.name);
    mysql_query_error(mysql, query_str, strlen(query_str));

    //释放res空间
    mysql_free_result(res);
}


/*解禁，如果用户不存在或者没有被禁言，发送失败信号FAIL
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无
 */
void no_quiet(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char query_str[100]={0};

	//查看用户是否存在
    if(user_exist(&rm, mysql, fd) == 1)
		return;

	//查看要踢的用户是否在群里
	sprintf(query_str, "select name from user_info where name='%s' and ingroup=1", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	strcpy(rm.cont, "该用户不在群内!");
    if(no_exist(res, &rm, fd) == 1)
        return;

	memset(query_str, '\0', sizeof(query_str));
    sprintf(query_str, "select isonline, sockfd from user_info where name='%s' and isquiet=0", rm.name);
    res = mysql_select_error(mysql, query_str, strlen(query_str));
	strcpy(rm.cont, "该用户未被禁言，无需解禁!");
    if(no_exist(res, &rm, fd) == 1)
        return;

    //如果被解禁的人在线，发消息告诉他已经被群主解禁，否则不发送
    row = mysql_fetch_row(res);
    if(atoi(row[0]) == 1)
        send_msg(atoi(row[1]), &rm);

    //将isquiet改为1
    memset(query_str, '\0', sizeof(query_str));
    sprintf(query_str, "update user_info set isquiet=1 where name='%s'", rm.name);
    mysql_query_error(mysql, query_str, strlen(query_str));

    //释放res空间
    mysql_free_result(res);
}


/*发送在线人数，已经对接
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无;
 */
void online_num(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res;
    char query_str[100] = {0};

    //查看在线人数，将人数写入rm.num里
    sprintf(query_str, "select name from user_info where isonline=1");
    res = mysql_select_error(mysql, query_str, strlen(query_str));
    rm.num = mysql_num_rows(res);
    send_msg(fd, &rm);
}


/*发送在线人员名单，已经对接
 *入参：struct msg rm, MYSQL mysql, int fd;
 *返回值：无;
 */
void online_name(struct msg rm, MYSQL mysql, int fd)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query_str[100] = {0};

    sprintf(query_str, "select name from user_info where isonline=1");
    res = mysql_select_error(mysql, query_str, strlen(query_str));
     //查将人数写入rm.num里
    rm.num = mysql_num_rows(res);
    //将姓名一个个写入rm.cont，中间有两个空格隔开
    rm.cont[0] = '\0';
    while(row = mysql_fetch_row(res))
    {
        strcat(rm.cont, row[0]);
        strcat(rm.cont,"  ");
    }
    send_msg(fd, &rm);
}

