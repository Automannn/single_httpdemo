//
// Created by 14394 on 2020/3/9.
//

#include "Server.h"
#include <iostream>
#include <time.h>
#include "params.h"

//我们存放Html文件的目录
char HtmlDir[512];

pNode pHead = NULL;
pNode pTail = NULL;

pThread pHeadThread;
pThread pTailThread;

DWORD WINAPI AcceptThread(LPVOID lpParam)   //接收线程
{
    //创建一个监听套接字
    SOCKET sListen = WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED); //使用事件重叠的套接字
    if (sListen==INVALID_SOCKET)
    {
        printf("Create Listen Error\n");
        return -1;
    }
    //初始化本服务器的地址
    sockaddr_in LocalAddr;
    LocalAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_port = htons(uPort);
    //绑定套接字 80端口
    int Ret = bind(sListen,(sockaddr*)&LocalAddr,sizeof(LocalAddr));
    if (Ret==SOCKET_ERROR)
    {
        printf("Bind Error\n");
        return -1;
    }
    //监听
    listen(sListen,5);
    //创建一个事件
    WSAEVENT Event = WSACreateEvent();
    if (Event==WSA_INVALID_EVENT)
    {
        printf("Create WSAEVENT Error\n");
        closesocket(sListen);
        CloseHandle(Event);     //创建事件失败 关闭套接字 关闭事件
        return -1;
    }
    //将我们的监听套接字与我们的事件进行关联属性为Accept
    WSAEventSelect(sListen,Event,FD_ACCEPT);
    WSANETWORKEVENTS NetWorkEvent;
    sockaddr_in ClientAddr;
    int nLen = sizeof(ClientAddr);
    DWORD dwIndex = 0;
    while (1)
    {
        //这里会阻塞等待信号
        dwIndex = WSAWaitForMultipleEvents(1,&Event,FALSE,WSA_INFINITE,FALSE);
        dwIndex = dwIndex - WAIT_OBJECT_0;
        if (dwIndex==WSA_WAIT_TIMEOUT||dwIndex==WSA_WAIT_FAILED)
        {
            continue;
        }
        //如果有真正的事件我们就进行判断
        WSAEnumNetworkEvents(sListen,Event,&NetWorkEvent);
        ResetEvent(&Event);   //
        if (NetWorkEvent.lNetworkEvents == FD_ACCEPT)
        {
            if (NetWorkEvent.iErrorCode[FD_ACCEPT_BIT]==0)
            {
                //我们要为新的连接进行接受并申请内存存入链表中
                SOCKET sClient = WSAAccept(sListen, (sockaddr*)&ClientAddr, &nLen, NULL, NULL);
                if (sClient==INVALID_SOCKET)
                {
                    continue;
                }
                else
                {
                    //如果接收成功我们要把用户的所有信息存放到链表中
                    if (!AddClientList(sClient,ClientAddr))
                    {
                        continue;
                    }
                }
            }
        }
    }
    return 0;
}

DWORD WINAPI WorkerThread(LPVOID lpParam){
    WSAEVENT Event = WSACreateEvent(); //该事件是与通信套接字关联以判断事件的种类
    WSANETWORKEVENTS NetWorkEvent;

    while (1){
        pNode _ptr_tmp = pHead;
        while (_ptr_tmp){
            //将事件 与当前 的套接字 进行关联
            WSAEventSelect(_ptr_tmp->s, Event, FD_READ | FD_WRITE | FD_CLOSE); //关联事件和套接字
            DWORD dwIndex = 0;
            dwIndex = WSAWaitForMultipleEvents(1,&Event,FALSE,100,FALSE);
            dwIndex = dwIndex - WAIT_OBJECT_0;
            if (dwIndex==WSA_WAIT_TIMEOUT||dwIndex==WSA_WAIT_FAILED)
            {
                //向后遍历
                _ptr_tmp =_ptr_tmp->pNext;
            }
            // 分析什么网络事件产生
            WSAEnumNetworkEvents(_ptr_tmp->s,Event,&NetWorkEvent);
            //其他情况
            if(!NetWorkEvent.lNetworkEvents)
            {
                //向后遍历
                _ptr_tmp =_ptr_tmp->pNext;
            }

            if (NetWorkEvent.lNetworkEvents & FD_READ)
            {
                //开辟客户端线程用于通信
                CreateThread(NULL,0,ClientThread,(LPVOID)_ptr_tmp,0, nullptr);
            }


            if(NetWorkEvent.lNetworkEvents & FD_CLOSE)
            {
                //在这里我没有处理，我们要将内存进行释放否则内存泄露
                //todo: 需要释放的内存包括：socket句柄，thread句柄，以及thread句柄中所动态申请的资源
                //向后遍历
                _ptr_tmp =_ptr_tmp->pNext;
            }
        }
    }


}


