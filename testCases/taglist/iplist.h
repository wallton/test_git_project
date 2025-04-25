#ifndef __IPLIST_H__
#define __IPLIST_H__


///	IP List was original designed for storage arbitary number of IPv4 address strings
///	In general, it can be used for storing a list of arbitary number of arbitary strings
///	It is actally a String List, but we keep its original name and call it IP List

typedef struct ipListType {
	int memsize;
	int ipz;
	char **ip;
	int allow_multiple;
} ipListType;

typedef struct linkipListType {
	int linkz;
	ipListType **link;
} linkipListType;

//	Create an empty IP List
ipListType *init_iplist(void);

//	Create an empty IP List that support multiple entries of same ip/item
ipListType *init_iplist_multiple(void);

//	Create an IP List from the input string, which is to be tokenized by
//	the specific seperator character (single charactor)
//	NOTE:
//	1. Leading / Trialing spaces of the token will be stripped
//	2. Repeated tokens / empty string tokens will be detected and stripped
//		from the list
ipListType *create_iplist(const char *str, char seperator);

//	Create an IP List from the input string, which is to be tokenized by
//	the specific seperator character (single charactor), and support
//	multiple entries of same ip/item
//	NOTE:
//	1. Leading / Trailing spaces of the token will be stripped
//	2. Unlike create_iplist(), repeated tokens will be preserved
//	3. Empty string tokens, on the list with more than one token, will
//		also be preserved
ipListType *create_iplist_multiple(const char *str, char seperator);

//	Free the IP List
void free_iplist(ipListType *list);

//	Insert a string at the end of the IP List
//	Repeated string will be detected and stripped if the list does not
//	allows multiple entries of same ip/item. e.g. list created with
//	init_iplist(), create_iplist(), or updated with
//	set_iplist_multiple(., 0)
//	Return Value:
//	1	Success (even value is found duplicated and no changes on list)
//	0	Fail	(probably Out-of-Memory)
int insert_iplist(ipListType *list, const char *ip);

//	Remove the specified string value from the IP List, if exists
//	Return Value:
//	1	Success (even num is originally not on the list)
//	0	Fail	(probably list is NULL)
int remove_iplist(ipListType *list, const char *ip);

//	Insert strings from the second list into end of the first list
//	Repeated integer will be ignored if the list is create by create_iplist()
//	Return Value:
//	1	Success (even duplicated ip/item is found, or no changes is done on the first list)
//	0	Fail	(probably Out-of-Memory)
int union_iplist(ipListType *list, const ipListType *transient_list);

//	Remove ip/item from the first list that is absent in the second list
//	Return Value:
//	1	Success
//	0	Fail	(probably Out-of-Memory)
int intersect_iplist(ipListType *list, const ipListType *transient_list);

//	Remove ip/item from the first list that is also found in the second list
//	Return Value:
//	1	Success
//	0	Fail	(probably Out-of-Memory)
int except_iplist(ipListType *list, const ipListType *transient_list);

//	Check if the specified string value is inside the IP List
//	Return Value:
//	1	Checked
//	0	Nope
int exist_iplist(const ipListType *list, const char *ip);

//	Check if this string list allows multiple entries of same ip/item
//	Return Value:
//	1	Allow storing duplicated / multiple same ip/items
//	0	Ignore duplicated / multiple same items on insertion
int is_iplist_multiple(const ipListType *list);

//	Update the setting if the list allows multiple entries of same ip/item
//	NOTE: To set allow_multiple to 0, the list may under go deduplication
//		to ensure it won't contain multiple entries of same ip/item
//	Return Value:
//	1	Success (This function never fails)
int set_iplist_multiple(ipListType *list, int alow_multiple);

//	Deduplication of the list, which is supporting multiple entries of
//	same ip/item
//	NOTE 1: If the list does not support multiple, no action will be taken
//	NOTE 2: Multiple support remains unchanged, use set_iplist_multiple()
//		if a change of multiple support is desired
//	Return Value:
//	1	Success (This function never fails)
int unique_iplist(ipListType *list);

//	Sort the list in ascending order, descedning order, or in custom order
//	of your choice
//	Return Value:
//	1	Success
//	0	Fail
int sort_iplist(ipListType *list);
int sort_iplist_reverse(ipListType *list);
int sort_iplist_ex(ipListType *list, int (*compar)(const void *, const void *));

//	Generate the serialized string format of the IP List with each string
//	delimited by the supplied sep string,
//	using the newly allocated memory space
//	NOTE: You are responsible to free() the result returned by seriailize_iplist()
char *serialize_iplist(const ipListType *list, const char *sep);

//	Generate the serialized string format of the IP List with each string
//	quoted by double quote (") and delimited by the supplied sep string,
//	using the newly allocated memory space
//	NOTE: You are responsible to free() the result returned by seriailize_iplist()
//	NOTE2: This function serve some particular purpose and should not be
//		a widely used everyday function
char *serialize_quoted_iplist(const ipListType *list, const char *sep);

///	Link/IP List are specific feature to be used in Balance/MAX Web,
///	no usage help is provided here.
///	If you are required to use this library on your project, please
///	consult kennyk@peplink.com.
#define	init_linkiplist(link_size)	create_linkiplist((link_size), NULL)
linkipListType *create_linkiplist(int link_size, const char *serialized_linkiplist);
void free_linkiplist(linkipListType *list);

int insert_linkiplist(linkipListType *list, int link, const char *ip);
int remove_linkiplist(linkipListType *list, int link, const char *ip);
int clear_linkiplist(linkipListType *list, int link);

int exist_linkiplist(const linkipListType *list, int link, const char *ip);

char *serialize_linkiplist(const linkipListType *list);

//	Generate a newly allocated string which joins all list items with a
//	connector string, and prepended/appended with prefix/suffix, i.e.
//	<prefix> + <ip1> + <connector> + <ip2> + ... <ipX> + <connector> + <ipX+1> + <suffix>
//	NOTE: You are responsible to free() the result returned by iplist_to_string()
char *iplist_to_string(const ipListType *list, const char *connector, const char *prefix, const char *suffix);


//	Insert a string at the end of the IP List, regardless if the string
//	value is already in the IP List
//	Return Value:
//	1	Success
//	0	Fail	(probably Out-of-Memory)
//	NEW: 2007/09/18, Multiple Same Element Support!
//
//	(Deprecated) As multiple entries support is enhanced, we should not
//	rely on this function anymore to insert deplicated entries.
//	It is advised to init/create/set the iplist as multiple supported,
//	as use insert_iplist() as usual.
//	WARNING: From this version, mixed use of insert_iplist() and this
//	depreated append_iplist() on the same list may get undesirable results
int append_iplist(ipListType *list, const char *ip) __attribute__ ((deprecated));

#include "numlist.h"

//	Create an IP List from a numlist
//	Accept multiple values if and only if the supplied numlist accepts
ipListType *create_iplist_from_numlist(numberListType *list);

#endif	/* __IPLIST_H__ */
