#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <libubox/uloop.h>
//#include <libubox/ustream.h>
//#include <libubox/utils.h>
//#include <libubox/usock.h>
#include <libubox/blob.h>
#include <libubox/blobmsg_json.h>
#include <libubox/blobmsg.h>

/*** --------------------------------------------------***/
void print_blob_buf(struct blob_buf *buf) {
    if (!buf || !buf->head) {
        printf("Empty or invalid blob_buf\n");
        return;
    }
    char *json_str = blobmsg_format_json_indent(buf->head, true, 0);
    if (json_str) {
        printf("blob_buf content:\n%s\n", json_str);
        free(json_str);
    } else {
        printf("Failed to format blob_buf to JSON\n");
    }
}
void
print_blob_attr(struct blob_attr *attr, bool full, int indent)
{
    char *json_str = blobmsg_format_json_indent(attr, full, indent);
    if (json_str) {
        printf("blob_attr content (full=%d, indent=%d):\n%s\n", full, indent, json_str);
        free(json_str);
    }
}
/*** --------------------------------------------------***/

int main(int argc, char *argv[]) 
{
    struct blob_buf b = {0};
    struct blob_buf added = {0};
	struct blob_attr *cur;
	int rem;
//----------------------------(1)
/*

blob_buf content:
{
        "name": "example",
        "value": 123,
        "config": {
                "mode": "active",
                "port": 8080,
                "settings": {
                        "protocol": "http",
                        "enabled": true
                }
        }
}

*/
    blob_buf_init(&b, 0); // 初始化 blob_buf

    // 添加頂層屬性
    blobmsg_add_string(&b, "name", "example");
    blobmsg_add_u32(&b, "value", 123);

    // 開啟一個命名表格
    void *table = blobmsg_open_table(&b, "config");
    blobmsg_add_string(&b, "mode", "active");
    blobmsg_add_u32(&b, "port", 8080);

    // 開啟一個嵌套表格
    void *nested_table = blobmsg_open_table(&b, "settings");
    blobmsg_add_string(&b, "protocol", "http");
    blobmsg_add_u8(&b, "enabled", 1);
    blobmsg_close_table(&b, nested_table); // 關閉嵌套表格

    blobmsg_close_table(&b, table); // 關閉外層表格

//----------------------------(2)
	blob_buf_init(&added, 0);

//printf("MsgType(%d) MsgName(%s)\n", blobmsg_type(b.head), blobmsg_name(b.head));


    //blobmsg_add_blob(&added, b.head);
/*Result

{
        null
}

*/

    //blobmsg_add_blob(&added, blob_data(b.head));
/*Result : only show first item in object

{
        "name": "example"
}


*/
    
    //blobmsg_add_field(&added, BLOBMSG_TYPE_TABLE, "", blob_data(b.head), blob_len(b.head));
/*Result


{
        {
                "name": "example",
                "value": 123,
                "config": {
                        "mode": "active",
                        "port": 8080,
                        "settings": {
                                "protocol": "http",
                                "enabled": true
                        }
                }
        }
}


*/

	blob_for_each_attr(cur, b.head, rem)
    {
        printf("MsgType(%d) MsgName(%s) rem(%d)\n", blobmsg_type(cur), blobmsg_name(cur), rem);
		blobmsg_add_blob(&added, cur);
    }
/*Result

{
        "name": "example",
        "value": 123,
        "config": {
                "mode": "active",
                "port": 8080,
                "settings": {
                        "protocol": "http",
                        "enabled": true
                }
        }
}

*/



//blobmsg_add_blob(struct blob_buf *buf, struct blob_attr *attr)

//  blobmsg_add_blob(&du->buf, blob_data(msg));
//	blobmsg_add_field(&du->buf, BLOBMSG_TYPE_TABLE, "", blob_data(msg), blob_len(msg));


    // 印出 blob_buf 內容
    //print_blob_buf(&b);
    printf("---------------------\n");
    //print_blob_attr(b.head, true, 0);

    print_blob_buf(&added);


    // 清理
    blob_buf_free(&b);

    return 0;
}
