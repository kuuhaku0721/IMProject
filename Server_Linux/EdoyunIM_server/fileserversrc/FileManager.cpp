
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
//#include <sys/stat.h>
#include "../base/logging.h"
#include "FileManager.h"

FileManager::FileManager()
{
}

FileManager::~FileManager()
{
}

bool FileManager::Init(const char* basepath)
{
    DIR* dp = opendir(basepath);  //dir是磁盘路径信息，传入的参数是一个磁盘路径，目录不存在则创建，在下面
    if (dp == NULL)
    {
        LOG_INFO << "open base dir error, errno: " << errno << ", " << strerror(errno);
                       //用户读写执行  组读写执行  其他读         其他执行
        if (mkdir(basepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) //如果打开的目录不存在就使用mkdir命令创建目录
            return true;

        LOG_ERROR << "create base dir error, " << basepath << ", errno: " << errno << ", " << strerror(errno);

        return false;
    }

    struct dirent* dirp;
    //struct stat filestat;
    while ((dirp = readdir(dp)) != NULL)  //读取当前文件夹里面所有内容
    {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)  //排除掉 . 和 .. 两个目录
            continue;


        //if (stat(dirp->d_name, &filestat) != 0)
        //{
        //    LOG_WARN << "stat filename: [" << dirp->d_name << "] error, errno: " << errno << ", " << strerror(errno);
        //    continue;
        //}

        m_listFiles.emplace_back(dirp->d_name);
        LOG_INFO << "filename: " << dirp->d_name;
    }

    closedir(dp);   //读完名字之后就把文件夹关掉了.       TODO:啊？
    
    return true;
}

bool FileManager::IsFileExsit(const char* filename)
{
    std::lock_guard<std::mutex> guard(m_mtFile);
    //先查看缓存
    for (const auto& iter : m_listFiles)
    {
        if (iter == filename)
            return true;
    }

    //再查看文件系统
    FILE* fp = fopen(filename, "r");  //以读的方式打开文件
    if (fp != NULL)
    {   //如果打开成功，代表文件存在，关掉文件，把文件名加入缓存
        fclose(fp);
        m_listFiles.emplace_back(filename);
        return true;
    }

    return false;
}

void FileManager::addFile(const char* filename)
{
    std::lock_guard<std::mutex> guard(m_mtFile);
    m_listFiles.emplace_back(filename);
}
