/*
muduo网络库
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

epoll+线程池
把网络I/O代码和业务代码区分开
                用户的连接和断开
                用户的可读写事件
*/
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;   //参数占位符


/*基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop时间循环的指针
3. 明确TcpServer的构造函数参数，确定构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的工作线程个数
*/
class ChatServer{
public:
    ChatServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const string& nameArg)
                :_server(loop,listenAddr,nameArg),_loop(loop)
            {
                //给服务器注册用户连接的创建和断开回调
                _server.setConnectionCallback(std::bind(&ChatServer::onConnnection,this,_1));
                
                //给服务器注册用户读写事件回调
                _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

                //设置服务器端的线程数量 该方法设置的是工作线程的数量
                _server.setThreadNum(3);    //一共3+1，工作线程+主线程，四个线程
            }
    
    //开启事件循环
    void start(){
        _server.start();
    }
private:
    //负责处理用户的连接创建和断开 epoll listenfd accept
    void onConnnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort()<<" -> "<<
            conn->localAddress().toIpPort()<<" status:online"<<endl; //返回连接的ip地址
        }else{
            cout<<conn->peerAddress().toIpPort()<<" -> "<<
            conn->localAddress().toIpPort()<<" status:offline"<<endl; //返回连接的ip地址
            conn->shutdown();   //释放连接close(fd)
            //_loop->quit();
        }
    }

    //负责用户处理读写事件
    void onMessage(const TcpConnectionPtr& conn,    //连接
                            Buffer* buffer,    //缓冲区
                            Timestamp time)  //接收数据的时间信息
    {
        string buf=buffer->retrieveAllAsString();//转化为字符串
        cout<<"reccv data: "<<buf<<" time: "<<time.toString()<<endl;
        conn->send(buf);//返回接收到的信息
    }

    //muduo::net::TcpServer 
    TcpServer _server;  //#1
    EventLoop* _loop;   //#2 epoll
};

int main(){
    EventLoop loop;//epoll;
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"EchoServer");

    server.start(); //listenfd epoll_ctl=>epoll
    loop.loop();    //epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等

    return 0;
}