#include <ev.h>
#include <stdio.h>
#include <netinet/in.h>
#include<stdlib.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "net.pb.h"
#include "common.h"

#define PORT 8888 
#define MAX_ALLOWED_CLIENT 1024
#define HEAD_LEN (8)
#define BUFFER_SIZE GUC_BUFFER_SIZE 
#define RBLEN(t,h) (BUFFER_SIZE-(BLEN(t,h))-1) 
#define BLEN(t,h) ((t-h+BUFFER_SIZE)%BUFFER_SIZE)
#define ISFULL(t,h) (((t+1)%BUFFER_SIZE)==h)
#define INDEX(l) ((l)%BUFFER_SIZE)
#define IN(t,n) (t=((t+n)%BUFFER_SIZE))
#define OUT(h,n) (h=((h+n)%BUFFER_SIZE))
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void timer_beat(struct ev_loop *loop, struct ev_timer *watcher, int revents);
typedef  char   byte;
typedef  void (*ev_cb)(struct ev_loop* , struct ev_io*,int);
long cnt = 0;


namespace GUC
{
	class pack_mgr
	{
		public:
			pack_mgr();
			~pack_mgr();
 
			typedef bool (pack_mgr::*PH)(int ,byte* ,int);
			bool process(int id,byte *buf,int len);
		private:
			bool test_guc_people_handle(int id,byte* buf,int len);
		private:
			std::map<PB::c2sid,PH> handles_map;	
			typedef std::map<PB::c2sid,PH>::iterator def_itr;
	};
	bool pack_mgr::process(int id,byte* buf,int len)
	{
		std::cout<<"handle id:"<<id<<std::endl;
		def_itr itr = handles_map.find((PB::c2sid)id);
		if(itr != handles_map.end())
			(this->*handles_map[(PB::c2sid)id])(id,buf,len);
		else 
			std::cout<<"handle not find id:"<<id<<std::endl;
		
	}
	pack_mgr::pack_mgr()
 	{
		handles_map[PB::guc_test_people] = &pack_mgr::test_guc_people_handle;
	}
	pack_mgr::~pack_mgr()
	{
	}
	bool pack_mgr::test_guc_people_handle(int id,byte* buf,int len)
	{
		std::string bodybuf;
		bodybuf.append(buf,len);
		PB::people p;
		p.ParseFromString(bodybuf);
		if(cnt%10000==0)
		{

			std::cout<<"recv process info:" 
				<< "id:"<< p.id()
				<< " name:"<< p.name().data() 
				<< " power:"<< p.power()
				<<" len:"<<len
				<<std::endl;
			for(int i=0;i<p.skills_size();++i)
			{
				const PB::people::skill &s = p.skills(i);
				std::cout<<"skillId:"<<s.skillid()
					<<"skillname:"<<s.skillname()
					<<std::endl;
			}
		}
		return true;
	}

	class session
	{
		public:
			int process_packet();
			int read_stream();
			void close();
			session();
			~session();
			void init(struct ev_io* w_client,ev_cb read,int fd,int revents);
			void start(struct ev_loop* loop,struct ev_io* w_client);
		private:
			pack_mgr package_handler;	
			byte recvbuff[BUFFER_SIZE];
			int tail;
			int head;
		public:
			struct ev_io* w_client;
	};

