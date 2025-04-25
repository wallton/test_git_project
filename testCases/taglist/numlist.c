#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "numlist.h"

#define	STRUTILS_NUMLIST_MEMBLK_SIZE	256
#define STRUTILS_NUMLIST_FREE_SLOT_MAX	1024

// Mimic the vscnprintf() / scnprintf() from Kernel
int vscnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int n = 0 < (int) size ? vsnprintf(str, size, format, ap) : 0;
	return n <= 0 ? 0 : (n < size ? n : size - 1);
}
int scnprintf(char *str, size_t size, const char *format, ...)
{
	int n;
	{
		va_list ap;
		va_start(ap, format);
		n = vscnprintf(str, size, format, ap);
		va_end(ap);
	}
	return n;
}

static int __exist_numlist(const numberListType *list, int num, int upper_bound)
{
	if (list) {
		int i_max = 0 < upper_bound && upper_bound <= list->size ?
			upper_bound : list->size;
		int i;

		for (i = i_max - 1; i >= 0; i--) {
			if (list->item[i] == num) {
				return i;	// Exists
			}
		}
	}
	return -1;	// Not exists
}

static int __append_numlist(numberListType *list, int num)
{
	if (!list) {
		return 0;	// OOM
	}
	if (list->memsize <= list->size) {
		int memsize_step = STRUTILS_NUMLIST_MEMBLK_SIZE;
		int *item = realloc(list->item, sizeof(int) *
			(list->memsize + memsize_step));

		if (!item) {
			return 0;	// OOM
		}
		list->item = item;
		list->memsize += memsize_step;
	}
	list->item[list->size++] = num;
	return 1;	// Success
}

static int __remove_numlist_at_pos(numberListType *list, int pos)
{
	if (!list) {
		return 0;	// OOM
	}
	if (0 <= pos && pos < list->size) {
		int i, i_max;
		for (i = pos + 1, i_max = list->size; i < i_max; i++) {
			list->item[i - 1] = list->item[i];
		}
		list->size--;
	}
	return 1;	// Success
}

static numberListType *__init_numlist(int size, int allow_multiple)
{
	numberListType *list = calloc(1, sizeof(numberListType));
	if (list) {
		list->allow_multiple = allow_multiple ? 1 : 0;
		if (size > 0) {
			int *item = malloc(sizeof(int) * size);
			if (item) {
				list->item = item;
				list->memsize = size;
			}
		}
	}
	return list;
}

static numberListType *__create_numlist(const char *str, int allow_multiple)
{
	int is_oom = 0;
	numberListType *numlist = NULL;

	// Initialize a numlist structure
	if (!is_oom) {
		numlist = __init_numlist(0, allow_multiple);
		if (!numlist) {
			is_oom = 1;
		}
	}

	// Parse the string and prepare the numlist
	if (!is_oom && str && str[0]) {
		struct DDM ddm;
		int pos = 0;
		int strz = strlen(str);

		if (!allow_multiple) {
			__DDM_init(&ddm);
		}
		while (pos < strz) {
			int v = -1, n = 0;

			if (sscanf(str + pos, "%d%n", &v, &n) != 1 || v < 0) {
				break;
			}
			pos += n + 1;
			// De-duplication Logic
			if (!allow_multiple) {
				switch (__DDM_register(&ddm, v)) {
				case DDM_UNSET:
					break;
				case DDM_SET:
					// Duplicated, skip ahead
					continue;
				case DDM_OOB:
				default:
					// Not covered by DDM, use slow checker
					if (__exist_numlist(numlist, v, 0) != -1) {
						// Duplicated, skip ahead
						continue;
					}
					break;
				}
			}
			if (!__append_numlist(numlist, v)) {
				is_oom = 1;
				break;
			}
		}
	}

	// Out-of-Memory handling
	if (is_oom) {
		free_numlist(numlist);
		numlist = NULL;
	}

	return numlist;
}


numberListType *create_numlist(const char *numlist_str)
{
	return __create_numlist(numlist_str, 0);
}

numberListType *create_numlist_multiple(const char *numlist_str)
{
	return __create_numlist(numlist_str, 1);
}



int exist_numlist(const numberListType *list, int num)
{
	return __exist_numlist(list, num, 0) == -1 ? 0 : 1;
}

