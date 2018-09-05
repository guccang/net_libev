#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include "net.pb.h"
#include "common.h"
#include <stdlib.h>
#include <time.h>

#define SKILL_COUNT (5)
namespace GUC
{
	class socket_tcp
	{
		public:
			int connect(std::string ip,short port);
			void close();
			bool init();
			int sock_client();
			socket_tcp();
			~socket_tcp();

		private:
			int _sock;
	};

	socket_tcp::socket_tcp()	
	{
		_sock=0;
	}
	socket_tcp::~socket_tcp()
	{
	}


	int socket_tcp::sock_client()
	{
		return _sock;
	}

	bool socket_tcp::init()
	{
		_sock= socket(AF_INET, SOCK_STREAM, 0);
		return _sock>0;
	}

	int socket_tcp::connect(std::string ip,short port)
	{
		struct sockaddr_in addrSrv;
		addrSrv.sin_addr.s_addr = inet_addr(ip.c_str());
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);
		int ret = ::connect(_sock, ( const struct sockaddr *)&addrSrv, sizeof(struct sockaddr_in));
		return ret;
	}

	void socket_tcp::close()
	{
		::close(_sock);	
	}

	class pkg_serialize
	{
		public:
			template<typename T>
				int sendpack(
						int socket_client,
						GUC::head &h,
						T& body
						);
			pkg_serialize();
			~pkg_serialize();

		private:
			std::string buf_body;
			std::stringstream ss;
	};

	pkg_serialize::pkg_serialize()
	{
	}

	pkg_serialize::~pkg_serialize()
	{
	}

	template<typename T>
		int pkg_serialize::sendpack(
				int socket_client,
				GUC::head &h,
				T& body
				)
		{
			buf_body = "";
			char buf[GUC_BUFFER_SIZE] = {0};
			body.SerializeToString(&buf_body);
			const int head_len = sizeof(GUC::head);
			int len = sizeof(head_len);
			len+=buf_body.size();
			if(len > GUC_BUFFER_SIZE)	
			{
				std::cout<<"h.id:"<<h.id<<" data len > GUC_BUFFER_SIZE"<<std::endl;
				return -1;
			}
			h.data_len = buf_body.size();

			memcpy(buf,&h,head_len);
			memcpy(buf+head_len,buf_body.c_str(),h.data_len);
			int num = send(socket_client,buf,h.data_len+head_len,0);
/*
			std::cout<<"h.data_len:"<<h.data_len
				<<" head_len:"<<head_len
				<<" send_num:"<<num
				<<std::endl;
*/
			return num;
		}
}

void test_set_people(GUC::head &h,PB::people &body,int cnt,std::stringstream &ss,int skillnum)
{
	h.id=(int)PB::guc_test_people;
	body.mutable_name()->append(ss.str());
	body.set_id(cnt);
	body.set_power(cnt*0.4f);
	for(int i=0;i<skillnum;++i)
	{
		PB::people::skill *s = body.add_skills();
		if(i!=3)
			s->set_skillid(i);
		else 
			s->set_skillid(0);
		ss.clear();
		ss.str("");
		ss<<"烈焰hu"<<i;
		s->mutable_skillname()->append(ss.str());
	}
	if(cnt%10000==0)
	{
		std::cout<<"recv process info:" 
			<< "id:"<< body.id()
			<< " name:"<< body.name().data() 
			<< " power:"<< body.power()
			<<std::endl;
		for(int i=0;i<body.skills_size();++i)
		{
			const PB::people::skill &s = body.skills(i);
			std::cout<<"skillId:"<<s.skillid()
				<<"skillname:"<<s.skillname()
				<<std::endl;
		}

	}	
}

int main()
{
	GUC::socket_tcp client;
	if(!client.init())
		std::cout<<"socket init error"<<std::endl;

	int ret = client.connect("127.0.0.1",8888);		
	if(ret<0)
	{
		std::cout<<"connect error "<<ret<<std::endl;
		exit(1);
	}
	GUC::pkg_serialize pkg;
	int cnt=0;
	std::stringstream ss;
	srand( (unsigned)time( NULL ));
	while(1)
	{
		ss.clear();
		ss.str("");
		if(0==(cnt%2))
		{
			ss<< "I LOVE HUYANYAN" << cnt;
		}
		else 
		{
			ss<<"I MAKE LOVE WITH HUYANYAN"<<cnt;
		}
		GUC::head h;
		PB::people body;
		int skillnum = rand()%SKILL_COUNT;
		test_set_people(h,body,cnt,ss,skillnum);
		int num = pkg.sendpack(client.sock_client(),h,body);
		if(num>0)
		{
			cnt++;
		}
		if(num<0)
		{
			std::cout<<"send error num="<<num<<std::endl;
			break;
		}
	}
	client.close();

	return 0;
}
