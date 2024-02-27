

#pragma once
#include <stdint.h>
#include <string>
#include <list>
#include <mutex>
#include <set>

using namespace std;

#define GROUPID_BOUBDARY   0x0FFFFFFF 

//�û�����Ⱥ
struct User
{
    int32_t        userid;      //0x0FFFFFFF������Ⱥ�ţ���������ͨ�û�
    string         username;    //Ⱥ�˻���usernameҲ��Ⱥ��userid���ַ�����ʽ
    string         password;
    string         nickname;    //Ⱥ�˺�ΪȺ����
    int32_t        facetype;
    string         customface;
    string         customfacefmt;//�Զ���ͷ���ʽ
    int32_t        gender;
    int32_t        birthday;
    string         signature;
    string         address;
    string         phonenumber;
    string         mail;
    int32_t        ownerid;        //����Ⱥ�˺ţ�ΪȺ��userid
    set<int32_t>   friends;        //Ϊ�˱����ظ�
};

class UserManager final
{
public:
    UserManager();
    ~UserManager();

    bool Init(const char* dbServer, const char* dbUserName, const char* dbPassword, const char* dbName);

    UserManager(const UserManager& rhs) = delete;
    UserManager& operator=(const UserManager& rhs) = delete;

    bool AddUser(User& u);
    bool MakeFriendRelationship(int32_t smallUserid, int32_t greaterUserid);
    bool ReleaseFriendRelationship(int32_t smallUserid, int32_t greaterUserid);
    bool AddFriendToUser(int32_t userid, int32_t friendid);
    bool DeleteFriendToUser(int32_t userid, int32_t friendid);
    bool UpdateUserInfo(int32_t userid, const User& newuserinfo);
    bool ModifyUserPassword(int32_t userid, const std::string& newpassword);

    bool AddGroup(const char* groupname, int32_t ownerid, int32_t& groupid);

    //������Ϣ���
    bool SaveChatMsgToDb(int32_t senderid, int32_t targetid, const std::string& chatmsg);

    //TODO: ���û�Խ��Խ�࣬������Խ��Խ���ʱ�����ϵ�еĺ���Ч�ʸ���
    bool GetUserInfoByUsername(const std::string& username, User& u);
    bool GetUserInfoByUserId(int32_t userid, User& u);
    bool GetFriendInfoByUserId(int32_t userid, std::list<User>& friends);

private:
    bool LoadUsersFromDb();
    bool LoadRelationhipFromDb(int32_t userid, std::set<int32_t>& r);

private:
    int                 m_baseUserId{ 0 };        //m_baseUserId, ȡ���ݿ�����userid���ֵ�������û�����������ϵ���
    int                 m_baseGroupId{0x0FFFFFFF};
    list<User>          m_allCachedUsers;
    mutex               m_mutex;

    string              m_strDbServer;
    string              m_strDbUserName;
    string              m_strDbPassword;
    string              m_strDbName;
};