int unique_numlist(numberListType *list)
{
	if (list && list->allow_multiple) {
		struct DDM ddm;
		int count, i;

		__DDM_init(&ddm);
		for (count = 0, i = count; i < list->size - 1; i++) {
			switch (__DDM_register(&ddm, list->item[i])) {
			case DDM_UNSET:
				break;
			case DDM_SET:
				// Duplicated, skip over
				continue;
			case DDM_OOB:
			default:
				// Not covered by DDM, use slow checker
				if (__exist_numlist(list, list->item[i], count) != -1) {
					// Duplicated, skip over
					continue;
				}
				break;
			}

			if (i != count) {
				list->item[count] = list->item[i];
			}
			count++;
		}
		if (list->size != count) {
			list->size = count;
		}
	}
	return 1;
}

int is_numlist_multiple(const numberListType *list)
{
	return list && list->allow_multiple ? 1 : 0;
}

int set_numlist_multiple(numberListType *list, int allow_multiple)
{
	if (list && (allow_multiple ^ list->allow_multiple)) {
		if (!allow_multiple) {
			unique_numlist(list);
		}
		list->allow_multiple = allow_multiple ? 1 : 0;
	}
	return 1;
}

int sort_numlist_ex(numberListType *list, int (*compar)(const void *, const void *))
{
	if (!compar) {
		return 0;
	}
	if (list && list->size > 1) {
		qsort(list->item, list->size, sizeof(int), compar);
	}
	return 1;
}

static int __numlist_compar_asc(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}
int sort_numlist(numberListType *list)
{
	return sort_numlist_ex(list, __numlist_compar_asc);
}

static int __numlist_compar_dsc(const void *a, const void *b)
{
	return *(int *)b - *(int *)a;
}
int sort_numlist_reverse(numberListType *list)
{
	return sort_numlist_ex(list, __numlist_compar_dsc);
}

int reorder_numlist(numberListType *list, numberListType *fragment, numberListType *ref)
{
	int is_oom = 0;	// Out-of-methodology flag
	numberListType *chain = NULL;

	// Validation on list
	if (!is_oom) {
		if (!(list && !list->allow_multiple)) {
			// Fail, list must be a valid non-multiple number list
			is_oom = 1;
		}
	}

	// Clone copy of fragment, for the operation need
	if (!is_oom && fragment && fragment->size) {
		chain = create_numlist("");
		union_numlist(chain, fragment);
		intersect_numlist(chain, list);
	}

	// Validation on supplied ref
	if (!is_oom && chain && chain->size && ref && ref->size) {
		int has_ref = 0;
		int i;

		for (i = 0; i < ref->size; i++) {
			if (exist_numlist(chain, ref->item[i])) {
				// Overlapping list items found
				//	among fragment and ref
				is_oom = 1;
				break;
			} else if (!has_ref &&
				exist_numlist(list, ref->item[i])) {
				// At least one of the reference is
				// observed and qualified
				has_ref = 1;
			}
		}
		if (!is_oom && !has_ref) {
			// None of the reference can be observed
			is_oom = 1;
		}
	}

	// Perform re-order of fragment from list, to the position suggested
	if (!is_oom && chain && chain->size) {
		int ref_pos = -1;
		int i;

		// Partition list and chain as mutually exclusive copy
		// NOTE: list is "destroyed" from now on, and we have no
		//	space to get failure result anymore
		// The show must go on...
		except_numlist(list, chain);

		if (ref && ref->size) {
			for (i = list->size - 1; i >= 0; i--) {
				if (exist_numlist(ref, list->item[i])) {
					ref_pos = i;
					break;
				}
			}
		}

		// Piece together the frgament inside the list
		// i.e.
		// [ list ...	X, [ chain ] ... ]
		//		^ref_pos
		// If ref_pos = -1, put [ chain ] before the list
		// [ chain ] [ list ]

		// Just to ensure it is as big,
		//	trialing content is not a concern this moment
		union_numlist(list, chain);
		for (i = list->size - 1 - chain->size; i > ref_pos; i--) {
			// Relocate content after ref_pos to the very
			//	end of the enlarged list
			list->item[i + chain->size] = list->item[i];
		}
		for (i = 0; i < chain->size; i++) {
			// Fill in the list from ref_pos all content
			// inside chain
			list->item[ref_pos + 1 + i] = chain->item[i];
		}
	}

	free_numlist(chain);

	return is_oom ? 0 : 1;
}

