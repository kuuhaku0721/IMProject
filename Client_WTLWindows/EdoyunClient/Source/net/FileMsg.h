
#pragma once

#include <stdint.h>

enum file_msg_type
{
    file_msg_type_unknown,
    msg_type_upload_req,
    msg_type_upload_resp,
    msg_type_download_req,
    msg_type_download_resp,
};

#pragma pack(push, 1)
//Э��ͷ
struct file_msg
{
    //linux������ֶ���64λ��
    int64_t  packagesize;       //ָ������Ĵ�С
};

#pragma pack(pop)

/** 
*  �ļ��ϴ�
*/
/**
    �ͻ��ˣ�cmd = msg_type_upload_req, seq = 0, filemd5, offset ,filesize, filedata
    ������Ӧ�� cmd = msg_type_upload_resp, seq = 0, filemd5, offset, filesize//offset��filesize=-1ʱ�ϴ����
**/

/** 
*  �ļ�����
*/
/** 
    �ͻ��ˣ�cmd = msg_type_download_req, seq = 0, filemd5, offset
    ������: cmd = msg_type_download_resp, seq = 0, filemd5, filesize, offset, filedata

**/

