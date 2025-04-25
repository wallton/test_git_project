#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>  /* assert */
#include "iplist.h"

#define	_STR_N(n)	#n
#define	_STR(VAL)	_STR_N(VAL)

#define	STRUTILS_LINKIPLIST_BUF_MAX	    8191
#define	STRUTILS_LINKIPLIST_MEMBLK_SIZE	256


static int __exist_iplist(const ipListType *list, const char *ip, int upper_bound)
{
	if (list && ip) {
		int i;
		if (upper_bound <= 0 || list->ipz < upper_bound) {
			upper_bound = list->ipz;
		}
		for (i = upper_bound - 1; i >= 0; i--) {
			if (list->ip[i][0] == ip[0] &&
				strcmp(list->ip[i], ip) == 0) {
				return i;	// Exists
			}
		}
	}
	return -1;	// Not exists
}

static int __remove_iplist_at_pos(ipListType *list, int pos)
{
	if (!list) {
		return 0;	// OOM;
	}
	if (0 <= pos && pos < list->ipz) {
		char *pull_ip = list->ip[pos];
		int i, i_max;
		for (i = pos + 1, i_max = list->ipz; i < i_max; i++) {
			list->ip[i - 1] = list->ip[i];
		}
		list->ipz--;
		free(pull_ip);
	}
	return 1;
}

#define _MIN(a, b) ((a) < (b) ? (a) : (b))
static ipListType *__create_iplist(const char *str, char seperator, int multiple, int parse_empty_string)
{
	int is_oom = 0;
	ipListType *list = NULL;

	if (!is_oom) {
		list = multiple ? init_iplist_multiple() : init_iplist();
		if (!list) {
			is_oom = 1;
		}
	}

	if (!is_oom && str && str[0]) {
		char token[STRUTILS_LINKIPLIST_BUF_MAX + 1];
		const char sep_str[2] = { seperator, 0 };
		const char *p = str;
		int n, copy_size;
		while (!is_oom) {

			// 1. Skip over leading spaces
			p += strspn(p, " ");

			// 2. Locate the token size (incl. trailing spaces)
			n = strcspn(p, sep_str);
			n += strspn(p + n, " ");

			// 3. Capture the token
			copy_size = _MIN(sizeof(token), n + 1);
			snprintf(token, copy_size, "%s", p);
			p += n;

			// 4. Trim trailing spaces of the captured token
			while (n-- && n < copy_size && token[n] == ' ') {
				token[n] = 0;
			}

			// 5. Put qualified token into the list
			// (Empty token is qualified only there are multiple
			//	tokens, and operates with parse_empty_string)
			if (token[0] ||
				(parse_empty_string && (*p || list->ipz))) {
				if (!insert_iplist(list, token)) {
					is_oom = 1;
				}
			}

			// 6. Check if the parsing is done; if not, skip over
			//	the seperator character for next the round
			if (!*p) {
				break;
			} else if (*p == seperator) {
				p++;
			}
		};
	}

	// Out-of-Memory Handling
	if (is_oom) {
		free_iplist(list);
		list = NULL;
	}

	return list;
}


ipListType *init_iplist(void)
{
	int is_oom = 0;
	ipListType *list = NULL;

	if (!is_oom) {
		list = calloc(1, sizeof(ipListType));
		if (!list) {
			is_oom = 1;
		}
	}

	if (!is_oom) {
		int default_memsize = STRUTILS_LINKIPLIST_MEMBLK_SIZE;
		list->ip = (char **) malloc(sizeof(char *) * default_memsize);
		if (list->ip) {
			list->memsize = default_memsize;
		} else {
			is_oom = 1;
		}
	}

	// Out-of-Memory handling
	if (is_oom) {
		free_iplist(list);
		list = NULL;
	}

	return list;
}

ipListType *init_iplist_multiple(void)
{
	ipListType *list = init_iplist();
	set_iplist_multiple(list, 1);
	return list;
}

ipListType *create_iplist(const char *str, char seperator)
{
	return __create_iplist(str, seperator, 0, 0);
}

ipListType *create_iplist_multiple(const char *str, char seperator)
{
	return __create_iplist(str, seperator, 1, 1);
}



void free_iplist(ipListType *list)
{
	if (!list) return;
	if (list->memsize) {
		int i;
		for (i = list->ipz - 1; i >= 0; i--) {
			free(list->ip[i]);
		}
		free(list->ip);
	}
	free(list);
	return;
}

