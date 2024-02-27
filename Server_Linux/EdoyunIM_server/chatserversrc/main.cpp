/*
* 一共两个启动文件，一个chatserver，一个fileserver
* 分别对应两个主文件夹：chatserversrc和fileserversrc
* 其他文件夹：
* database:数据库操作
* mysql:数据库事务操作，包括sql事务类，事务线程，事务管理
* net、base、jsoncpp、common:都是库文件
* 跟建议版本比起来，多的内容无非就，ClientSession的业务处理函数，多了一堆，以及文件服务器
*/
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../base/logging.h"
#include "../base/singleton.h"
#include "../mysql/mysqlmanager.h"
#include "../net/eventloop.h"
#include "../net/eventloopthreadpool.h"
#include "UserManager.h"
#include "IMServer.h"

using namespace net;

EventLoop g_mainLoop;

void prog_exit(int signo)
{
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;

    g_mainLoop.quit();
}

void daemon_run()
{
    int pid;
    signal(SIGCHLD, SIG_IGN);
    //1）在父进程中，fork返回新创建子进程的进程ID；
    //2）在子进程中，fork返回0；
    //3）如果出现错误，fork返回一个负值；
    pid = fork();
    if (pid < 0)
    {
        std::cout << "fork error" << std::endl;
        exit(-1);
    }
    //父进程退出，子进程独立运行
    else if (pid > 0) 
    {
        exit(0);
    }
    //之前parent和child运行在同一个session里,parent是会话（session）的领头进程,
    //parent进程作为会话的领头进程，如果exit结束执行的话，那么子进程会成为孤儿进程，并被init收养。
    //执行setsid()之后,child将重新获得一个新的会话(session)id。
    //这时parent退出之后,将不会影响到child了。
    setsid();
    int fd;
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
    }
    if (fd > 2)
        close(fd);
}


int main(int argc, char* argv[])
{
    //设置信号处理
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, prog_exit);
    signal(SIGKILL, prog_exit);
    signal(SIGTERM, prog_exit);

    short port = 0;
    int ch;
    bool bdaemon = false;
    while ((ch = getopt(argc, argv, "d")) != -1)
    {
        switch (ch)
        {
        case 'd':
            bdaemon = true;
            break;
        }
    }

    if (bdaemon)
        daemon_run();


    if (port == 0)
        port = 12345;

    Logger::setLogLevel(Logger::DEBUG);

    //初始化数据库配置
    const char* dbip = "192.168.65.132";
    const char* dbuser = "root";
    const char* dbpassword = "123456";
    const char* dbname = "myim";
    //CMysqlManager 只关注用户名和密码，以及数据库是否存在？ 所有的真真正正的数据库操作交给我成员m_poConn
	if (!Singleton<CMysqlManager>::Instance().Init(dbip, dbuser, dbpassword, dbname))
    {
        LOG_FATAL << "please check your database config..............";
    }
    //我只关心用户间的关系  
    if (!Singleton<UserManager>::Instance().Init(dbip, dbuser, dbpassword, dbname))
    {
        LOG_FATAL << "please check your database config..............";
    }

    Singleton<EventLoopThreadPool>::Instance().Init(&g_mainLoop, 4);                    //TODO:这俩可能需要着重看一看
    Singleton<EventLoopThreadPool>::Instance().start();

    Singleton<IMServer>::Instance().Init("0.0.0.0", 10721, &g_mainLoop);
    
    g_mainLoop.loop();

    return 0;
}
