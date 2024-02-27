
#include <string.h>
#include <sstream>
#include <list>
#include "../net/tcpconnection.h"
#include "../net/protocolstream.h"
#include "../base/logging.h"
#include "../base/singleton.h"
#include "FileMsg.h"
#include "FileManager.h"
#include "FileSession.h"

using namespace net;

FileSession::FileSession(const std::shared_ptr<TcpConnection>& conn) 
    : TcpSession(conn), m_id(0),m_seq(0)
{
}

FileSession::~FileSession()
{
}

//OnRead,一致，功能是读数据包头控制信息，读出来包头数据之后交给Process处理
void FileSession::OnRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receivTime)
{
    while (true)
    {
        //不够一个包头大小
        if (pBuffer->readableBytes() < (size_t)sizeof(file_msg))
        {
            LOG_INFO << "buffer is not enough for a package header, pBuffer->readableBytes()=" << pBuffer->readableBytes() << ", sizeof(msg)=" << sizeof(file_msg);
            return;
        }

        //不够一个整包大小
        file_msg header;        //这个结构体里面只有一个packagesize包大小
        memcpy(&header, pBuffer->peek(), sizeof(file_msg)); //把pBuffer中的内容读取到header当中，peek只拿大小不拿数据
        if (pBuffer->readableBytes() < (size_t)header.packagesize + sizeof(file_msg))
            return;

        pBuffer->retrieve(sizeof(file_msg)); //retrieve会拿走数据，把数据拿出来给file_msg结构体保存
        std::string inbuf;
        inbuf.append(pBuffer->peek(), header.packagesize); //append追加(开始位置，大小) 开始位置是peek指向位置 现在指向的是数据包头，大小是包大小
        pBuffer->retrieve(header.packagesize); //信息追加完毕就从pBuffer中实际拿走这部分数据，大小还是包大小
        if (!Process(conn, inbuf.c_str(), inbuf.length()))  //把信息交给Process去处理
        {
            LOG_WARN << "Process error, close TcpConnection";
            conn->forceClose();
        }
    }// end while-loop

}

bool FileSession::Process(const std::shared_ptr<TcpConnection>& conn, const char* inbuf, size_t length)
{
    yt::BinaryReadStream2 readStream(inbuf, length);
    int cmd;
    if (!readStream.Read(cmd))          //下面读取的信息全都是包含在包头控制信息里的，也就是，协议数据格式中的消息头
    {               //指令类型
        LOG_WARN << "read cmd error !!!";
        return false;
    }

    //int seq;
    if (!readStream.Read(m_seq))        //次序
    {
        LOG_WARN << "read seq error !!!";
        return false;
    }

    std::string filemd5;
    size_t md5length;                   //文件名(md5加密形式)
    if (!readStream.Read(&filemd5, 0, md5length) || md5length == 0)
    {
        LOG_WARN << "read filemd5 error !!!";
        return false;
    }

    int offset;
    if (!readStream.Read(offset))       //偏移（大文件分批上传）
    {
        LOG_WARN << "read offset error !!!";
        return false;
    }

    int filesize;
    if (!readStream.Read(filesize))     //文件总大小（判断是否读完了）
    {
        LOG_WARN << "read filesize error !!!";
        return false;
    }

    string filedata;
    size_t filedatalength;              //文件实际数据
    if (!readStream.Read(&filedata, 0, filedatalength))
    {
        LOG_WARN << "read filedata error !!!";
        return false;
    }

   
    LOG_INFO << "Recv from client: cmd=" << cmd << ", seq=" << m_seq << ", header.packagesize:" << length << ", filemd5=" << filemd5 << ", md5length=" << md5length;
    LOG_DEBUG_BIN((unsigned char*)inbuf, length);

    switch (cmd)
    {
        //文件上传          //客户端传给服务器
        case msg_type_upload_req:
        {
            OnUploadFileResponse(filemd5, offset, filesize,  filedata, conn);
        }
            break;


        //客户端上传的文件内容, 服务器端下载        //服务器传给客户端
        case msg_type_download_req:
        {           
            //对于下载，客户端不知道文件大小， 所以值是0
            if (filedatalength != 0)
                return false;
            OnDownloadFileResponse(filemd5, offset, filesize,  conn);
        }
            break;

        default:
            //pBuffer->retrieveAll();
            LOG_WARN << "unsupport cmd, cmd:" << cmd << ", connection name:" << conn->peerAddress().toIpPort();
            //conn->forceClose();
            return false;
    }// end switch

    ++m_seq;

    return true;
}

