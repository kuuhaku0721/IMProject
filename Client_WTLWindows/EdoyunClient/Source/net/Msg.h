
#pragma once

#include <stdint.h>

enum msg_type
{
    msg_type_unknown,
    msg_type_heartbeart = 1000,
    msg_type_register,
    msg_type_login,
    msg_type_getofriendlist,
    msg_type_finduser,
    msg_type_operatefriend,
    msg_type_userstatuschange,
    msg_type_updateuserinfo,
    msg_type_modifypassword,
    msg_type_creategroup,
    msg_type_getgroupmembers,
    msg_type_chat   = 1100,         //������Ϣ
    msg_type_multichat,             //Ⱥ����Ϣ
};

#pragma pack(push, 1)
//Э��ͷ
struct msg
{
    int32_t  packagesize;       //ָ������Ĵ�С
};

#pragma pack(pop)

/** 
 *  ������
 *  0   �ɹ�
 *  100 ע��ʧ��
 *  101 ��ע��
 *  102 δע��
 *  103 �������
 *  104 �����û���Ϣʧ��
 *  105 �޸�����ʧ��
 *  106 ����Ⱥʧ��
 */

/**
 *  ������Э��
 **/
/*
    cmd = 1000, seq = 0
    cmd = 1000, seq = 0
 **/

/**
 *  ע��Э��
 **/
/*
    cmd = 1001, seq = 0,  {"username": "13917043329", "nickname": "balloon", "password": "123"}
    cmd = 1001, seq = 0,  {"code": 0, "msg": "ok"}
 **/

/**
 *  ��¼Э��
 **/
/*
    //status: ����״̬ 0���� 1���� 2æµ 3�뿪 4����
    //clienttype: �ͻ�������,pc=1, android=2, ios=3
    cmd = 1002, seq = 0, {"username": "13917043329", "password": "123", "clienttype": 1, "status": 1}
    cmd = 1002, seq = 0, {"code": 0, "msg": "ok", "userid": 8, "username": "13917043320", "nickname": "zhangyl",
                          "facetype": 0, "customface":"�ļ�md5", "gender":0, "birthday":19891208, "signature":"���������ڳɹ���",
                          "address":"�Ϻ��ж���·3261��", "phonenumber":"021-389456", "mail":"balloonwj@qq.com"}
 **/

/** 
 * ��ȡ�û��б�
 **/
/*
    cmd = 1003, seq = 0
    cmd = 1003, seq = 0,  
    {"code": 0, "msg": "ok", "userinfo":[{"userid": 2,"username":"qqq", 
                                          "nickname":"qqq123", "facetype": 0, 
                                          "customface":"466649b507cdf7443c4e88ba44611f0c", 
                                          "gender":1, "birthday":19900101, "signature":"������Ҫ�ܶ������ѽ��xx",
                                          "address": "", "phonenumber": "", "mail":"", "clienttype": 1, 
                                          "status":1},
                                          {"userid": 3,"username":"hujing", "nickname":"hujingx", 
                                          "facetype": 0, "customface":"", "gender":0, "birthday":19900101, 
                                          "signature":"", "address": "", "phonenumber": "", "mail":"", 
                                          "clienttype": 1, "status":0}]}
**/

/** 
 * �����û�
 **/
/*
    //type�������� 0���У� 1�����û� 2����Ⱥ
    cmd = 1004, seq = 0, {"type": 1, "username": "zhangyl"}
    cmd = 1004, seq = 0, { "code": 0, "msg": "ok", "userinfo": [{"userid": 2, "username": "qqq", "nickname": "qqq123", "facetype":0}] } 
**/

/** 
 *  �������ѣ������Ӻ��ѡ�ɾ������
 **/
/* 
    //typeΪ1�����Ӻ������� 2 �յ��Ӻ�������(���ͻ���ʹ��) 3Ӧ��Ӻ��� 4ɾ���������� 5Ӧ��ɾ������
    //��type=3ʱ��accept�Ǳ����ֶΣ�0�Է��ܾ���1�Է�����
    cmd = 1005, seq = 0, {"userid": 9, "type": 1}
    cmd = 1005, seq = 0, {"userid": 9, "type": 2, "username": "xxx"}
    cmd = 1005, seq = 0, {"userid": 9, "type": 3, "username": "xxx", "accept": 1}

    //����
    cmd = 1005, seq = 0, {"userid": 9, "type": 4}
    //Ӧ��
    cmd = 1005, seq = 0, {"userid": 9, "type": 5, "username": "xxx"}
 **/

/**
 *  �û�״̬�ı�
 **/
/*
    //type 1����(��������չ�����ߡ�����æµ���뿪��״̬) 2���� 3�û�ǩ����ͷ���ǳƷ����仯
    cmd = 1006, seq = 0, {"type": 1, "onlinestatus": 1} //����onlinestatus=1, ����onlinestatus=0
    cmd = 1006, seq = 0, {"type": 3}
    **/

/**
 *  �����û���Ϣ
 **/
/*
    cmd = 1007, seq = 0, �û���Ϣ: {"nickname": "xx", "facetype": 0,"customface":"�ļ�md5", "gender":0, "birthday":19891208, "signature":"���������ڳɹ���",
                                                                "address":"�Ϻ��ж���·3261��", "phonenumber":"021-389456", "mail":"balloonwj@qq.com"}
    cmd = 1007, seq = 0, {"code": 0, "msg": "ok", "userid": 9, "username": "xxxx", "nickname": "xx", "facetype": 0,
                                                                "customface":"�ļ�md5", "gender":0, "birthday":19891208, "signature":"���������ڳɹ���",
                                                                "address":"�Ϻ��ж���·3261��", "phonenumber":"021-389456", "mail":"balloonwj@qq.com"}
**/

/**
 *  �޸�����
 **/
/*
    cmd = 1008, seq = 0, {"oldpassword": "xxx", "newpassword": "yyy"}
    cmd = 1008, seq = 0, {"code":0, "msg": "ok"}
**/

/**
 *  ����Ⱥ
 **/
/*
    cmd = 1009, seq = 0, {"groupname": "�ҵ�Ⱥ����", "type": 0}
    cmd = 1009, seq = 0, {"code":0, "msg": "ok", "groupid": 12345678, "groupname": "�ҵ�Ⱥ����"}, �û�id��Ⱥid����4�ֽ����ͣ�32λ��Ⱥid�ĸ�λ����λΪ1
**/

/**
 *  ��ȡȺ��Ա
 **/
/*
cmd = 1010, seq = 0, {"groupid": Ⱥid}
cmd = 1010, seq = 0, {"code":0, "msg": "ok", "groupid": 12345678, 
                    "members": [{"userid": 1, "username": xxxx, "nickname": yyyy, "facetype": 1, "customface": "ddd", "status": 1, "clienttype": 1},
                    {"userid": 1, "username": xxxx, "nickname": yyyy, "facetype": 1, "customface": "ddd", "status": 1, "clienttype": 1},
                    {"userid": 1, "username": xxxx, "nickname": yyyy, "facetype": 1, "customface": "ddd", "status": 1, "clienttype": 1}]}
**/

/**
 *  ����Э��
 **/
/* 
    cmd = 1100, seq = 0, data:�������� targetid(��Ϣ������)
    cmd = 1100, seq = 0, data:�������� senderid(��Ϣ������), targetid(��Ϣ������)
 **/

/**
 *  Ⱥ��Э��
 **/
/*
    cmd = 1101, seq = 0, data:�������� targetid(��Ϣ������)
    cmd = 1101, seq = 0, data:��������, {"targets": [1, 2, 3]}(��Ϣ������)
**/