int remove_iplist(ipListType *list, const char *ip)
{
	if (!list) {
		return 0;	// OOM;
	}
	do {
		int pos = __exist_iplist(list, ip, 0);
		if (pos == -1) {
			break;
		}
		__remove_iplist_at_pos(list, pos);
	} while (list->allow_multiple);
	return 1;
}

int exist_iplist(const ipListType *list, const char *ip)
{
	return (__exist_iplist(list, ip, 0) == -1 ? 0 : 1);
}

static int __append_iplist(ipListType *list, const char *ip)
{
	if (!list) {
		return 0;	// OOM;
	}
	if (list->memsize <= list->ipz)
	{
		int memsize_step = STRUTILS_LINKIPLIST_MEMBLK_SIZE;
		char **ip = (char **) realloc(list->ip, sizeof(char *) * (list->memsize + memsize_step));
		if (!ip) {
			return 0;	// OOM;
		}
		list->ip = ip;
		list->memsize += memsize_step;
	}
	{
		char *s = strdup(ip);
		if (!s) {
			return 0;	// OOM!
		}
		list->ip[list->ipz++] = s;
	}
	return 1;
}

int insert_iplist(ipListType *list, const char *ip)
{
	if (!list) {
		return 0;	// OOM!
	}
	if (list->allow_multiple || !exist_iplist(list, ip)) {
		if (!__append_iplist(list, ip)) {
			return 0;	// OOM!
		}
	}
	return 1;
}

int append_iplist(ipListType *list, const char *ip)
{
	// As a deprecated function, we still need to maintain the
	//	"allow_multiple", in case called this, expected
	//	multiple entries to be inserted.
	if (!list->allow_multiple) {
		list->allow_multiple = 1;
	}
	return __append_iplist(list, ip);
}

int union_iplist(ipListType *list, const ipListType *transient_list)
{
	int is_oom = 0;
	if (!list) {
		is_oom = 1;
	}
	if (!is_oom && transient_list && transient_list->ipz) {
		int i, i_max;
		for (i = 0, i_max = transient_list->ipz; i < i_max; i++) {
			if (!insert_iplist(list, transient_list->ip[i])) {
				is_oom = 1;
				break;
			}
		}
	}
	return is_oom ? 0 : 1;
}

// Internal Handling of string list Intersection (yin) and Except / Minus (yan)
//	yin - Intersection	(yin = 1)
//	yan - Except / Minus	(yin = 0)
static int yinyan_iplist(ipListType *list, const ipListType *transient_list, int yin)
{
	int is_oom = 0;
	if (!list) {
		is_oom = 1;
	}
	if (!is_oom && list->ipz) {
		int i, is_yin;
		yin = yin ? 1 : 0;	// Ensure yin is either 0 or 1
		for (i = list->ipz - 1; i >= 0; i--) {
			is_yin = exist_iplist(transient_list, list->ip[i]);
			if (yin ^ is_yin) {
				__remove_iplist_at_pos(list, i);
			}
		}
	}
	return is_oom ? 0 : 1;
}

int intersect_iplist(ipListType *list, const ipListType *transient_list)
{
	return yinyan_iplist(list, transient_list, 1);
}

int except_iplist(ipListType *list, const ipListType *transient_list)
{
	return yinyan_iplist(list, transient_list, 0);
}

int unique_iplist(ipListType *list)
{
	if (list && list->allow_multiple) {
		int i;
		for (i = list->ipz - 1; i > 0; i--) {
			if (__exist_iplist(list, list->ip[i], i)) {
				__remove_iplist_at_pos(list, i);
			}
		}
	}
	return 1;
}

int is_iplist_multiple(const ipListType *list)
{
	return list && list->allow_multiple ? 1 : 0;
}

int set_iplist_multiple(ipListType *list, int allow_multiple)
{
	if (list && (allow_multiple ^ list->allow_multiple)) {
		if (!allow_multiple) {
			unique_iplist(list);
		}
		list->allow_multiple = allow_multiple ? 1 : 0;
	}
	return 1;
}

int sort_iplist_ex(ipListType *list, int (*compar)(const void *, const void *))
{
	if (!compar) {
		return 0;
	}
	if (list && list->ipz > 1) {
		qsort(list->ip, list->ipz, sizeof(char *), compar);
	}
	return 1;
}

static int __iplist_compar_asc(const void *a, const void *b)
{
	return strcmp(*(char **)a, *(char **)b);
}

int sort_iplist(ipListType *list)
{
	return sort_iplist_ex(list, __iplist_compar_asc);
}

static int __iplist_compar_dsc(const void *a, const void *b)
{
	return strcmp(*(char **)b, *(char **)a);
}

