#ifndef __NUMLIST_H__
#define __NUMLIST_H__

#include "ddm.h"


// Ordered Number List ADT
typedef struct numberListType {
	int size;
	int memsize;
	int *item;
	int allow_multiple;
} numberListType;

//	Create a numlist from the input string, which should be a space seperated list of non-negative integer value
//	Repeated integer will be detected and stripped by the number list
//	e.g. "1 2 1 7 4" -> numlist { 1, 2, 7, 4 }
numberListType *create_numlist(const char *numlist_str);

//	Create a numlist from the input string, allowing repeated integer be preserved as is
//	e.g. "1 2 1 7 4" -> numlist { 1, 2, 1, 7, 4 }
numberListType *create_numlist_multiple(const char *numlist_str);


//	Free the numlist object
void free_numlist(numberListType *list);

//	Insert a number at the end of the numlist
//	Repeated integer will be detected and stripped if the list is created by create_numlist()
//	Return Value:
//	1	Success	(even num is found duplicated, and no changes on list)
//	0	Fail	(probably Out-of-Memory)
int insert_numlist(numberListType *list, int num);

//	Insert a number at the specific position (pos) of the numlist
//	Repeated integer will be detected and stripped if the list is created by create_numlist()
//	If pos exceeds the list size, postition target will be assumed,
//		head-of-list when pos < 0, or
//		end-of-list when pos >= list->size
//	Return Value:
//	1	Success	(even num is found duplicated, and no changes on list)
//	0	Fail	(probably Out-of-Memory)
int insert_numlist_at_pos(numberListType *list, int pos, int num);

//	Insert a number at the beginning of the numlist
//	Repeated integer will be detected and stripped if the list is created by create_numlist()
//	Return Value:
//	1	Success	(even num is found duplicated, and no changes on list)
//	0	Fail	(probably Out-of-Memory)
//	NOTE: This is alias to insert_numlist_at_pos(list, 0, num)
int unshift_numlist(numberListType *list, int num);

//	Remove the specified number from the numlist (if exists)
//	Return Value:
//	1	Success	(even num is originally not on the list)
//	0	Fail	(probably list is NULL)
int remove_numlist(numberListType *list, int num);

//	Insert numbers from the second list into end of the first list
//	Repeated integer will be ignored if the list is created by create_numlist()
//	Return Value:
//	1	Success (even num is found duplicated, and no changes on the first list)
//	0	Fail	(probably Out-of-Memory)
int union_numlist(numberListType *list, const numberListType *transient_list);

//	Remove numbers in the first list that is absent in the second list
//	Return Value:
//	1	Success
//	0	Fail	(probably Out-of-Memory)
int intersect_numlist(numberListType *list, const numberListType *transient_list);

//	Remove numbers in the first list that is also found in the second list
//	Return Value:
//	1	Success
//	0	Fail	(probably Out-of-Memory)
int except_numlist(numberListType *list, const numberListType *transient_list);

//	Suggest a *positive* integer not within the numlist
//	This value is typically the smallest possible result (but don't make this assumption)
//	Return Value:
//	n >= 1	Suggested vacant
//	0	Fail	(This function never *suggest* zero, and this return value means a failure)
//	NOTE: This function will never suggest "0", by design, even "0" is a legitimate value to be stored / used in numlist
int get_free_numlist(const numberListType *list);

//	Check if the specified number is inside the numlist
//	Return Value:
//	1	Checked
//	0	Nope
//	NOTE: If list is bad, this function will simply return 0
int exist_numlist(const numberListType *list, int num);

//	Check if the number list allows multiple entries of same value
//	Return Value:
//	1	Allows storing duplicated / multiple same items
//	0	Ignore duplicated / multiple same items on insertion
int is_numlist_multiple(const numberListType *list);

//	Update the setting if the list allows multiple entries of same value
//	NOTE: To set allow_multiple to 0, the list may under go deduplication
//		to ensure it won't contains multiple entries of same value
//	Return Value:
//	1	Success (This function will not fail)
int set_numlist_multiple(numberListType *list, int allow_multiple);

//	Deduplication of the list, which is supporting multiple entries of
//	same value
//	NOTE 1: If the list does not support multiple, no action will be taken
//	NOTE 2: Multiple support remains unchanged, use set_numlist_multiple()
//		if a change of multiple support is desired
//	Return Value:
//	1	Success (This function will not fail)
int unique_numlist(numberListType *list);

//	Sort the list in ascending order, descending order, or in custom
//	order of your choice
//	Return Value:
//	1	Success
//	0	Fail
int sort_numlist(numberListType *list);
int sort_numlist_reverse(numberListType *list);
int sort_numlist_ex(numberListType *list, int (*compar)(const void *, const void *));

//	Reorder the list, gathered ordered fragment (from the list),
//	immediately follows the last list item among ref.
//	If there is no ref (NULL), the fragment should put just before the
//	rest of the list
//
//	The supplied fragment and ref is being flexibly treated, and any
//	excessive items not among the list would be gracefully ignored;
//	However, ref, whenever defined, must contain at least one item
//	from list to serve as an effective reference position
//
//	Return Value:
//	1	Success
//	0	Fail
//
//	NOTE: This function may fail in these specific situations below,
//	- None of the supplied ref are inside the list
//	- Overlapping list items found among both fragment and ref
//
//	Sample Usage:
//
//	numberListType *list = create_numlist("1 2 3 4 5 6 7 8 9");
//	numberListType *fragment = create_numlist("8 101 6 4 102 2");
//	numberListType *ref = create_numlist("5 7 103 3");
//	reorder_numlist(list, fragment, ref);
//	=> Resulting list would be:
//	[ 1 3 5 7 8 6 4 2 9 ]
//          ^ ^ ^ <.....> fragment inserted, right after all items in ref
//	    ref, their position remains unchanged
//	101, 102, 103 from fragment / ref are bad, not on the list,
//	but will be ignored and not affecting the reorder result
int reorder_numlist(numberListType *list, numberListType *fragment, numberListType *ref);

//	Generate the serialized string format of the numlist, using the newly allocated memory space
//	NOTE: You are responsible to free() the result returned by seriailize_numlist()
char *serialize_numlist(const numberListType *list);

static inline int valid_numlist(const numberListType *l)
{
	return l && l->size && l->item;
}

// Obsolete function - will be removed eventually
#define	create_multiple_numlist(s)	create_numlist_multiple(s)


#include "iplist.h"

//	Create a numlist from an IP list.
//	Accept multiple values if and only if the supplied IP list accepts
//	Non-numeric content values from the IP list will be ignored
numberListType *create_numlist_from_iplist(ipListType *list);


#endif	/* __NUMLIST_H__ */
