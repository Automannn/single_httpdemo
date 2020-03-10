//
// Created by 14394 on 2020/3/9.
//
#include <Winsock2.h>
#include <windows.h>

#ifndef SINGLE_HTTPDEMO_SERVER_H
#define SINGLE_HTTPDEMO_SERVER_H

//标识客户端的节点 链表
typedef struct _NODE_
{
    SOCKET s;
    sockaddr_in Addr;
    _NODE_* pNext;

}Node,*pNode;

//标识线程的节点，多线程处理多个客户端的连接
typedef struct _THREAD_
{
    DWORD ThreadID;
    HANDLE hThread;
    _THREAD_* pNext;
}Thread,*pThread;

//socket链表的 头节点 及 尾节点
extern pNode pHead;
extern pNode pTail;

//thread链表的 头节点 及 尾节点
extern pThread pHeadThread;
extern pThread pTailThread;

extern char HtmlDir[512];

bool InitSocket();//线程函数

DWORD WINAPI AcceptThread(LPVOID lpParam); //接收线程
DWORD WINAPI ClientThread(LPVOID lpParam); //客户端线程

bool IoComplete(char* szRequest);     //数据包的校验函数

bool AddClientList(SOCKET s,sockaddr_in addr);  //添加客户节点
bool AddThreadList(HANDLE hThread,DWORD ThreadID); //添加线程节点

bool ParseRequest(char* szRequest, char* szResponse, BOOL &bKeepAlive);//解析请求

#endif //SINGLE_HTTPDEMO_SERVER_H
