
#pragma once

#include "types.h"
#include <stdio.h>

#define WSYSPLUS_API
#define NOVTABLE

///////////////////////////////////////////////////////////////////////////////////////////////////
//�����ڴ�:lsizeΪ��Ҫ�ĳ���,msize��Ϊ��ʱ�洢ʵ�ʷ���ĳ���(һ�㶼������һЩ,����Խ��)
//�ɹ������ڴ��ַ,ʧ�ܷ���NULL
//ע��:����ɹ����ڴ������wsysplus_release�ͷ�
char* wsysplus_malloc(XINT32 lsize, XINT32* msize = NULL);
void  wsysplus_count(XINT32 lsize);

//�ͷ��ڴ�(lpDataΪ�ڴ��ַ)
//�ͷ����,�Զ�����ַָ����NULL
//lpDataΪNULLʱ,��������,�����쳣
void  wsysplus_release(char*&lpData);

///////////////////////////////////////////////////////////////////////////////////////////////////
class WSYSPLUS_API  NOVTABLE  wsysplus_memory
{
public:
	//�����ڴ�,�ɹ����ص�ַ,ʧ�ܷ���NULL
	//����ѷ���ؼ�����,����֮;����,��������
	//�ɹ�����ʱ,��ַ�ռ��Ѿ�ȫ����0
	LPSTR	GetBuf(XINT32 lsize);

	//Ϊ��ʱ���᷵��NULL,����0�������ַ���
	LPSTR	SafeBuffer(void);

	//��չ�����ڴ浽�³���,�ɹ������׵�ַ,ʧ�ܷ���NULL
	//�ɹ�����ʱ,�´��ԭ��ַ���ݵ��¿ռ�
	LPSTR	Extern(XINT32 maxSize);

	//�������ݵ�������(���Զ��ж��ڴ泤��),�ɹ������׵�ַ,ʧ�ܷ���NULL
	LPSTR	Copy(LPCVOID lpData, XINT32 lsize);

	//׷�����ݵ�������(���Զ��ж��ڴ泤��),�ɹ������׵�ַ,ʧ�ܷ���NULL
	LPSTR	Append(LPCVOID lpData, XINT32 lsize);

	//����һ������,����������
	void	Attach(wsysplus_memory*pMemory);

	//�ͷ��ڴ�
	void	Release(void);
	//�ÿ�,��Ӧ�Ľṹ�Ѿ����ⲿŲ��
	void	Detach(void);

	//����������:�����ڴ��׵�ַ
	inline operator LPCSTR(void) const { return bufData; }

	//Copy��������������
	const wsysplus_memory& operator=(LPCSTR lpString);
	const wsysplus_memory& operator=(wsysplus_memory&iMemory);

	//Append��������������
	const wsysplus_memory& operator+=(LPCSTR lpString);

	wsysplus_memory(LPCSTR lpString = NULL);
	wsysplus_memory(wsysplus_memory&iMemory);
	~wsysplus_memory(void);

	// UMYPRIVATE:
	XINT32	sizeData;		//��ʹ�����ݳ���
	XINT32	sizeMalloc;		//ʵ�ʷ������ݳ���
	LPSTR	bufData;		//�����׵�ַ
};

//���������:
//1.��������,һ���ͷ�
//2.���������Զ��ͷ�,�����˹���Ԥ
//3.������С��Χ�����ڴ����
class WSYSPLUS_API  NOVTABLE wsysplus_vector
{
	typedef	struct tagItem
	{
		tagItem	*pNext;
		char	szData[1];
	}ITEM, *PITEM;
public:
	//����
	LPSTR	Malloc(XINT32 dataSize, XINT32*msize = NULL);
	//һ���ͷ������ѷ����ڴ�
	void	ReleaseAll(void);
	//ת��
	void	Attach(wsysplus_vector*pVector);

	wsysplus_vector(XINT32 nType = 0);
	~wsysplus_vector(void);
private:
	PITEM	_items;
	XINT32	_nType;	//0:�ڴ�ط���;1:ֱ�ӷ���
};

//ʹ����ͨ�ڴ�������չ�ڴ�̫�˷���,����ڴ涶��
//��������һ��������,һ��Ԥ������ռ�
class WSYSPLUS_API  NOVTABLE  wsysplus_array
{
public:
	//��β׷��numData������
	BOOL	Append(LPCVOID lpData, XDWORD numData = 1);
	XDWORD	GetNext(LPVOID lpData, BOOL blFirst = FALSE, XDWORD maxData = 1);

	void	Release(void);
	XDWORD	GetTotal(void);

	//perSize�����鵥λ���
	wsysplus_array(XDWORD perSize);
	~wsysplus_array(void);
protected:
	typedef struct tagBufArray
	{
		tagBufArray*pNext;
		char	data[4000];
	}BUF_ARRAY, *PBUF_ARRAY;

	PBUF_ARRAY	_ptrData;	//������ҳ��ַ
	PBUF_ARRAY	_ptrEnd;	//����βҳ��ַ(������,ֱ��׷��,��!)
	PBUF_ARRAY	_ptrRead;	//��ǰ��ȡ��ҳ���ַ
	XDWORD	_perSize;	//���鵥λ���
	XDWORD	_off;		//��ǰ�����ҳ��ƫ��
	XDWORD	_rdoff;		//��ǰ��ȡƫ��
	XDWORD	_Total;		//��ʹ���ܸ���
};