void FileSession::OnUploadFileResponse(const std::string& filemd5, int32_t offset, int32_t filesize, const std::string& filedata, const std::shared_ptr<TcpConnection>& conn)
{   //上传文件
    if (filemd5.empty())
    {
        LOG_WARN << "Empty filemd5, connection name:" << conn->peerAddress().toIpPort();
        return;
    }
    
    std::string outbuf;
    yt::BinaryWriteStream3 writeStream(&outbuf);
    
    if (Singleton<FileManager>::Instance().IsFileExsit(filemd5.c_str()))
    {   //如果文件已经存在的话
        writeStream.Write(msg_type_upload_resp);
        writeStream.Write(m_seq);
        writeStream.Write(filemd5.c_str(), filemd5.length()); //把文件名写回去通知客户端
        offset = filesize = -1;     //-1就是终止命令
        writeStream.Write(offset);  //写回去一个-1，告诉客户端可以停了
        writeStream.Write(filesize);
        string dummyfiledata;
        writeStream.Write(dummyfiledata.c_str(), dummyfiledata.length());
        LOG_INFO << "Response to client: cmd=msg_type_upload_resp" << ", connection name:" << conn->peerAddress().toIpPort();
        writeStream.Flush();

        Send(outbuf);
        return;
    }
    //如果文件不存在的话，先把必须的回复准备好，然后开始接收文件，接收完毕写回复回去(先准备是因为万一接收中途挂了也是要写回复的)
    writeStream.Write(msg_type_upload_resp);
    writeStream.Write(m_seq);
    writeStream.Write(filemd5.c_str(), filemd5.length());
    //如果偏移是0，说明这是新文件，刚接收
    if (offset == 0)
    {
        string filename = "filecache/";
        filename += filemd5;
        m_fp = fopen(filename.c_str(), "w");    //以写入的形式打开文件
        if (m_fp == NULL)
        {
            LOG_INFO << "fopen file error, filemd5=" << filemd5 << ", connection name:" << conn->peerAddress().toIpPort();
            return;
        }
    }

        //注意：：接收文件的时候是直接把接收的数据写到磁盘里面去了，最后全部接收完毕在缓冲区里只留下了文件名，用以标记接收完毕

    fseek(m_fp, offset, SEEK_SET); //如果偏移不是0，说明不是新接收而是文件太大截开了，上面的就不会执行，直接执行这里的fseek，在服务端的文件中重定位，然后开始写入
    if (fwrite(filedata.c_str(), filedata.length(), 1, m_fp) != 1)  //把文件数据部分写入到文件中去，filedata是OnRead读到的内容
    {
		LOG_ERROR << "fwrite error, filemd5: " << filemd5
					<< ", errno: " << errno << ", errinfo: " << strerror(errno)
					<< ", filedata.length(): " << filedata.length()
					<< ", m_fp: " << m_fp
					<< ", buffer size is 512*1024"
					<< ", connection name:" << conn->peerAddress().toIpPort();
        return;
    }

    //文件上传成功
    if (offset + (int32_t)filedata.length() == filesize)  //偏移加上这一次的文件数据部分等于文件大小，说明所有的都传完了，也全部接收完毕
    {                                                   //这样就会执行这部分逻辑，文件全部接收完毕就调用addFIle，添加进去文件缓冲
        offset = filesize = -1;
        Singleton<FileManager>::Instance().addFile(filemd5.c_str());
        ResetFile();
    }

    writeStream.Write(offset);  //如果全部接收完毕 回复信息偏移和大小都是-1
    writeStream.Write(filesize); //如果截开了，一次没接收完，写回去的就是当次接收量和偏移
    string dummyfiledatax;
    writeStream.Write(dummyfiledatax.c_str(), dummyfiledatax.length());
    writeStream.Flush();

    Send(outbuf);
           
    LOG_INFO << "Response to client: cmd=msg_type_upload_resp" << ", connection name:" << conn->peerAddress().toIpPort();
}

