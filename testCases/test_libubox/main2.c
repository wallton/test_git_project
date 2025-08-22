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
void
print_blob_buf(struct blob_buf *buf) {
    if (!buf || !buf->head) {
        printf("Empty or invalid blob_buf\n");
        return;
    }
    char *json_str = blobmsg_format_json_indent(buf->head, true, 0);
    if (json_str) {
        printf("blob_buf content:\n%s\n", json_str);
        free(json_str);
    }
}
void
print_blob_attr(struct blob_attr *attr)
{
    char *json_str = blobmsg_format_json_indent(attr, true, 0);
    if (json_str) {
        printf("blob_attr content:\n%s\n", json_str);
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
    struct blob_attr   *qsw_attr;
    struct blobmsg_hdr *qsw_hdr;
    int                 qsw_len = 0;
//----------------------------(1)
/*

{
  "error_code": 200,
  "error_message": "OK",
  "result": {
    "access_token": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJGb290UHJpbnQiOiJuZHIiLCJJcCI6IjE5Mi4xNjguMTAuMTEiLCJNYWMiOiIyNDo1ZTpiZTo2YTpiOToyNCIsIlByaXZpbGVnZSI6MTUsIlByb3RvIjoiSFRUUC8xLjEiLCJVc2VyTmFtZSI6ImFkbWluIiwiZXhwIjoxNzU0MjcyMzI0LCJpYXQiOjE3NTM2Njc1MjR9.H2ZBHys9fuMVfW6Ht8PhxhRVcuVwhxiUEdP3iun2rpPBVYoXUdTqkYTM_KA5riO1Z1GFw7l-ywuwwbW_krQVgrlK8KhhWDQmdMnrM5UCY7I5-rgzlxbRbB4wr3Y6iUGMbgXD7noVul7Jn_j4EXXyxlrsKlqYVkUhPwyspi7Y8y_kV3Ocp9GSv709gJOc4FjLdLhbpQV_wc4eU_nBlDW4WPoDjwHc1UCc5ukM7xTl8X6Hde6JgKVjPzVyJnQ5HSzd_jY0qHpJNIafiQ1JImjymruJ8Fa2pcd3cTTK8ZRSWmOXkrYPgZQ-NA4PLpEby_aXdH825nEIa3IREG-2lY_U-g",
    "hostname": "QSW-M2116P-2T2S",
    "qsw_uuid": "24:5E:BE:68:57:2E",
    "model": "QSW-M2116P-2T2S",
    "chip": "microsemi"
  }
}



*/
    blob_buf_init(&b, 0);

    blobmsg_add_string(&b, "error_message", "OK");
    blobmsg_add_u32(&b, "error_code", 200);

    // 1
    void *table = blobmsg_open_table(&b, "Payload");
/*user---------------------------------------------------*/
    blobmsg_add_string(&b, "user", "admin");
/*pri----------------------------------------------------*/
    blobmsg_add_u32(&b, "pri", 1);
/*exp----------------------------------------------------*/
    blobmsg_add_u32(&b, "exp", 0);

    blobmsg_close_table(&b, table);

    // 2
    void *result_table = blobmsg_open_table(&b, "result");

    blobmsg_add_string(&b, "hostname", "QSW-M2116P-2T2S");
    blobmsg_add_string(&b, "qsw_uuid", "24:5E:BE:68:57:2E");
    blobmsg_add_string(&b, "model", "QSW-M2116P-2T2S");
    blobmsg_add_string(&b, "chip", "realtek");

    blobmsg_close_table(&b, result_table);

	blob_for_each_attr(cur, b.head, rem)
    {
        printf("MsgType(%d) MsgName(%s) rem(%d)\n", blobmsg_type(cur), blobmsg_name(cur), rem);
        
        if ( !strcmp(blobmsg_name(cur), "result") )
        {
            print_blob_attr(cur);
        }
    }
/*Result



*/
    print_blob_buf(&b);
    printf("----------------------End of blob_buf b\n\n");
/*==================================================*/
	blob_buf_init(&added, 0);

    result_table = blobmsg_open_table(&added, "result");
#if 0
	blob_for_each_attr(cur, b.head, rem)
    {
		blobmsg_add_blob(&added, cur);
    }
#else
	blob_for_each_attr(cur, b.head, rem)
    {
        if ( !strcmp(blobmsg_name(cur), "result") )
        {
            qsw_len = blobmsg_data_len(cur);

            __blob_for_each_attr(qsw_attr, blobmsg_data(cur), qsw_len)
            {
                qsw_hdr = blob_data(qsw_attr);

                printf("billy(%s# %d) Field(%s)\n", __func__, __LINE__, qsw_hdr->name);

                blobmsg_add_blob(&added, qsw_attr);
            }
        }
    }


#endif
    blobmsg_close_table(&added, result_table);
    print_blob_buf(&added);


    // 清理
    blob_buf_free(&b);
    blob_buf_free(&added);

    return 0;
}


