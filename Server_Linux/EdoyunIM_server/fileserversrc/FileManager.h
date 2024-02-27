
#pragma once
#include <string>
#include <list>
#include <mutex>

class FileManager final
{
public:
    FileManager();
    ~FileManager();
    FileManager(const FileManager& rhs) = delete;
    FileManager& operator = (const FileManager& rhs) = delete;

    bool Init(const char* basepath);        //初始化,准备缓冲区
    bool IsFileExsit(const char* filename); //判断文件是否存在
    void addFile(const char* filename);     //添加文件

private:
    //上传的文件都以文件的md5值命名
    std::list<std::string>      m_listFiles;   //添加，判断是否存在，初始化，全都是把文件名保存到这个list缓冲区中
    std::mutex                  m_mtFile;
};