int insert_numlist(numberListType *list, int num)
{
	int is_oom = 0;

	if (!list) {
		is_oom = 1;
	}
	if (!is_oom && (list->allow_multiple || !exist_numlist(list, num))) {
		if (!__append_numlist(list, num)) {
			is_oom = 1;
		}
	}
	return is_oom ? 0 : 1;
}

int insert_numlist_at_pos(numberListType *list, int pos, int num)
{
	int is_oom = 0;

	if (!insert_numlist(list, num)) {
		is_oom = 1;
	}
	if (!is_oom) {
		// Shift target to the list pos
		int cursor = __exist_numlist(list, num, 0);
		int direction;

		if (list->size <= pos) {
			pos = list->size - 1;
		}
		if (pos < 0) {
			pos = 0;
		}
		direction = pos < cursor ? -1 : (pos == cursor ? 0 : 1);
		if (direction != 0 &&
			0 <= pos && pos < list->size &&
			0 <= cursor && cursor < list->size) {
			for (; cursor != pos; cursor += direction) {
				list->item[cursor] =
					list->item[cursor + direction];
			}
			list->item[cursor] = num;
		}
	}
	return is_oom ? 0 : 1;
}

int unshift_numlist(numberListType *list, int num)
{
	return insert_numlist_at_pos(list, 0, num);
}

int remove_numlist(numberListType *list, int num)
{
	if (!list) {
		return 0;	// OOM
	}
	do {
		int pos = __exist_numlist(list, num, 0);

		if (pos == -1) {
			break;
		}
		__remove_numlist_at_pos(list, pos);
	} while (list->allow_multiple);
	return 1;	// Ensured removal
}

int union_numlist(numberListType *list, const numberListType *transient_list)
{
	int is_oom = 0;

	if (!list) {
		is_oom = 1;
	}
	if (!is_oom && transient_list && transient_list->size) {
		int i, i_max;

		for (i = 0, i_max = transient_list->size; i < i_max; i++) {
			if (!insert_numlist(list, transient_list->item[i])) {
				is_oom = 1;
				break;
			}
		}
	}
	return is_oom ? 0 : 1;
}

// Internal Handling of number list intersection (yin) and Except / Minus (yan)
//	yin - Intersection	(yin = 1)
//	yan - Except / Minus	(yin = 0)
static int yinyan_numlist(numberListType *list, const numberListType *transient_list, int yin)
{
	int is_oom = 0;

	if (!list) {
		is_oom = 1;
	}
	if (!is_oom && list->size) {
		int i, is_yin;

		yin = yin ? 1 : 0;	// Ensure yin is either 0 or 1
		for (i = list->size - 1; i >= 0; i--) {
			is_yin = exist_numlist(transient_list, list->item[i]);
			if (yin ^ is_yin) {
				__remove_numlist_at_pos(list, i);
			}
		}
	}
	return is_oom ? 0 : 1;
}
int intersect_numlist(numberListType *list, const numberListType *transient_list)
{
	return yinyan_numlist(list, transient_list, 1);
}
int except_numlist(numberListType *list, const numberListType *transient_list)
{
	return yinyan_numlist(list, transient_list, 0);
}

char *serialize_numlist(const numberListType *list)
{
	char *result = NULL;

	if (list && list->size > 0) {
		int i, i_max, n;
		int resultz;

	//	Estimate the required memory of the serialized result
		n = 0;
		for (i = list->size - 1; i >= 0; i--) {
			if (n < list->item[i]) {
				n = list->item[i];
			}
		}
		resultz = list->size * snprintf(NULL, 0, " %d", n);

	//	Reserve required memory
		result = malloc(sizeof(char) * resultz);
		if (!result) {
			return NULL;	// OOM;
		}

	//	Serialize the elements
		i_max = list->size;
		i = 0;
		n = scnprintf(result, resultz, "%d", list->item[i++]);
		while (i < i_max) {
			n += scnprintf(result + n, resultz - n, " %d", list->item[i++]);
		}

	}
	return result ? result : strdup("");
}

