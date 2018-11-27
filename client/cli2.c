
#include"./head.h"

/*聊天室总菜单菜单
 *入参：无
 *返回值：无
 */
int all_menu()
{
	printf("~~~~~~~~~~~~~~~~~~~~欢迎使用聊天室管理系统~~~~~~~~~~~~~~~~~~~~\n");
	printf("------[1]注册用户\n");
	printf("------[2]登录聊天室\n");
	printf("------[3]忘记或修改密码\n");
	printf("------[0]退出程序\n");
	printf("------请从<0-3>中选择操作类型:");
	int n = input_limit(5);
	return n;
}


/*一般登录菜单
 *入参：无
 *返回值：无
 */
int login_menu()
{
	printf("~~~~~~~~~~~~~~~~~~~~欢迎登录卓越班专属聊天室~~~~~~~~~~~~~~~~~~~~\n");
	printf("------[1]查看在线人数\n"); //服务器没写
	printf("------[2]打印所有在线人员名单\n"); //服务器没写
	printf("------[3]私聊\n");
	printf("------[4]群聊\n");
	printf("------[5]发送文件\n");
	printf("------[0]退出聊天室\n");
	printf("------请从<0-5>中选择操作类型:\n");
	int n = input_limit(5);
	return n;
}


/*群主登录菜单
 *入参：无
 *返回值：无
 */
int login_master_menu()
{
	printf("~~~~~~~~~~~~~~~~~~~~欢迎登录卓越班群主专属聊天室~~~~~~~~~~~~~~~~~~~~\n");
	printf("------[1]查看在线人数\n");
	printf("------[2]打印所有在线人员名单\n");
	printf("------[3]私聊\n");
	printf("------[4]群聊\n");
	printf("------[5]发送文件\n");
	printf("------[6]踢人\n");
	printf("------[7]禁言\n");
	printf("------[8]解禁\n");
	printf("------[0]退出聊天室\n");
	printf("------请从<0-8>中选择操作类型:\n");
	int n = input_limit(8);
	return n;
}


/*对菜单的输入数字进行约束
 *入参：int n;
 *返回值：无
 */
int input_limit(int n)
{
    int m;
    while(1)
    {
        scanf("%d",&m);
		getchar();
        if(m >= 0 && m <= n)
            break;
        printf("#####无此选项，请从<0-%d>中选择操作类型:",n);
    }
    return m;
}


/*进入聊天管理系统的一个选择，选择退出程序，就会返回主函数
 *入参：int fd;
 *返回值：无
 */
void instruction(int fd)
{
    int menu_ret;
    while(1)
    {
        menu_ret = all_menu();
        if(menu_ret == 0) //退出程序
            break;
        switch(menu_ret)
        {
        case 1:
            cli_reg(fd); //注册ok
            break;
        case 2:
            cli_login(fd); //登录
            break;
        case 3:
            cli_change_pwd(fd); //修改密码
        }
    }
}


/*进入聊天系统的一个线程，只有选择退出程序，才能跳出循环，结束线程
 *入参：void *aa
 *返回值：无
 */
void *client_send(void *aa)
{
    int menu_ret;
    struct msg sm;
    int *a = (int *)aa;

    while(1)
    {
        if(a[1] == 1) //说明是群主
            menu_ret = login_master_menu();
        else
            menu_ret = login_menu();
        switch(menu_ret)
        {
        case 1: //查看在线人数ok
            sm.type = ONLINE_NUM;
            send_msg(a[0], &sm);
            break;
        case 2: //打印所有在线人员名单ok
            sm.type = ONLINE_NAME;
            send_msg(a[0], &sm);
            break;
        case 3: //私聊ok
            sm.type = PRI_CHAT;
            printf("请输入对方用户名：");
            scanf("%s",sm.name);
            getchar();
            printf("请输入信息：");
            fgets(sm.cont, MES_SIZE-1, stdin);
			sm.cont[strlen(sm.cont)-1] = '\0';
            send_msg(a[0], &sm);
            break;
        case 4: //群聊ok
            sm.type = GRO_CHAT;
            printf("请输入信息：");
            fgets(sm.cont, MES_SIZE-1, stdin);
			sm.cont[strlen(sm.cont)-1] = '\0';
            send_msg(a[0], &sm);
            break;
        case 5: //发送文件ok
            send_file(a[0], &sm);
            break;
        case 6: //踢人ok
            sm.type = KICK;
            printf("请输入对方用户名：");
            scanf("%s",sm.name);
            send_msg(a[0], &sm);
            break;
        case 7: //禁言ok
            sm.type = QUIET;
            printf("请输入对方用户名：");
            scanf("%s",sm.name);
            send_msg(a[0], &sm);
            break;
        case 8: //解禁ok
            sm.type = NO_QUIET;
            printf("请输入对方用户名：");
            scanf("%s",sm.name);
            send_msg(a[0], &sm);
            break;
        case 0: //退出聊天室ok
            sm.type = LOG_OUT;
            send_msg(a[0], &sm);
            break;
        }
        if(menu_ret == 0)
            break;
		usleep(10);
    }
    pthread_exit(NULL);
}