	void session::init(struct ev_io* w_client,ev_cb read,int fd,int revents)
	{
		tail =0;
		head = 0;
		memset(recvbuff,0,BUFFER_SIZE);
		this->w_client = w_client;
		ev_io_init(w_client,read,fd,revents);
	}
	void session::start(struct ev_loop* loop,struct ev_io* w_client)
	{
		ev_io_start(loop,w_client);
	}
	session::session()
	{

	}
	void session::close()
	{
		tail = 0;
		head = 0;
		if(NULL != w_client)
		{
			::close(w_client->fd);
			free(w_client);
			w_client=NULL;
		}
	}
	int session::read_stream()
	{
		//正常的recv
		ssize_t read=0;
		int free_size = process_packet();
		if(free_size<0)
		{
			std::cout<<"data_len > BUFFER_SIZE data_len:"<<std::endl;
			return 0;
		}

		int read_len_1 =  BUFFER_SIZE-tail-1;
		if(tail==head)
		{
			read_len_1 = BUFFER_SIZE-tail-1;
			if(tail!=0)
				read_len_1+=1;
		}
		else if(tail>head)
		{
			read_len_1 = BUFFER_SIZE-tail;
			if(head == 0)
				read_len_1 -= 1;
		}
		else if(tail<head)
		{
			read_len_1 = head - tail - 1;
		}

		int read_len_2 = free_size-read_len_1;
		//printf("len_1:%d,len_2:%d,free:%d\n",read_len_1,read_len_2,free_size);
		if(read_len_1>0)
		{
			read = recv(w_client->fd,recvbuff+tail,read_len_1,0);
			//printf("read2:%d\n",read);
			if(read>0 && read_len_1>0)
			{
				IN(tail,read);
			}
		}
		if(read_len_2>0 && read==read_len_1)
		{
			read = recv(w_client->fd,recvbuff,read_len_2,0);
			//printf("read3:%d\n",read);
			if(read>0 && read_len_2>0)
			{
				IN(tail,read);
			}
		}
		if(read < 0)
		{
			std::cout<<"read error"<<std::endl;
			return read;
		}
		if(read == 0)
		{
			std::cout<<"client disconnected."<<std::endl;
			return read;
		}

		// process
		process_packet();

		return read;

	}
	int session::process_packet()
	{		
		byte buf[BUFFER_SIZE-1]={0};
		// 数据为空
		if (head == tail)
			return RBLEN(tail,head);

		// 小于包頭
		if(BLEN(tail,head)< HEAD_LEN)
			return RBLEN(tail,head);

		GUC::head h;
		char buff_h[HEAD_LEN] = {0};
		for(int i=0;i<HEAD_LEN;++i)
		{
			buff_h[i] = recvbuff[INDEX(head+i)];
		}
		memcpy(&h,buff_h,HEAD_LEN); 

		int len = h.data_len;
		//std::cout<<"headlen:"<<len<<std::endl;
		//std::cout<<"headId:"<<h.id<<std::endl;
		if(len>BUFFER_SIZE)
			return -1;

		if(BLEN(tail,head)< len+HEAD_LEN)
			return RBLEN(tail,head);

		int i = 0,j=len;
		OUT(head,HEAD_LEN);
		do 
		{
			if(head+len<=BUFFER_SIZE) 
			{
				memcpy(buf,recvbuff+head,len);
			}
			else 
			{
				memcpy(buf,recvbuff+head,BUFFER_SIZE-head);	
				memcpy(buf+BUFFER_SIZE-head,recvbuff,len+head-BUFFER_SIZE);
			}
		}while(0);
		OUT(head,len);
		// process
		package_handler.process(h.id,buf,len);

		return RBLEN(tail,head);
	}  

	class socket_tcp_s
	{
		public:
			socket_tcp_s();
			~socket_tcp_s();
			int init(short port);
			void run();
			session* find_session(int fd);
			bool add_session(int fd,session* se);
			bool del_session(int fd);
			bool has_session(int fd);
			int freelibev(int fd);
		private:
			int sd;
			//一个io watcher
			struct ev_io socket_accept;
			//一个timer watcher
			struct ev_timer timeout_w;
			std::map<int,session*> _session_map;
			typedef std::map<int,session*>::iterator def_itr;
		public:
			struct ev_loop *loop;
	};
	int  socket_tcp_s::freelibev(int fd)
	{
		session* se = find_session(fd);
		if(NULL == se)
			return -1;
		ev_io_stop(loop, se->w_client);
		se->close();
		_session_map.erase(fd);
		return 0;
	}
	bool socket_tcp_s::has_session(int fd)
	{
		session* fse = find_session(fd); 
		if(NULL==fse)
			return false;
		return true;
	}
	bool socket_tcp_s::del_session(int fd)
	{
		session* fse = find_session(fd);
		if(NULL==fse)
			return false;
		_session_map.erase(fd);
		return true;
	}
	bool socket_tcp_s::add_session(int fd,session*se)
	{
		session* fse = find_session(fd);
		if(fse!=NULL)
			return false;
		_session_map[fd] = se;
		return true;
	}
	session* socket_tcp_s::find_session(int fd)
	{
		def_itr itr = _session_map.find(fd);
		if(itr != _session_map.end())
			return itr->second;
		return NULL;
	}

