
#include <iostream>
#include "main/Server.h"
#include "main/params.h"
using namespace std;

int main()
{
    if (!InitSocket())
    {
        printf("InitSocket Error\n");
        return -1;
    }
    //获取当前路径长度
//    unsigned long size=GetCurrentDirectory(0,NULL);
//    char path[size];
    GetCurrentDirectory(512,HtmlDir);
    strcat(HtmlDir,"\\..\\html\\"); // 寻找 html 所在目录
    strcat(HtmlDir,FileName);

    std::cout<<"the path is:"<<HtmlDir<<std::endl;

    //启动一个接收线程
    HANDLE hAcceptThread = CreateThread(NULL,0,AcceptThread,NULL,0,NULL);

    //在这里我们使用事件模型来实现我们的Web服务器
    //创建一个事件
    WaitForSingleObject(hAcceptThread,INFINITE);
}















