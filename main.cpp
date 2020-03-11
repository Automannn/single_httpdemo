
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
    GetCurrentDirectory(512,HtmlDir);
    strcat(HtmlDir,"\\..\\html\\"); // 寻找 html 所在目录
    strcat(HtmlDir,FileName);
    std::cout<<"the path is:"<<HtmlDir<<std::endl;
    //启动一个接收线程
    HANDLE hAcceptThread = CreateThread(NULL,0,AcceptThread,NULL,0,NULL);
    //启动工作者线程
    CreateThread(NULL,0,WorkerThread,NULL,0,NULL);
    WaitForSingleObject(hAcceptThread,INFINITE);
}