	socket_tcp_s::socket_tcp_s()
	{
	}

	socket_tcp_s::~socket_tcp_s()
	{
		for(std::map<int,session*>::iterator itr=_session_map.begin();itr!=_session_map.end();itr++)
		{
			// close 
			freelibev(itr->first);
		}
		_session_map.erase(_session_map.begin(),_session_map.end());
	}

	void socket_tcp_s::run()
	{
		ev_run(loop, 0);
	}

	int socket_tcp_s::init(short port)
	{
		loop = ev_default_loop(0);
		struct sockaddr_in addr;
		int addr_len = sizeof(addr);

		//创建socket连接
		sd = socket(PF_INET, SOCK_STREAM, 0);
		if(sd < 0)
		{
			std::cout<<"socket error"<<std::endl;
			return -1;
		}
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;
		//正常bind
		if(bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
		{
			std::cout<<"bind error"<<std::endl;
			return -1;
		}

		//正常listen
		if(listen(sd, SOMAXCONN) < 0)
		{
			std::cout<<"listen error"<<std::endl;
			return -1;
		}

		//设置fd可重用
		int bReuseaddr=1;
		if(setsockopt(sd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(bReuseaddr)) != 0)
		{
			std::cout<<"setsockopt error in reuseaddr sd:"<<sd<<std::endl;
			return 0;
		}

		//初始化io watcher，用于监听fd
		ev_io_init(&socket_accept, accept_cb, sd, EV_READ);
		ev_io_start(loop, &socket_accept);

		//可以向远端发送心跳包
		//ev_timer_init(&timeout_w, timer_beat, 1., 0.);
		//ev_timer_start(loop, &timeout_w);

		return 0;
	}

}

GUC::socket_tcp_s *service;
int main()
{
	service = new GUC::socket_tcp_s();
	if(service->init(8888) != 0)
		return -1;
	service->run();
	return 0;
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	if(EV_ERROR & revents)
	{
		std::cout<<"error event in read"<<std::endl;
		return ;
	}

	int readcnt = 0;
	// get session by watcher->fd
	GUC::session* se = service->find_session(watcher->fd);
	if(NULL != se)
	{
		readcnt = se->read_stream();
	}
	if(0 >= readcnt)
		service->freelibev(watcher->fd);
}

void timer_beat(struct ev_loop *loop, struct ev_timer *watcher, int revents)
{
	float timeout = 2.0;
	//这里可以发送心跳包，也可以什么都不做
	std::cout<<"send beat per:"<<timeout<<std::endl;    

	if(EV_ERROR & revents)
	{
		std::cout<<"error event in timer_beat"<<std::endl;
		return ;
	}

	ev_timer_set(watcher, timeout,0.);
	ev_timer_start(loop, watcher);
	return;
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	/*
	   如果有链接，则继续监听fd；
	   */
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_sd;

	if(EV_ERROR & revents)
	{
		std::cout<<"error event in accept"<<std::endl;
		return ;
	}

	//获取与客户端相连的fd
	client_sd = accept(watcher->fd, (struct sockaddr*)&client_addr, &client_len);
	if(client_sd < 0)
	{
		std::cout<<"accept error"<<std::endl;
		return;
	}
	//如果连接数超出指定范围，则关闭连接
	if( client_sd > MAX_ALLOWED_CLIENT)
	{
		std::cout<<"fd too large:"<<client_sd<<std::endl;
		close(client_sd);
		return ;
	}

	if(service->has_session(client_sd))
	{
		std::cout<<"client_sd not NULL fd is :"<< client_sd<<std::endl;
		return;
	}


	std::cout<<"client connected\n";

	//创建客户端的io watcher
	struct ev_io *w_client = (struct ev_io*) malloc(sizeof(struct ev_io));

	if(w_client == NULL)
	{
		std::cout<<"malloc error in accept_cb\n";
		//freelibev(loop,client_sd);	
		close(client_sd);
		return ;
	}
	GUC::session* s = new GUC::session();
	s->init(w_client,read_cb,client_sd,EV_READ);
	s->start(service->loop,w_client);

	//监听新的fd
	//ev_io_init(w_client, read_cb, client_sd, EV_READ);
	//ev_io_start(loop, w_client);

	service->add_session(client_sd,s);
}
