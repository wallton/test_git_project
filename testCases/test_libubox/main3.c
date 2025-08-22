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
print_blob_buf(struct blob_buf *buf)
{
    if (!buf || !buf->head) {
        printf("Empty or invalid blob_buf\n");
        return;
    }
    char *json_str = blobmsg_format_json_indent(buf->head, true, 0);
    if (json_str) {
        printf("\n%s\n", json_str);
        free(json_str);
    }
}
void
print_blob_attr(struct blob_attr *attr)
{
    char *json_str = blobmsg_format_json_indent(attr, true, 0);
    if (json_str) {
        printf("\n%s\n", json_str);
        free(json_str);
    }
}

/*
    //if(strncmp(KeyName,"",strlen(KeyName)) != 0)
    //{
        __blob_for_each_attr(attr, blobmsg_data(head), len)
        {
            type = blobmsg_type(attr);
            hdr = blob_data(attr);
            printf("billy(%s# %d) hdr->name(%s) type(%d)\n", __func__, __LINE__, hdr->name, type);
            
            if(type == BLOBMSG_TYPE_ARRAY)
            {
                if((strlen(hdr->name) == strlen(KeyName)) && (strncmp(hdr->name, KeyName, strlen(KeyName)) == 0))
                {
                    printf("billy(%s# %d) ---------------\n", __func__, __LINE__);
                    i4ListSize =  blobmsg_check_array(attr, BLOBMSG_TYPE_STRING);
                    ListObj = blob_memdup(attr);

                }
            }
        }
    //}

*/
#define INT4    int
#define UINT2 short
typedef unsigned char UINT1;
#define STRLEN  strlen
#define STRNCMP strncmp
#define STRNCPY strncpy
#define MEM_FREE(p) free(p)
#define MEMSET(s,c,n) memset((void *)(s), (int )c, n)
#define RESTFUL_SUCCESS 1
#define RESTFUL_FAILURE 0
#define RESTFUL_DATA_ERROR -1

INT4 SN_Get_Obj_StrList(struct blob_attr *head, UINT1 *KeyName, struct blob_attr **ListObj)
{
    struct blob_attr   *attr;
    struct blobmsg_hdr *hdr;
    int                 len = 0;
    int                 type = 0;
    int                 i4ListSize = 0;

    len = blobmsg_data_len(head);

    if(STRNCMP(KeyName,"",STRLEN(KeyName)) != 0)
    {
        __blob_for_each_attr(attr, blobmsg_data(head), len)
        {
            type = blobmsg_type(attr);
            hdr = blob_data(attr);

            if(type == BLOBMSG_TYPE_ARRAY)
            {
                if((STRLEN(hdr->name)==STRLEN(KeyName)) && (STRNCMP(hdr->name,KeyName,STRLEN(KeyName)) == 0))
                {
                    i4ListSize =  blobmsg_check_array(attr, BLOBMSG_TYPE_STRING);
                    *ListObj = blob_memdup(attr);
                    return i4ListSize;
                }
            }
        }
    }
    else /* blobmsg_type(head) == BLOBMSG_TYPE_ARRAY */
    {
        i4ListSize =  blobmsg_check_array(head, BLOBMSG_TYPE_STRING);
        *ListObj = blob_memdup(head);
        return i4ListSize;
    }
    *ListObj = NULL;
    return 0;
}

INT4 SN_Get_Obj_String(struct blob_attr *head, INT4 i4ListIdx, UINT1 *pu1Value, UINT2 u2MaxStringLen)
{
    struct blob_attr *attr;
    int               len = 0;
    int               type = 0;
    int               Idx = 0;
    char             *attr_string = NULL;
    size_t            attr_string_len = 0;
    
    len = blobmsg_data_len(head);

    __blob_for_each_attr(attr, blobmsg_data(head), len)
    {
        type = blobmsg_type(attr);

        if(type == BLOBMSG_TYPE_STRING)
        {
            if(i4ListIdx == Idx)
            {
                attr_string = blobmsg_get_string(attr);
                attr_string_len = (attr_string) ? STRLEN(attr_string) : 0;

                if ((attr_string_len + 1) > u2MaxStringLen) {
                    return RESTFUL_DATA_ERROR;
                }
                if (attr_string_len > 0 && pu1Value) {
                    STRNCPY(pu1Value, attr_string, attr_string_len);
                    pu1Value[attr_string_len] = '\0';
                }

                return RESTFUL_SUCCESS;
            }
            Idx++;
        }
    }

    return RESTFUL_FAILURE;
}




/*** --------------------------------------------------***/

