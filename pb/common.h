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

