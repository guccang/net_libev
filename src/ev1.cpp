#include <ev.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "guc_net.h"



GUC::socket_tcp_s *service;
int main()
{
	service = new GUC::socket_tcp_s();
	if(service->init(8888) != 0)
		return -1;
	service->run();
	service->close();
	return 0;
}