void FileSession::OnDownloadFileResponse(const std::string& filemd5, int32_t offset, int32_t filesize, const std::shared_ptr<TcpConnection>& conn)
{
    if (filemd5.empty())
    {
        LOG_WARN << "Empty filemd5, connection name:" << conn->peerAddress().toIpPort();
        return;
    }
    
    //TODO: 客户端下载不存在的文件，不应答客户端？     ps:其实是需要应答一下的，但是这里没写
        //客户端申请下载文件的时候需要先判断你要下载的文件在服务器是否存在（就是list缓存中有没有这个文件名）,存在才可以继续进行
        //不存在的话，这里就直接返回了，客户端并不知道，所有应该需要写个回复通知客户端（但是客户端不会，所以不敢轻易写）
    if (!Singleton<FileManager>::Instance().IsFileExsit(filemd5.c_str()))
    {
        LOG_WARN << "filemd5 not exsit, filemd5: " << filemd5 << ", connection name:" << conn->peerAddress().toIpPort();
        return;
    }

    if (m_fp == NULL)  //安全保证，确保m_fp没有上一次残留的数据
    {
        string filename = "filecache/";
        filename += filemd5;  //拿到想要下载的文件的文件名
        m_fp = fopen(filename.c_str(), "r+");  //以读方式打开文件
        if (m_fp == NULL)
        {
            LOG_ERROR << "fopen file error, filemd5: " << filemd5 << ", connection name:" << conn->peerAddress().toIpPort();
            return;
        }
        fseek(m_fp, 0, SEEK_END);  //直接读到最后，拿到文件大小
        m_filesize = ftell(m_fp);
		if (m_filesize <= 0)
		{
			LOG_ERROR << "m_filesize: " << m_filesize << ", errno: " << errno << ", filemd5: " << filemd5 << ", connection name : " << conn->peerAddress().toIpPort();
			return;
		}
        fseek(m_fp, 0, SEEK_SET); //成功拿到文件大小之后再回去文件头，下面才开始正式读取
    }

    std::string outbuf;
    yt::BinaryWriteStream3 writeStream(&outbuf);
    writeStream.Write(msg_type_download_resp);
    writeStream.Write(m_seq);
    writeStream.Write(filemd5.c_str(), filemd5.length());  //先写公共部分的回复信息
        //然后开始读文件，写回去filedata
    string filedata;
    //m_offset += offset;
    int32_t currentSendSize = 512 * 1024; //一次发送512KB，半兆大小，不算很快
    char buffer[512 * 1024] = { 0 };
    if (m_filesize <= m_offset + currentSendSize) //如果文件大小甚至不到半兆或者剩下的部分已经不够512K(一个包大小)，那就以文件大小为主
    {
        currentSendSize = m_filesize - m_offset;
    }

	LOG_INFO << "currentSendSize: " << currentSendSize 
			 << ", m_filesize: " << m_filesize 
			 << ", m_offset: " << m_offset 
			 << ", filemd5: " << filemd5
			 << ", connection name:" << conn->peerAddress().toIpPort();
		

	if (currentSendSize <= 0 || fread(buffer, currentSendSize, 1, m_fp) != 1) //读的时候出错了
	{
		LOG_ERROR << "fread error, filemd5: " << filemd5
					<< ", errno: " << errno << ", errinfo: " << strerror(errno)
					<< ", currentSendSize: " << currentSendSize
					<< ", m_fp: " << m_fp
					<< ", buffer size is 512*1024"
					<< ", connection name:" << conn->peerAddress().toIpPort();
	}


    writeStream.Write(m_offset);
    m_offset += currentSendSize;  //记录传输的偏移量，文件过大的时候分片传输
    filedata.append(buffer, currentSendSize);    //回复信息的filedata部分写入当次数据(之所以是512K是因为这部分数据不宜过大)
    writeStream.Write(m_filesize);
    writeStream.Write(filedata.c_str(), filedata.length());
    writeStream.Flush();

    LOG_INFO << "Response to client: cmd = msg_type_download_resp, filemd5: " << filemd5 << ", connection name:" << conn->peerAddress().toIpPort();

    Send(outbuf);

    //文件已经下载完成
    if (m_offset == m_filesize)
    {
        ResetFile();
    }
}

void FileSession::ResetFile()
{
    if (m_fp)
    {
        fclose(m_fp);
        m_offset = 0;
        m_filesize = 0;
		m_fp = NULL;
    }
}