/*发送文件
 *入参：int fd, struct msg *sm
 *返回值：无
 */
void send_file(int fd, struct msg *sm)
{
    char file[100] = {0};
    char newfile[30] = {0};
    FILE *fp;
	int i, flag = 0;
	struct msg rm;
	int ret;

    printf("请输入收件人:");
    scanf("%s",sm->name);
    getchar();

    printf("请输入要发送的文件名:");
    scanf("%s",file);
    fp = fopen(file, "r");
    if(fp == NULL)
    {
        perror("fopen");
        return;
    }

    //将文件名处理一下放入sm->pro
	for(i = 0; i < strlen(file); ++i)
	{
		if(file[i] == '/')
		{
			flag = 1;
			break;
		}
	}
	if(flag == 1)
	{
    	strcpy(newfile, strrchr(file,'/'));
    	strcpy(sm->pro, newfile+1);
	}
	else
		strcpy(sm->pro, file);

    sm->type = FILE_NAME;
    send_msg(fd, sm);

    while(1)
    {
        memset(sm->cont, '\0', MES_SIZE);
        if(feof(fp))
        {
			sm->type = FILE_DONE;
			printf("文件发送成功.\n");
			send_msg(fd, sm);
			break;
		}
		if(ferror(fp))
		{
			sm->type = FILE_EXCEP;
			printf("文件发送异常，已停止传输.\n");
			send_msg(fd, sm);
			break;
		}
		sm->type = FILE_CONT;
        ret = fread(sm->cont, 1, MES_SIZE-1, fp);
		if(ret < 0)
		{
			perror("fread");
			exit(-1);
		}
		sm->num = ret;
        send_msg(fd, sm);
    }
}


/*进入聊天系统的一个线程，一旦收到服务器发送的退出程序的信号，就会跳出循环，结束线程
 *入参：void *aa
 *返回值：无
 */
void *client_recv(void *aa)
{
    int *a = (int *)aa;
    struct msg rm;
	FILE *fp;

    while(1)
    {
        if(recv(a[0], &rm, sizeof(struct msg), 0) <= 0)
        {
            perror("send");
            exit(-1);
        }
        //当另一个子线程发送了退出请求，服务器接收到这个请求，再发送给这个子线程，来结束这个线程
        if(rm.type == LOG_OUT)
            break;

        switch(rm.type)
        {
        case ONLINE_NUM: //在线人数ok
            printf("共有%d人在线\n",rm.num);
            break;
        case ONLINE_NAME: //在线人员名单ok
            printf("共有%d人在线\n",rm.num);
            printf("名单：%s\n",rm.cont);
            break;

        case PRI_CHAT: //私聊
            printf("私聊：%s：%s\n", rm.name, rm.cont);
            break;
        case FAIL: //发送消息、禁言、解禁、踢人等失败
            printf("%s\n", rm.cont);
            break;

        case FILE_NAME: //接收文件名
            printf("%s向你发送了一份文件%s,接收中……\n",rm.name, rm.pro);
            break;
        case FILE_CONT: //接收文件内容
            fp = fopen(rm.pro, "a+");
            fprintf(fp, "%s", rm.cont);
            fclose(fp);
            break;
        case FILE_DONE: //文件接收成功
            printf("文件%s接收成功！\n", rm.pro);
            break;
        case FILE_EXCEP: //文件接收异常
            printf("文件%s接收异常，停止接收！\n", rm.pro);
            break;

        case GRO_CHAT: //群聊
            printf("群聊：%s:%s\n", rm.name, rm.cont);
            break;

        case QUIET: //说明被禁言
            printf("你已被群主禁言.\n");
            break;
        case NO_QUIET: //说明解禁
            printf("你已被群主解禁.\n");
            break;
        case KICK: //说明被踢群
            printf("你已被群主移出群.\n");
        }
    }
	pthread_exit(NULL);
}


/*main函数
 *入参：服务器ip地址
 *返回值：0
 */
int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("please input ip:\n");
		exit(1);
	}

	int sockfd;
	struct sockaddr_in seraddr;

	//连接
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("sockfd");
		exit(1);
	}

	bzero(&seraddr, sizeof(seraddr)); //结构体初始化,初始化网络协议
	seraddr.sin_family = AF_INET; //设置协议簇,确定ip协议
	seraddr.sin_addr.s_addr = inet_addr(argv[1]); //指定本地绑定网卡
	seraddr.sin_port = htons(SPORT); //指定绑定的监听端口

	if(connect(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr)) < 0)
	{
		perror("connect");
		exit(1);
	}

	instruction(sockfd);

	close(sockfd);
	return 0;
}