int sort_iplist_reverse(ipListType *list)
{
	return sort_iplist_ex(list, __iplist_compar_dsc);
}

static char *__serialize_iplist(const ipListType *list, const char *sep, const char *prefix, const char *suffix)
{
	char *result = NULL;
	if (list && list->ipz > 0)
	{
		int resultz, i, n;
		char *ptr = NULL;

//	Estimate Result Size
		resultz = 0;
		for (i = 0; i < list->ipz; i++)
		{
			resultz += strlen(list->ip[i]);
		}
		resultz += (list->ipz - 1) * strlen(sep) + strlen(prefix) + strlen(suffix) + 1;
		ptr = result = (char *) malloc(sizeof(char) * resultz);

//	Construct Serialized Result
		if (ptr)
		{
			{
				n = sprintf(ptr, "%s", prefix);
				ptr += n;
			}
			{
				n = sprintf(ptr, "%s", list->ip[0]);
				ptr += n;
			}
			for (i = 1; i < list->ipz; i++)
			{
				n = sprintf(ptr, "%s%s", sep, list->ip[i]);
				ptr += n;
			}
			{
				n = sprintf(ptr, "%s", suffix);
				ptr += n;
			}
			assert(ptr <= result + resultz);
		}
	}
	return (result? result: strdup(""));
}

char *serialize_iplist(const ipListType *list, const char *sep)
{
	return __serialize_iplist(list, sep, "", "");
}

char *serialize_quoted_iplist(const ipListType *list, const char *sep)
{
	char *result = NULL;
	{
		char quote[] = "\"";
		char *quoted_sep = (char *) malloc(sizeof(char) * (strlen(sep) + strlen(quote) * 2 + 1));
		if (quoted_sep)
		{
			sprintf(quoted_sep, "%s%s%s", quote, sep, quote);
			result = __serialize_iplist(list, quoted_sep, quote, quote);
			free(quoted_sep);
		}
	}
	return (result? result: strdup(""));
}

char *iplist_to_string(const ipListType *list, const char *connector, const char *prefix, const char *suffix)
{
	return __serialize_iplist(list, connector, prefix, suffix);
}


// linkiplist functions

void free_linkiplist(linkipListType *list)
{
	if (!list) return;
	if (list->link)
	{
		int i;
		for (i = list->linkz - 1; i >= 0; i--)
		{
			free_iplist(list->link[i]);
		}
		free(list->link);
	}
	free(list);
}

//	Integrated constructor (with flat code structure)
linkipListType *create_linkiplist(int link_size, const char *str)
{
// We support 0 to link_size, i.e. linkz = link_size + 1;
	int is_oom = 0;
//	1. Initialize basic sturcture
	linkipListType *list = (linkipListType *) malloc(sizeof(linkipListType));
	if (!list)
	{
		is_oom = 1;
	}
	if (!is_oom)
	{
		int linkz = link_size + 1;
		list->link = (ipListType **) calloc(linkz, sizeof(ipListType *));
		list->linkz = linkz;
		if (!list->link)
		{
			is_oom = 1;
		}
	}
//	2. Parse the serialized string (if any)
	if (!is_oom && str && str[0])
	{
		char token[STRUTILS_LINKIPLIST_BUF_MAX + 1];
		int pos = 0, strz = strlen(str);
		while (pos < strz)
		{
			int n = 0, link_id = 0;
			token[0] = 0;
			if (sscanf(str + pos,
				"%d:%" _STR(STRUTILS_LINKIPLIST_BUF_MAX) "[^ ]%n",
				&link_id, token, &n) != 2) break;
			if (!n) break;			// Rare case, abnormal reading on information
			pos += n + 1;
			if (!token[0]) continue;	// No token, skip ahead
			if (0 <= link_id && link_id < list->linkz)
			{
				ipListType *ipl = create_iplist(token, ',');
				if (!ipl)
				{
					is_oom = 1;
					break;
				}
				free_iplist(list->link[link_id]);
				list->link[link_id] = ipl;
			}
		}
	}
//	3. Initialize each iplist without predefined values
	if (!is_oom)
	{
		int i, i_max;
		for (i = 0, i_max = list->linkz; i < i_max; i++)
		{
			if (!list->link[i])
			{
				list->link[i] = init_iplist();
			}
			if (!list->link[i])
			{
				is_oom = 1;
				break;
			}
		}
	}
//	Out-of-memory Exception Handling
	if (is_oom)
	{
		free_linkiplist(list);
		list = NULL;
	}
	return list;
}

