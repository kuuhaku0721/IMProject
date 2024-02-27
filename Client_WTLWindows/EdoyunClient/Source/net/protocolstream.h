#pragma once

#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <sstream>
using namespace std;
//������Э��Ĵ������࣬�ڲ��ķ�����֮��ͨѶ��ͳһ������Щ��
namespace yt
{
	enum {
		TEXT_PACKLEN_LEN	= 4,
		TEXT_PACKAGE_MAXLEN	= 0xffff,
		BINARY_PACKLEN_LEN	= 2,
		BINARY_PACKAGE_MAXLEN	= 0xffff,

		TEXT_PACKLEN_LEN_2	= 6,
		TEXT_PACKAGE_MAXLEN_2	= 0xffffff,

		BINARY_PACKLEN_LEN_2 	= 4,//4�ֽ�ͷ����
		BINARY_PACKAGE_MAXLEN_2 = 0x10000000,//����󳤶���256M,�㹻��

		CHECKSUM_LEN		= 2,
	};

	//����У���
	unsigned short checksum(const unsigned short*buffer,int size);
	bool compress_(unsigned int i, char*buf, size_t &len);
	bool uncompress_(char*buf, size_t len, unsigned int &i);

	struct ReadStreamImpl
	{
		virtual ~ReadStreamImpl() {}
		virtual const char* GetData() const = 0;
		virtual size_t GetSize() const = 0;
	};

	struct WriteStreamImpl
	{
		virtual ~WriteStreamImpl() {}
		virtual const char* GetData() const = 0;
		virtual size_t GetSize() const = 0;
	};
	class BinaryReadStream2 : public ReadStreamImpl
	{
		private:
			const char* const ptr;
			const size_t len;
			const char* cur;
			BinaryReadStream2(const BinaryReadStream2&);
			BinaryReadStream2& operator=(const BinaryReadStream2&);
		public:
			BinaryReadStream2(const char* ptr, size_t len);
			virtual const char* GetData() const;
			virtual size_t GetSize() const;
			bool IsEmpty() const;
			bool Read(string*str,size_t maxlen,size_t& outlen);
			bool Read(char* str,size_t strlen,size_t& len);
			bool Read(const char** str,size_t maxlen,size_t& outlen);
			bool Read(int &i);
			bool Read(short &i);
			bool Read(char &c);
			size_t ReadAll(char* szBuffer, size_t iLen) const;
			bool IsEnd() const;
			const char* GetCurrent() const{ return cur; }

		public:
			bool ReadLength(size_t & len);
			bool ReadLengthWithoutOffset(size_t &headlen, size_t & outlen);
	};
	class BinaryWriteStream3 : public WriteStreamImpl
	{
		public:
			BinaryWriteStream3(string*data);
			virtual const char* GetData() const;
			virtual size_t GetSize() const;
			bool Write(const char* str,size_t len);
			bool  Write(double value,bool isNULL = false);
			bool  Write(long value,bool isNULL = false);
			bool Write(int i,bool isNULL = false);
			bool Write(short i,bool isNULL = false);
			bool Write(char c,bool isNULL = false);
			size_t GetCurrentPos() const{return m_data->length();}
			void Flush();
			void Clear();
		private:
			string*m_data;
	};
}