/*注册用户
 *入参：int fd;
 *返回值：无
 */
void cli_reg(int fd)
{
    struct msg sm;
    char buf[SHORT_SIZE];

	while(1)
	{
		sm.type = REG_CHECK;
    	printf("请输入用户名(len<20)：");
    	fgets(sm.name, SHORT_SIZE-1, stdin);
    	sm.name[strlen(sm.name)-1] = '\0';

		send_msg(fd, &sm);
		if(recv(fd, &sm, sizeof(struct msg), 0) <= 0)
    	{
        	perror("recv");
        	exit(1);
    	}
		if(sm.type != FAIL)
			break;
		printf("用户名已存在.\n");
	}

	sm.type = REG;
    while(1)
    {
        printf("请输入密码：");
        fgets(sm.pwd, SHORT_SIZE-1, stdin);
        sm.pwd[strlen(sm.pwd)-1] = '\0';

        printf("请再次确认密码：");
        fgets(buf, SHORT_SIZE-1, stdin);
        buf[strlen(buf)-1] = '\0';

        if(strcmp(sm.pwd, buf) == 0)
            break;
        printf("两次密码输入不一致，请重新输入密码！\n");
    }

    printf("请输入密保问题：");
    fgets(sm.pro, SHORT_SIZE-1, stdin);
    sm.pro[strlen(sm.pro)-1] = '\0';

    printf("请输入答案：");
    fgets(sm.cont, MES_SIZE-1, stdin);
    sm.cont[strlen(sm.cont)-1] = '\0';

    send_msg(fd, &sm);
    printf("注册成功!\n");
}


/*登录
 *入参：int fd;
 *返回值：无
 */
void cli_login(int fd)
{
    struct msg sm;
    int i;

    sm.type = LOG_IN;

    printf("请输入用户名：");
    fgets(sm.name, SHORT_SIZE-1, stdin);
    sm.name[strlen(sm.name)-1] = '\0';

    printf("请输入密码：");
    fgets(sm.pwd, SHORT_SIZE-1, stdin);
    sm.pwd[strlen(sm.pwd)-1] = '\0';

    printf("登陆中……\n");

    if(send(fd, (void *)&sm, sizeof(struct msg), 0) < 0)
    {
        perror("send");
        exit(1);
    }

    if(recv(fd, &sm, sizeof(struct msg), 0) <= 0)
    {
        perror("recv");
        exit(1);
    }

    if(sm.type == FAIL)
        printf("用户不存在或密码错误！\n");
    else
    {
        printf("登录成功！\n");

        int ret1, ret2;
        pthread_t pth1,pth2;
        void *retval;
        int a[2];

        a[0] = fd; //a[0]存放服务器套接字
        //a[1]存放是否是群主
        if(strcmp(sm.name, "yes") == 0)
            a[1] = 1;
        else
            a[1] = 0;

        ret1 = pthread_create(&pth1, NULL, client_send, (void *)a);
        ret2 = pthread_create(&pth2, NULL, client_recv, (void *)a); //可以用来添加一个功能，群主不能禁言、解禁和踢自己
        if(ret1 < 0 || ret2 < 0)
        {
            perror("pthread");
            exit(1);
        }

        pthread_join(pth1, &retval);
        pthread_join(pth2, &retval);
    }
}


/*修改密码
 *入参：int fd;
 *返回值：无
 */
void cli_change_pwd(int fd)
{
    struct msg sm;
    char ch[SHORT_SIZE];
	int i;

    printf("请输入用户名：");
    scanf("%s", sm.name);
    getchar();
    sm.type = CHANGE_PWD;
    send_msg(fd, &sm);

    if(recv(fd, &sm, sizeof(struct msg), 0) <= 0)
    {
        perror("recv");
        exit(1);
    }

    if(sm.type == FAIL)
    {
        printf("没有该用户.\n");
        return;
    }

	printf("你有3次机会回答密保问题.\n");
	for(i = 0; i < 3; ++i)
	{
    	printf("%s : ", sm.pro);
    	fgets(ch, SHORT_SIZE-1, stdin);
    	ch[strlen(ch)-1] = '\0';
    	if(strcmp(sm.cont, ch) != 0)
        	printf("回答错误.\n");
		else
			break;
	}

	if(i == 3)
	{
		printf("三次回答均不正确，请认真回忆答案在作答.\n");
		return;
	}
    printf("答案正确，请输入你的新密码：");
    fgets(sm.pwd, SHORT_SIZE-1, stdin);
    sm.pwd[strlen(sm.pwd)-1] = '\0';

    sm.type = AFTER_PWD;
    send_msg(fd, &sm);

    printf("密码修改成功.\n");
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