DWORD WINAPI ClientThread(LPVOID lpParam)
{
    //我们将每个用户的信息以参数的形式传入到该线程
    pNode pTemp = (pNode)lpParam;
    SOCKET sClient = pTemp->s; //这是通信套接字

    char szRequest[1024]={0}; //请求报文
    char szResponse[1024]={0}; //响应报文
    BOOL bKeepAlive = FALSE; //是否持续连接
    DWORD NumberOfBytesRecvd;
    WSABUF Buffers;
    DWORD dwBufferCount = 1;
    char szBuffer[MAX_BUFFER];
    DWORD Flags = 0;
    Buffers.buf = szBuffer;
    Buffers.len = MAX_BUFFER;
    DWORD Ret = WSARecv(sClient,&Buffers,dwBufferCount,&NumberOfBytesRecvd,&Flags,NULL,NULL);
    //我们在这里要检测是否得到的完整请求
    memcpy(szRequest,szBuffer,NumberOfBytesRecvd);
    if (!IoComplete(szRequest)) //校验数据包
     {
      //todo: 检验读取时数据包是否完成
     }
     if (!ParseRequest(szRequest, szResponse, bKeepAlive)) //分析数据包
     {
         //我在这里就进行了简单的处理
       //todo: 处理 响应失败
      }
      DWORD NumberOfBytesSent = 0;
      DWORD dwBytesSent = 0;
      //发送响应到客户端
      do{
         Buffers.len = (strlen(szResponse) - dwBytesSent) >= SENDBLOCK ? SENDBLOCK : strlen(szResponse) - dwBytesSent;
                Buffers.buf = (char*)((long long)szResponse + dwBytesSent);
                Ret = WSASend(
                        sClient,
                        &Buffers,
                        1,
                        &NumberOfBytesSent,
                        0,
                        0,
                        NULL);
                if(SOCKET_ERROR != Ret)
                    dwBytesSent += NumberOfBytesSent;
            }
      while((dwBytesSent < strlen(szResponse)) && SOCKET_ERROR != Ret);
    return 0;
}

//校验数据包
bool IoComplete(char* szRequest)
{
    char* pTemp = NULL;   //定义临时空指针
    int nLen = strlen(szRequest); //请求数据包长度
    pTemp = szRequest;
    pTemp = pTemp+nLen-4; //定位指针
    if (strcmp(pTemp,"\r\n\r\n")==0)   //校验请求头部行末尾的回车控制符和换行符以及空行
    {
        return true;
    }
    return false;
}

bool AddClientList(SOCKET s,sockaddr_in addr)
{
    pNode pTemp = (pNode)malloc(sizeof(Node));
    HANDLE hThread = NULL;
    DWORD ThreadID = 0;

    //todo:这里需要加锁  用于保证线程安全
    if (pTemp==NULL)
    {
        printf("No Memory\n");
        return false;
    }
    else
    {
        pTemp->s = s;
        pTemp->Addr = addr;
        pTemp->pNext = NULL;
        if (pHead==NULL)
        {
            pHead = pTail = pTemp;
        }
        else
        {
            pTail->pNext = pTemp;
            pTail = pTail->pNext;
        }
    return true;
}
}

//分析数据包
bool ParseRequest(char* szRequest, char* szResponse, BOOL &bKeepAlive)
{
    char* p = NULL;
    p = szRequest;
    int n = 0;
    char* pTemp = strstr(p," "); //判断字符串str2是否是str1的子串。如果是，则该函数返回str2在str1中首次出现的地址；否则，返回NULL。
    n = pTemp - p;    //指针长度
// pTemp = pTemp + n - 1; //将我们的指针下移
    //定义一个临时的缓冲区来存放我们
    char szMode[10]={0};
    char szFileName[10]={0};
    memcpy(szMode,p,n);   //将请求方法拷贝到szMode数组中
    if (strcmp(szMode,"GET")==0)  //一定要将Get写成大写
    {
        //获取文件名
        pTemp = strstr(pTemp," ");
        pTemp = pTemp + 1;   //只有调试的时候才能发现这里的秘密
        memcpy(szFileName,pTemp,1);
        if (strcmp(szFileName,"/")==0)
        {
            strcpy(szFileName,FileName);
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    // 分析链接类型
    pTemp = strstr(szRequest,"\nConnection: Keep-Alive");  //协议版本
    n = pTemp - p;
    if (p>(char *)0)
    {
        bKeepAlive = TRUE;
    }
    else  //这里的设置是为了Proxy程序的运行
    {
        bKeepAlive = TRUE;
    }
    //定义一个回显头
    char pResponseHeader[512]={0};
    char szStatusCode[20]={0};
    char szContentType[20]={0};
    strcpy(szStatusCode,"200 OK");
    strcpy(szContentType,"text/html");
    char szDT[128];
    struct tm *newtime;
    time_t ltime;
    time(&ltime);
    newtime = gmtime(&ltime);
    strftime(szDT, 128,"%a, %d %b %Y %H:%M:%S GMT", newtime);
    //读取文件
    //定义一个文件流指针
    FILE* fp = fopen(HtmlDir,"rb");
    fpos_t lengthActual = 0;
    int length = 0;
    char* BufferTemp = NULL;
    if (fp!=NULL)
    {
        // 获得文件大小
        fseek(fp, 0, SEEK_END);
        fgetpos(fp, &lengthActual);
        fseek(fp, 0, SEEK_SET);
        //计算出文件的大小后我们进行分配内存
        BufferTemp = (char*)malloc(sizeof(char)*((int)lengthActual));
        length = fread(BufferTemp,1,(int)lengthActual,fp);
        fclose(fp);
        // 返回响应
        sprintf(pResponseHeader, "HTTP/1.0 %s\r\nDate: %s\r\nServer: %s\r\nAccept-Ranges: bytes\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: %s\r\n\r\n",
                szStatusCode, szDT, SERVERNAME, length, bKeepAlive ? "Keep-Alive" : "close", szContentType);   //响应报文
    }
        //如果我们的文件没有找到我们将引导用户到另外的错误页面
    else
    {
        printf("没找到文件\n");
    }
    strcpy(szResponse,pResponseHeader);
    strcat(szResponse,BufferTemp);
    free(BufferTemp);
    BufferTemp = NULL;
    return true;
}

bool InitSocket()
{
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2),&wsadata)==0)    //使用Socket前必须调用 参数 作用 返回值
    {
        return true;
    }
    return false;
}