int main(int argc, char *argv[]) 
{
    struct blob_buf    b = {0};

    void              *Array;
    void              *Array2;
    int                i4ListSize = 0;
    struct blob_attr  *monitorPortStrList = NULL;
//----------------------------(1)
/* Request

{
        "mirrorPort": "3",
        "monitorPort": [
                "4",
                "6"
        ]
}

*/
    blob_buf_init(&b, 0);

    blobmsg_add_string(&b, "mirrorPort", "3");
    // 1
    Array = blobmsg_open_array(&b, "monitorPort");
    blobmsg_add_string(&b, NULL, "4");
    blobmsg_add_string(&b, NULL, "6");
    blobmsg_add_string(&b, NULL, "13");
    blobmsg_add_string(&b, NULL, "22");
    blobmsg_close_array(&b, Array);
    // 2
    Array2 = blobmsg_open_array(&b, "abc");
    blobmsg_add_string(&b, NULL, "a");
    blobmsg_add_string(&b, NULL, "bb");
    blobmsg_add_string(&b, NULL, "ccc");
    blobmsg_add_string(&b, NULL, "dddd");
    blobmsg_add_string(&b, NULL, "eeeee");
    blobmsg_close_array(&b, Array2);

    print_blob_buf(&b);

//-----------------------------------------
    struct blob_attr *head = b.head;
    int    i4ListIdx = 0;
    char   ai1IfStr[10];

    i4ListSize = SN_Get_Obj_StrList(head, "monitorPort", &monitorPortStrList);

printf("billy(%s# %d) i4ListSize(%d)\n", __func__, __LINE__, i4ListSize);

    if (i4ListSize > 0)
    {
        for (i4ListIdx = 0; i4ListIdx < i4ListSize; i4ListIdx++)
        {
            MEMSET (ai1IfStr, 0, sizeof(ai1IfStr));

            if (SN_Get_Obj_String(monitorPortStrList, i4ListIdx, ai1IfStr, sizeof(ai1IfStr)) == RESTFUL_SUCCESS)
            {
                printf("billy(%s# %d) ai1IfStr(%s)\n", __func__, __LINE__, ai1IfStr);
            }
        }

        MEM_FREE(monitorPortStrList);
    }

    // 清理
    blob_buf_free(&b);


    return 0;
}

/*
#define blobmsg_for_each_attr(pos, attr, rem) \
	for (rem = attr ? blobmsg_data_len(attr) : 0, pos = (struct blob_attr *) (attr ? blobmsg_data(attr) : NULL); \
	     rem > 0 && (blob_pad_len(pos) <= rem) && (blob_pad_len(pos) >= sizeof(struct blob_attr)); \
         rem -= blob_pad_len(pos), pos = blob_next(pos))

enum blobmsg_type{
	BLOBMSG_TYPE_UNSPEC,
	BLOBMSG_TYPE_ARRAY,
	BLOBMSG_TYPE_TABLE,    <-- 2
	BLOBMSG_TYPE_STRING,   <-- 3
	BLOBMSG_TYPE_INT64,
	BLOBMSG_TYPE_INT32,    <-- 5
	BLOBMSG_TYPE_INT16,
	BLOBMSG_TYPE_INT8,
	BLOBMSG_TYPE_DOUBLE,
	__BLOBMSG_TYPE_LAST,
	BLOBMSG_TYPE_LAST = __BLOBMSG_TYPE_LAST - 1,
	BLOBMSG_TYPE_BOOL = BLOBMSG_TYPE_INT8,
};
*/


/*
    struct blob_attr *head = b.head;
    struct blob_attr *ListObj = NULL;
    char *KeyName = "monitorPort";
    struct blob_attr *attr;
    struct blobmsg_hdr *hdr;
    int len = 0;
    int type = 0;
    int i4ListSize = 0;

    len = blobmsg_data_len(head);
    type = blobmsg_type(head);

    printf("billy(%s# %d) type(%d)\n", __func__, __LINE__, type);

    //if(strncmp(KeyName,"",strlen(KeyName)) != 0)
    //{
        __blob_for_each_attr(attr, blobmsg_data(head), len)
        {
            type = blobmsg_type(attr);
            hdr = blob_data(attr);
            printf("billy(%s# %d) hdr->name(%s) type(%d)\n", __func__, __LINE__, hdr->name, type);
            
            if(type == BLOBMSG_TYPE_ARRAY)
            {
                if((strlen(hdr->name) == strlen(KeyName)) && (strncmp(hdr->name, KeyName, strlen(KeyName)) == 0))
                {
                    printf("billy(%s# %d) ---------------\n", __func__, __LINE__);
                    i4ListSize =  blobmsg_check_array(attr, BLOBMSG_TYPE_STRING);
                    ListObj = blob_memdup(attr);

                }
            }
        }
    //}


*/
/*
    // 遍歷數組中的字符串元素
    struct blob_attr *elem;
    struct blob_attr *array = monitorPortStrObj;

    if (blobmsg_type(array) == BLOBMSG_TYPE_ARRAY)
    {
        printf("Parsed strings:\n");
        blobmsg_for_each_attr(elem, array, rem) {
            if (blobmsg_type(elem) == BLOBMSG_TYPE_STRING) {
                printf("- %s\n", (char *) blobmsg_data(elem));
            }
        }
    }
*/