//	Refreshed implementation of get_free_numlist(),
//	to obtain an unused numbert(N) from list, where N >= 1
//		by Kenny Kwok, 2010/04/29
//
//	Theory behind:
//	If each item are all occupied between slot(1) and slot(size(list)),
//		the smallest free vacant would be slot(size(list) + 1);
//	If slot(size(list) + 1) is occupied,
//		or any slot other than between slot(1) and slot(size(list)) is occupied,
//		from pigeonhole principle,
//		there must be free vacant between slot(1) and slot(size(list))
//	=> There exists at least one free vacant between slot(1) and slot(size(list) + 1)
//
//	Updated Algo., by Kenny Kwok, 2014/06/23
//	1. Generate Map, mapz = (size(list) + 1) + 1
//	map | 0 | 1 | 2 | ... | size(list) | size(list) + 1 |
//	val | 0 | ? | ? | ... | ?          | 0              |
//	NOTE:	map[0] is ignore, as it is not out target
//		map[size(list) + 1] always contain value 0,
//		and should never get dirty (even it is occupied in the list)
//	2. Walk Map from (1), skip according to val (as skip ahead step count),
//		until we obtain val == 0 (i.e. free slot)
int get_free_numlist(const numberListType *list)
{
	int i = 0;

	if (list) {
		int static_map[STRUTILS_NUMLIST_FREE_SLOT_MAX];	//	Static Map for general purpose
		int *dynamic_map = NULL;			//	Dynamic Map if the list size >= STRUTILS_NUMLIST_FREE_SLOT_MAX
		int *map = static_map;
		int mapz;

	// Select usage map container for the operation
		mapz = list->size + 2;	// Map holder requires slots between [0, size(list) + 1]
		if (mapz >= STRUTILS_NUMLIST_FREE_SLOT_MAX) {
			map = dynamic_map = calloc(mapz, sizeof(int));
			if (!map) {
				// FIXME: we need to define better return value
				return 0;
			}
		} else {
			memset(map, 0, sizeof(int) * mapz);
		}

	// Create usage map pattern
		for (i = list->size - 1; i >= 0; i--) {
			if (list->item[i] + 1 < mapz) {
				map[list->item[i]] = map[list->item[i] + 1] + 1;
			}
		}

	// Discover the smallest available number
		for (i = 1; i < mapz && map[i] > 0; i += map[i]) {
			// I really have nothing to do inside
		}

		if (dynamic_map) {
			free(dynamic_map);
		}
	}

	return i;
}

void free_numlist(numberListType *list)
{
	if (list) {
		list->size = 0;
		list->memsize = 0;
		free(list->item);
		list->item = NULL;
	}
	free(list);
}



static numberListType *__create_numlist_from_iplist(ipListType *list, int allow_multiple)
{
	int is_oom = 0;
	numberListType *numlist = NULL;

	if (!is_oom) {
		int size = list ? list->ipz : 0;
		numlist = __init_numlist(size, allow_multiple);
		if (!numlist) {
			is_oom = 1;
		}
	}

	if (!is_oom && list && list->ipz) {
		struct DDM ddm;
		int i;

		if (!allow_multiple) {
			__DDM_init(&ddm);
		}
		for (i = 0; i < list->ipz; i++) {
			char *s = list->ip[i];
			int v = strtol(s, NULL, 10);

			// Screening of non-numeric items
			if (!(v > 0 || (v == 0 && strcmp(s, "0") == 0))) {
				continue;
			}
			// De-duplication Logic
			if (!allow_multiple) {
				switch (__DDM_register(&ddm, v)) {
				case DDM_UNSET:
					break;
				case DDM_SET:
					// Duplicated, skip ahead
					continue;
				case DDM_OOB:
				default:
					// Not covered by DDM, use slow checker
					if (__exist_numlist(numlist, v, 0) != -1) {
						// Duplicated, skip ahead
						continue;
					}
					break;
				}
			}
			if (!__append_numlist(numlist, v)) {
				is_oom = 1;
				break;
			}
		}
	}

	// Out-of-Memory handling
	if (is_oom) {
		free_numlist(numlist);
		numlist = NULL;
	}

	return numlist;
}
numberListType *create_numlist_from_iplist(ipListType *list)
{
	int allow_multiple = list ? list->allow_multiple : 0;
	return __create_numlist_from_iplist(list, allow_multiple);
}

