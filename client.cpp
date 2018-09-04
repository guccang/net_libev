#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include "net.pb.h"


#define SKILL_COUNT (50)
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
					PB::head &h,
					T& body
					);
			pkg_serialize();
			~pkg_serialize();

		private:
			std::string buf_body;
			std::string buf;
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
			PB::head &h,
			T& body
			)
	{
		ss.clear();
		ss.str("");
		buf = "";
		buf_body = "";
		int len = 4;
		int id = h.id();
		body.SerializeToString(&buf_body);
		len+=buf_body.size();
		char szbuf[8];
		//memset(szbuf,0,8);
		//h.set_size(buf_body.size());
		//h.SerializeToString(&buf);
		memcpy(szbuf,&len,sizeof(int));
	    	memcpy(szbuf+4,&id,sizeof(int));	
		//ss<<szbuf<<buf_body;
		//ss>>buf;
		int num = send(socket_client,(char*)&len,sizeof(int),0);
		
		num += send(socket_client,buf_body.c_str(),buf_body.size(),0);
		return num;
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
		PB::head h;
		h.set_id(1);
		PB::people body;
		body.set_id(cnt);
		body.mutable_name()->append("huyanyan");
		body.set_power(cnt*0.002f);
		for(int i=0;i<SKILL_COUNT;++i)
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
		if(cnt%10000==0 || cnt%10001==0)
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
		//ostringstream oss(ios_base::binary);
		//body.SerializePartialToOstream(oss);
		std::string buf_body;
		PB::people ppp;
		body.SerializeToString(&buf_body);
		ppp.ParseFromString(buf_body);
		
		int num = pkg.sendpack(client.sock_client(),h,body);
		if(num>0)
		{
			cnt++;
			if(cnt%1000 == 0 || cnt%1001==0)
				std::cout<<"name:"<<body.name()
						<<"id:"<<body.id()
						<<std::endl;
		}
	}
	client.close();

	return 0;
}
