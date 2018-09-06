#pragma once
#include <ev.h>
#include <netinet/in.h>
#include "net.pb.h"
#include "common.h"
namespace GUC
{
	/*
	 *	根据消息id 处理消息
	 *
	 * */
	class pack_mgr
	{
		public:
			pack_mgr();
			~pack_mgr();
			// 定义消息处理函数指针
			typedef bool (pack_mgr::*PH)(int ,byte* ,int);
			// 分发消息处理函数
			bool process(int id,byte *buf,int len);
		private:
			// test_guc_people消息处理
			bool test_guc_people_handle(int id,byte* buf,int len);
		private:
			// 消息id-消息处理函数映射
			std::map<PB::c2sid,PH> handles_map;	
			// 定义迭代器别名
			typedef std::map<PB::c2sid,PH>::iterator def_itr;
	};

	
	/*
 *	客户端socket维护，tcp读写buff维护
 * */
	class session
	{
		public:
			// 处理tcp缓存中的数据，交给pack_mgr处理
			int process_packet();
			// 读取内核tcp数据，放入用户缓冲区
			int read_stream();
			// 关闭socket,释放资源
			void close();
			session();
			~session();
			// 初始化libev数据 
			void init(struct ev_io* w_client,ev_cb read,int fd,int revents);
			// 开始libev监听
			void start(struct ev_loop* loop,struct ev_io* w_client);
		private:
			// 消息处理类
			pack_mgr package_handler;	
			// 用户缓冲区,循环buff
			byte recvbuff[BUFFER_SIZE];
			// 循环buffer尾指针
			int tail;
			// 循环buffer头指针
			int head;
		public:
			// libev io watcher
			struct ev_io* w_client;
	};


	/*
 *  服务器tcp socket 实现
 *  通过libev库，监听，接受，发送数据
 * */

	class socket_tcp_s
	{
		public:
			socket_tcp_s();
			~socket_tcp_s();
			// 初始化,socket和libev
			int init(short port);
			// 开始监听连接，处理消息
			void run();
			// 通过socket描述符查找session数据
			session* find_session(int fd);
			// accept后加入session维护
			bool add_session(int fd,session* se);
			// 关闭socket后移除session
			bool del_session(int fd);
			// 是否有指定描述符session
			bool has_session(int fd);
			// 清理socket文件描述符对应的libev资源
			int freelibev(int fd);
			// 关闭
			void close();
		private:
			// 服务器socket文件描述符
			int sd;
			//一个io watcher,用于接受客户端连接
			struct ev_io socket_accept;
			//一个timer watcher
			struct ev_timer timeout_w;
			// socket文件描述符和对应的session
			std::map<int,session*> _session_map;
			// 定义迭代器别名
			typedef std::map<int,session*>::iterator def_itr;
		public:
			// libev loop
			struct ev_loop *loop;
	};

}
