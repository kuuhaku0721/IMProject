
#pragma once

#include <stdint.h>

enum file_msg_type
{
    file_msg_type_unknown,      //未知类型
    msg_type_upload_req,        //上传request请求
    msg_type_upload_resp,       //上传response回复
    msg_type_download_req,      //下载request请求
    msg_type_download_resp,     //下载response回复
};

#pragma pack(push, 1)
//协议头
struct file_msg
{
    int64_t  packagesize;       //指定包体的大小
};

#pragma pack(pop)

/** 
 *  文件上传
 */
/**
    客户端：cmd = msg_type_upload_req, seq = 0, filemd5, offset ,filesize, filedata
    服务器应答： cmd = msg_type_upload_resp, seq = 0, filemd5, offset, filesize//offset与filesize=-1时上传完成
 **/

/** 
 *  文件下载
 */
/** 
    客户端：cmd = msg_type_download_req, seq = 0, filemd5, offset, filesize, filedata
    服务器: cmd = msg_type_download_resp, seq = 0, filemd5, filesize, offset, filedata

 **/

