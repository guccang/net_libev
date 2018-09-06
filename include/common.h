#pragma once
#pragma pack (1) /*指定按1字节对齐*/
namespace GUC
{
	struct head
	{
		int data_len;
		int id;
	};
}
#define GUC_BUFFER_SIZE (999)
#pragma pack () /*取消指定对齐，恢复缺省对齐*/
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

typedef  char   byte;// 定义byte
typedef  void (*ev_cb)(struct ev_loop* , struct ev_io*,int); // libev 回调函数声明