int insert_linkiplist(linkipListType *list, int link_id, const char *ip)
{
	if (list && 0 <= link_id && link_id < list->linkz)
	{
		return insert_iplist(list->link[link_id], ip);
	}
	return 0;
}

int remove_linkiplist(linkipListType *list, int link_id, const char *ip)
{
	if (list && 0 <= link_id && link_id < list->linkz)
	{
		return remove_iplist(list->link[link_id], ip);
	}
	return 0;
}

int clear_linkiplist(linkipListType *list, int link_id)
{
	int is_cleared = 0;
	if (list && 0 <= link_id && link_id < list->linkz)
	{
		if (list->link[link_id]->ipz)
		{
			ipListType *ipl = init_iplist();
			if (ipl)
			{
				free_iplist(list->link[link_id]);
				list->link[link_id] = ipl;
			}
		}
		is_cleared = (list->link[link_id]->ipz == 0? 1: 0);
	}
	return is_cleared;	// 1: Ensured, 0: Failed
}

int exist_linkiplist(const linkipListType *list, int link_id, const char *ip)
{
	if (list && 0 <= link_id && link_id < list->linkz)
	{
		return exist_iplist(list->link[link_id], ip);
	}
	return 0;
}

char *serialize_linkiplist(const linkipListType *list)
{
	char *result = NULL;
	if (list && list->linkz > 0)
	{
		char **s_segment = (char **) malloc(sizeof(char *) * list->linkz);
		if (s_segment)
		{
			int resultz;
			int i, i_max;

//	Prepare serialized segments, and estimate the approximate result length
			resultz = 0;
			for (i = 0, i_max = list->linkz; i < i_max; i++)
			{
				s_segment[i] = serialize_iplist(list->link[i], ",");
				if (s_segment[i])
				{
					resultz += strlen(s_segment[i]);
				}
			}
			//	Relaxed estimation, as each has a prefix " n:"
			{
				char pfx_sample[32];
				sprintf(pfx_sample, " %d:", list->linkz);
				resultz += list->linkz * strlen(pfx_sample);
			}
			result = (char *) malloc(sizeof(char) * resultz);

//	Compose the serialized result
			if (result)
			{
				char init_sep[10] = "", inter_sep[10] = " ";
				char *ptr = result;
				char *sep = init_sep;
				for (i = 0, i_max = list->linkz; i < i_max; i++)
				{
					if (s_segment && strlen(s_segment[i]))
					{
						int n = sprintf(ptr, "%s%d:%s", sep, i, s_segment[i]);
						ptr += n;
						sep = inter_sep;
					}
				}
				*ptr = 0;
			}

//	Release serialized segments, after use
			for (i = 0, i_max = list->linkz; i < i_max; i++)
			{
				free(s_segment[i]);
			}
			free(s_segment);
		}
	}
	return (result? result: strdup(""));
}

//						         111       22
// Buffer size for integer as string		123456789012345678901
// 11 is enough for 32-bit unsigned integer:	4294967296
// 12 is enough for 32-bit signed integer:	-2147483648
// 21 is enough for 64-bit unsigned integer:	18446744073709551616
// 21 is enough for 64-bit signed integer:	-9223372036854775808
#define	STRUTILS_ITOA_BUF_SIZE	21
#define	STRUTILS_ITOA_SLOT_SIZE	10

// turn an integer to a string
const char *itoa(int x)
{
	static int p = 0;
	static char out[STRUTILS_ITOA_SLOT_SIZE][STRUTILS_ITOA_BUF_SIZE];
	int i = p = (p + 1) % STRUTILS_ITOA_SLOT_SIZE;
	sprintf(out[i], "%d", x);
	return out[i];
}

static ipListType *__create_iplist_from_numlist(numberListType *numlist, int multiple)
{
	int is_oom = 0;
	ipListType *list = NULL;

	if (!is_oom) {
		list = multiple ? init_iplist_multiple() : init_iplist();
		if (!list) {
			is_oom = 1;
		}
	}

	if (!is_oom) {
		int i;
		for (i = 0; i < numlist->size; i++) {
			int v = numlist->item[i];
			if (!insert_iplist(list, itoa(v))) {
				is_oom = 1;
				break;
			}
		}
	}

	// Out-of-Memory Handling
	if (is_oom) {
		free_iplist(list);
		list = NULL;
	}

	return list;
}
ipListType *create_iplist_from_numlist(numberListType *list)
{
	int allow_multiple = list ? list->allow_multiple : 0;
	return __create_iplist_from_numlist(list, allow_multiple);
}


