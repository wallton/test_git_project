#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>  /* struct stat */
#include <time.h>     /* time */
#include <limits.h>  /* PATH_MAX */
#include <unistd.h> /* unlink */

#include "taglist.h"


// Support of over-long line, with length exceeding STRUTILS_TAGLIST_MAX_LINE_LENGTH limit
#define	STRUTILS_OVERLONG_SUPPORT		1

#define	STRUTILS_TAGLIST_MAX_LINE_LENGTH	8192	// This should be more than enough!
#define	STRUTILS_TAGLIST_MEMBLK_SIZE		1024
#define	STRUTILS_TAGLIST_MEMSTEP_SIZE		256
#define	STRUTILS_TAGLIST_BUF_SIZE		    1024

#define __STRUTILS_VSNPRINTF(s, sz, fmt) { va_list ap; va_start(ap, (fmt)); vsnprintf((s),(sz),(fmt),ap); va_end(ap); }

#define	__STRUTILS_PREP_TAG(TAG, FMT, BUF)	{\
	TAG = FMT;\
	if (strchr(FMT, '%'))\
	{\
		__STRUTILS_VSNPRINTF(BUF, sizeof(BUF), FMT);\
		TAG = BUF;\
	}\
}


//	Approximate Binary Search by Interative Approach
//	Note: This function even outperforms native bsearch() if compiled with -O2
#define __bsearch_pos(key, base, nmemb, size, compar) __bs_i((key), (base), 0, (nmemb)-1, (size), (compar))

static inline int __bs_i(const void *v, const void *num, int low, int high, size_t size, int (*compar)(const void *, const void *))
{
	while (low < high) {
		int mid = (high + low) / 2;
		int r = compar(v, num + mid * size);

		if (r == 0) {		//	(v == num[mid])
			// All Cases, v is match
			return mid;	// exact match, let's jump out!
		} else if (r < 0) {
			//	(v < num[mid])
			high = mid - 1;	// high for next cycle
		} else {
			// if (r > 0)	//	(v > num[mid])
			low = mid + 1;	// low for next cycle
		}
	}
	if (low == high && compar(v, num + low * size) > 0) {
		// Case 1, v is bigger
		return low + 1;
	}
	return low;
}

int bsearch_pos(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	return __bsearch_pos(key, base, nmemb, size, compar);
}

void *bsearch_fast(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	int pos = bsearch_pos(key, base, nmemb, size, compar);

	return compar(key, base + pos) == 0 ? (void *)base + pos : NULL;
}

static inline time_t __get_mtime(const char *filename)
{
	struct stat st;

	return stat(filename, &st) == 0 ? st.st_mtime : 0;
}

// Tag Item Handling - Internal

static void __free_tagitem(tagItemType *item)
{
	if (item) {
		free(item->tag);
		free(item->value);
	}
	free(item);
	return;
}

static tagItemType *__create_tagitem(const char *tag, const char *value)
{
	tagItemType *item = NULL;

	if (tag && value) {
		item = malloc(sizeof(tagItemType));
	}
	if (item) {
		item->tag = strdup(tag);
		item->value = strdup(value);
		item->removed = 0;	// For lazy deletion support
		if (!(item->tag && item->value)) {
			// Unfortunate victims goes here.
			__free_tagitem(item);
			item = NULL;
		}
	}
	return item;
}



// Capture Tag/Value Element
//
//	Input:
//		tagline_str, Tag="Value"
//	Output:
//		tag	<- Tag
//		value	<- Value
//
//	=== NOTE ============================================================
//	This function replaces parse_tagitem(), providing the same functionality,
//	but may actively alter / destroy the input "tagline_str" variable,
//	hoping for better efficiency when compare with old parse_tagitem() function
//	by Kenny Kwok, 2010/05/05
//	=====================================================================
static tagItemType *__parse_tagitem(char *tagline_str, const char *restricted_prefix, int restricted_prefixz)
{
	tagItemType *result = NULL;
	char *s = tagline_str;
	char *p = NULL, *eol_p = NULL, eol_v = 0;

	if (s && *s && *s != '#') {		// Assume	s s s s s s s s s s
		int e = strlen(s);		// Locate	s s s s s s s s s "

		while (	e > 0 &&
			s[e] != '"' &&
			(s[e - 1] == '\n' || s[e - 1] == '\r' || s[e - 1] == '"')) {
			e--;
		}
		if (e >= 0 && s[e]) {		// Rewrite	s s s s s s s s s \0
			eol_p = &(s[e]);
			eol_v = *eol_p;
			*eol_p = 0;
		}
		p = strchr(s, '=');		// Locate       s s s = . . . . . \0
	}
	if (p && *p == '=') {
		char *eq_p = p++;		// Locate	s s s = p p p p p \0

		*eq_p = 0;			// Rewrite	s s s \0p p p p p \0
		if (*p == '"') {		// Locate	s s s \0" p p p p \0
			p++;
		}
		if (restricted_prefixz &&
			strncmp(s, restricted_prefix,
			restricted_prefixz) != 0) {
			// Skip over unmatched prefix, if it is specified
		} else {
			result = __create_tagitem(s, p);
		}
		*eq_p = '=';			// Restore	s s s = . . . . . \0
	}
	if (eol_p && eol_v) {
						// Restore	s s s s s s s s s "
		*eol_p = eol_v;
	}
	return result;
}

static int __insert_taglist(tagListType *list, tagItemType *tagItem);

static void __empty_taglist(tagListType *list)
{
	if (list) {
		tagItemType **items = list->item;

		while (list->size) {
			__free_tagitem(items[--(list->size)]);
		}
		list->file_timestamp = 0;
		list->read_timestamp = 0;
	}
	return;
}

int reload_taglist(tagListType *list)
{
	int has_reload = 0;
	time_t file_timestamp, read_timestamp;

	if (!(list && list->filename)) {
		// List is wrong or does not support reload
		return has_reload;
	}

	file_timestamp = __get_mtime(list->filename);
	read_timestamp = time(NULL);
	if (file_timestamp == list->file_timestamp) {
		if (list->read_timestamp != list->file_timestamp) {
			// File is not changed
			return has_reload;
		}
		if (read_timestamp == list->read_timestamp) {
			// Rapid calls within same second will never reload
			return has_reload;
		}
	}

//	Reload from file
	{
		char linestr[STRUTILS_TAGLIST_MAX_LINE_LENGTH + 1];
		int restricted_prefixz = list->restricted_prefix ?
			strlen(list->restricted_prefix) : 0;
		FILE *fp = fopen(list->filename, "r");
		tagItemType *item;

// Overlong handler
#ifdef	STRUTILS_OVERLONG_SUPPORT
#define	STRUTILS_OVERLONG_BAIT_MARK	'@'
#define	STRUTILS_OVERLONG_PREP(buf) do {\
	(buf)[sizeof(buf) - 1] = STRUTILS_OVERLONG_BAIT_MARK;\
} while (0)
#define	STRUTILS_OVERLONG_CHECK(buf) \
	((buf)[sizeof(buf) - 1] == 0 && \
	(buf)[sizeof(buf) - 2] != '\n')
#define STRUTILS_OVERLONG_PENDING()	(!!(_olbuf))
#define	STRUTILS_OVERLONG_PUSH(BUF) do {\
	if (!_olbuf) {\
		_olbuf = init_iplist_multiple();\
	} \
	insert_iplist(_olbuf, (BUF));\
} while (0)
#define	STRUTILS_OVERLONG_DUMP_AND_FREE(OUT) do {\
	if (_olbuf) {\
		(OUT) = serialize_iplist(_olbuf, "");\
		free_iplist(_olbuf);\
		_olbuf = NULL;\
	} \
} while (0)
		ipListType *_olbuf = NULL;
#else
#define STRUTILS_OVERLONG_PREP(buf)
#endif	// STRUTILS_OVERLONG_SUPPORT

		if (!fp) {
			// File cannot be opened
			return has_reload;
		}

		__empty_taglist(list);
		while (!feof(fp)) {
			// Overlong handling, pre-dirty the buffer
			STRUTILS_OVERLONG_PREP(linestr);
			if (fgets(linestr, sizeof(linestr), fp)) {

#ifdef	STRUTILS_OVERLONG_SUPPORT
				int loop = STRUTILS_OVERLONG_CHECK(linestr);

				if (loop || STRUTILS_OVERLONG_PENDING()) {
					// Overlong segment all push into...
					STRUTILS_OVERLONG_PUSH(linestr);
				}
				if (loop) {
					// Overlong in action, more to read...
					continue;
				}

				if (STRUTILS_OVERLONG_PENDING()) {
					char *value = NULL;

					STRUTILS_OVERLONG_DUMP_AND_FREE(value);
					item = __parse_tagitem(value,
						list->restricted_prefix,
						restricted_prefixz);
					free(value);
				} else
#endif	// STRUTILS_OVERLONG_SUPPORT
				item = __parse_tagitem(linestr,
					list->restricted_prefix,
					restricted_prefixz);

				if (item) {
					// Regards how, whole item is ready
					__insert_taglist(list, item);
				}
			}
		}

		list->file_timestamp = file_timestamp;
		list->read_timestamp = read_timestamp;
		has_reload = 1;
		fclose(fp);
	}
	return has_reload;
}

tagListType *unserialize_taglist_ex(char *tagline_str, int should_sort, const char *pfx, ...)
{
	tagListType *list = create_taglist();

	if (list) {
		int is_oom = 0;

		if (should_sort) {
			// It actally pre-marks the sorting flag
			sort_taglist(list);
		}

		if (pfx && pfx[0]) {
			char buf[255];
			const char *tag = NULL;

			__STRUTILS_PREP_TAG(tag, pfx, buf);
			if (tag && tag[0]) {
				list->restricted_prefix = strdup(tag);
				if (!list->restricted_prefix) {
					is_oom = 1;
				}
			}
		}
		if (!is_oom) {
#ifndef	STRUTILS_OVERLONG_SUPPORT
			char chr_max = 0;
#endif	// STRUTILS_OVERLONG_SUPPORT
			char chr_nl;
			int restricted_prefixz = list->restricted_prefix ?
				strlen(list->restricted_prefix) : 0;
			char *ptr = NULL, *next_ptr = NULL;

			for (ptr = tagline_str; ptr && ptr[0]; ptr = next_ptr) {
				tagItemType *item;

#ifndef	STRUTILS_OVERLONG_SUPPORT
				// Guard for line with more than STRUTILS_TAGLIST_MAX_LINE_LENGTH characters
				// Minic behavior as in reload_taglist
				int len = strlen(ptr);

				if (len > STRUTILS_TAGLIST_MAX_LINE_LENGTH) {
					// Value captured before mark-up
					chr_max = ptr[STRUTILS_TAGLIST_MAX_LINE_LENGTH];
					ptr[STRUTILS_TAGLIST_MAX_LINE_LENGTH] = 0;
				}
#endif	// STRUTILS_OVERLONG_SUPPORT

				// Guard for inspected newline boundary
				next_ptr = strchr(ptr, '\n');
				if (next_ptr) {
					// Value captured before mark-up
					chr_nl = *(++next_ptr);
					*next_ptr = 0;
				}

				item = __parse_tagitem(ptr,
					list->restricted_prefix,
					restricted_prefixz);

				if (item) {
					// Whole item ready
					__insert_taglist(list, item);
				}

				// Guard for inspected newline boundary (restore)
				if (next_ptr) {
					*next_ptr = chr_nl;
					// Value is now restored
				}

#ifndef	STRUTILS_OVERLONG_SUPPORT
				// Guard for line with more than STRUTILS_TAGLIST_MAX_LINE_LENGTH characters (restore)
				if (len > STRUTILS_TAGLIST_MAX_LINE_LENGTH) {
					ptr[STRUTILS_TAGLIST_MAX_LINE_LENGTH] = chr_max;
					// Value is now restored
				}
#endif	// STRUTILS_OVERLONG_SUPPORT
			}
		}
	}
	return list;
}

tagListType *load_taglist_ex(const char *filename, int should_sort, const char *pfx, ...)
{
	tagListType *list = create_taglist();

	if (list) {
		int is_oom = 0;

		if (should_sort) {
			// It actally pre-marks the sorting flag
			sort_taglist(list);
		}
		if (pfx && pfx[0]) {
			char buf[255];
			const char *tag = NULL;

			__STRUTILS_PREP_TAG(tag, pfx, buf);
			if (tag && tag[0]) {
				list->restricted_prefix = strdup(tag);
				if (!list->restricted_prefix) {
					is_oom = 1;
				}
			}
		}
		if (!is_oom) {
			list->filename = strdup(filename);
			if (!list->filename) {
				is_oom = 1;
			}
		}
		if (!is_oom) {
			// Action to proceed
			reload_taglist(list);
		}
		if (is_oom) {
			free_taglist(list);
			list = NULL;
		}
	}
	return list;
}

#define SERIALIZED_PATTERN	"%s=\"%s\"\n"
#define SERIALIZED_OVERHEAD	4

static int dump_taglist(const tagListType *list, FILE *fp)
{
	int is_error = 0;
	int i, i_max;

	if (list && fp) {
		for (i = 0, i_max = list->size; i < i_max; i++) {
			// T=""\n is at least 5 characters
			if (fprintf(fp, SERIALIZED_PATTERN,
				list->item[i]->tag,
				list->item[i]->value) < 5) {
				// Something wrong, the dump taglist must be failed!
				is_error = 1;
				break;
			}
		}
	} else {
		is_error = 1;
	}
	return is_error ? 0 : 1;	// 1: OKAY, 0: FAILURE
}

char *serialize_taglist(const tagListType *list)
{
	char *packed_result = NULL;
	int packed_size = 0;

	// Estimate the serialized memory size
	if (list) {
		int overhead = SERIALIZED_OVERHEAD; // =" "<newline>, There are four overhead per line!
		int i, i_max;

		packed_size = 0;
		for (i = 0, i_max = list->size; i < i_max; i++) {
			packed_size += strlen(list->item[i]->tag) +
				strlen(list->item[i]->value) + overhead;
		}
		packed_size++;	// for NULL terminated character

		// Allocate memory for serialized string to be generated
		packed_result = (char *) malloc(sizeof(char) * packed_size);
	}

	// Generate the serialized string (packed_result)
	if (packed_result) {
		char *ptr = packed_result;
		int i, i_max, n;

		ptr[0] = 0;
		for (i = 0, i_max = list->size; i < i_max; i++) {
			n = sprintf(ptr,
				SERIALIZED_PATTERN,
				list->item[i]->tag, list->item[i]->value);
			ptr += n;
		}
	}

	return packed_result;
}

static int __save_taglist_quick(const tagListType *list, const char *filename)
{
	FILE *fp = NULL;
	int is_success = 0;

	fp = fopen(filename, "w");
	if (fp) {
		is_success = dump_taglist(list, fp) ? 1 : 0;
		fclose(fp);
	}
	return is_success ? 1 : 0;	// 1: Success / 0: Failed
}

// New Implementation, by Kenny Kwok, 2011/02/08
//	protected from file original content corruption during write corruption
//	*not* protected the potential lost of modification during concurrent write
static int __save_taglist_atomic(const tagListType *list, const char *filename)
{
	int is_success = 0;
	char buffer[PATH_MAX];
	const char *realname = realpath(filename, buffer);

	if (!realname) {
		// Just in case, we have a name to start with
		realname = filename;
	}
	if (strlen(realname) < PATH_MAX) {
		FILE *fp = NULL;
		char tempname[PATH_MAX + 10];	// +10 for ".XXXXXX"
						// just big enough!

		snprintf(tempname, sizeof(tempname), "%s.XXXXXX", realname);
		{
			int temp_fd;

			temp_fd = mkstemp(tempname);
			fp = fdopen(temp_fd, "w");
		}
		if (fp) {
			is_success = dump_taglist(list, fp) ? 1 : 0;
			fclose(fp);
			if (is_success) {
				// The next action to proceed
				is_success = rename(tempname, realname) == 0 ? 1 : 0;
			}
			unlink(tempname);	// This is not necessery, but we do our best to clean up, in case
		}
	}
	return is_success ? 1 : 0;	// 1: Success / 0: Failed
}

int save_taglist_ex(const tagListType *list, const char *filename, int mode)
{
	int max_retry = 3;		// Just try something
	int auto_retry_parts = 2;	// Auto will loop through algo. x retry times!
	int is_success = 0;

	if (!(list && filename)) {
		// Screen out access with bad parameters
		return is_success;
	}

	switch (mode) {
	case LIBSTRUTILS_TAGLIST_AUTO:
		max_retry = auto_retry_parts;
		// passthrough
	case LIBSTRUTILS_TAGLIST_ATOMIC:
	default:
		for (; max_retry; max_retry--) {
			is_success = __save_taglist_atomic(list, filename);
			if (is_success) {
				// Done upon success
				return is_success;
			}
		}
		break;
	case LIBSTRUTILS_TAGLIST_STDOUT:
		return dump_taglist(list, stdout);
	}
	switch (mode) {
	case LIBSTRUTILS_TAGLIST_AUTO:
		max_retry = auto_retry_parts;
		// passthrough
	case LIBSTRUTILS_TAGLIST_QUICK:
		for (; max_retry; max_retry--) {
			is_success = __save_taglist_quick(list, filename);
			if (is_success) {
				// Done upon success
				return is_success;
			}
		}
		break;
	default:
		break;
	}
	return is_success;
}

// Create an empty tag list
tagListType *create_taglist(void)
{
	const int default_memsize = STRUTILS_TAGLIST_MEMBLK_SIZE;
	tagListType *taglist = (tagListType *) calloc(1, sizeof(tagListType));

	if (taglist) {
		taglist->item = (tagItemType **) malloc(
			sizeof(tagItemType *) * default_memsize);
		taglist->memsize = taglist->item ? default_memsize : 0;
	}
	return taglist;
}

// Free the memory of the tag list
void free_taglist(tagListType *list)
{
	if (!list) {
		// Screen out access with bad parameters
		return;
	}
	__empty_taglist(list);
	if (list->memsize > 0) {
		list->memsize = 0;
		if (list->item) {
			free(list->item);
			list->item = NULL;
		}
	}
	if (list->filename) {
		free(list->filename);
		list->filename = NULL;
	}
	if (list->restricted_prefix) {
		free(list->restricted_prefix);
		list->restricted_prefix = NULL;
	}
	free(list);
	return;
}

int __tagitem_tag_compare_func(const void *p1, const void *p2)
{
	return strcmp((*(tagItemType **)p1)->tag, (*(tagItemType **)p2)->tag);
}

int sort_taglist(tagListType *list)
{
	qsort(list->item, list->size, sizeof(tagItemType *),
		__tagitem_tag_compare_func);
	list->sorted = 1;
	return 1;	// TRUE;
}

int unsort_taglist(tagListType *list)
{
	list->sorted = 0;
	return 0;	// FALSE
}

//	INTERNAL FUNCTION, Do not expose it to header
static int __insert_taglist_pos(tagListType *list, int pos, tagItemType *tagItem)
{
	// Insert the entry (sure need to ensure memory is available)
	if (list->memsize <= list->size) {
		int increment_size = STRUTILS_TAGLIST_MEMSTEP_SIZE;
		tagItemType **item = realloc(list->item,
			sizeof(tagItemType *) * (list->memsize +
			increment_size));

		if (!item) {
			__free_tagitem(tagItem);
			return 0;	// Out-of-Memory
		}
		list->item = item;
		list->memsize += increment_size;
	}
	if (pos < list->size) {
		memmove(list->item + (pos + 1), list->item + pos,
			sizeof(tagItemType *) * (list->size - pos));
	}
	list->item[pos] = tagItem;
	list->size++;
	return 1;	// True, means sucessfully inserted
}
static int __remove_taglist_pos(tagListType *list, int pos)
{
	tagItemType *item = list->item[pos];

	if (pos + 1 < list->size) {
		if (list->sorted) {
			// Sorted list MUST maintain sort order
			memmove(list->item + pos, list->item + (pos + 1),
				sizeof(tagItemType *) *
				(list->size - (pos + 1)));
		} else {
			// Unsorted list need to perform lazy arrangement only
			list->item[pos] = list->item[list->size - 1];
		}
	}
	list->size--;
	__free_tagitem(item);
	return 1;	// True, means sucessfully removed
}
static int __search_taglist(const tagListType *list, const char *tag)
{
	int pos = -1;

	if (!(list && tag)) {
		// Screen out access with bad parameters
		return pos;
	}

	if (list->sorted) {
		// if sorted, we should use binary search
		int i;
		tagItemType tmp;
		tagItemType *item = &tmp;

		item->tag = (char *)tag;
		i = bsearch_pos(&item, list->item, list->size, sizeof(tagItemType *), __tagitem_tag_compare_func);
		if (i < list->size &&
			__tagitem_tag_compare_func(&item, list->item + i) == 0) {
			pos = i;
		}
	} else {
		// if unsorted (default), use simple linear search of tag
		int i, i_max, tagz = strlen(tag);

		for (i = 0, i_max = list->size; i < i_max; i++) {
			if (list->item[i]->tag[0] == tag[0] &&
				strlen(list->item[i]->tag) == tagz &&
				strcmp(list->item[i]->tag, tag) == 0) {
				pos = i;
				break;
			}
		}
	}
	return pos;
}


// Insert a defined tag item into the tag list
int __insert_taglist(tagListType *list, tagItemType *tagItem)
{
	int pos;
	if (!list) return 0;
	if (list->sorted) {
		// if sorted, we should use binary search, with dedicate position
		pos = bsearch_pos(&tagItem, list->item, list->size,
			sizeof(tagItemType *),
			__tagitem_tag_compare_func);
		if (pos < list->size &&
			__tagitem_tag_compare_func(&tagItem,
			list->item + pos) == 0) {
			__free_tagitem(tagItem);
			return 0;	// No insert is needed
		}
	} else {
		// Ensure no duplicated entry exist
		if (exist_taglist(list, tagItem->tag)) {
			__free_tagitem(tagItem);
			return 0;	// No insert is needed
		}
		pos = list->size;
	}
	return __insert_taglist_pos(list, pos, tagItem);
}

// Remove an tag item into the tag list
static int __remove_taglist(tagListType *list, const char *tag)
{
	int pos = -1;
	if (list && tag) {
		pos = __search_taglist(list, tag);
		if (pos >= 0) {
			// Item found
			return __remove_taglist_pos(list, pos);
		}
	}
	return 0;	// Not found
}

// Update a defined tag item into the tag list
static int __replace_taglist(tagListType *list, tagItemType *tagItem)
{
	int pos;

	if (!(list && tagItem)) {
		return 0;
	}
	if (list->sorted) {
		// if sorted, we should use binary search, with dedicate position
		pos = bsearch_pos(&tagItem, list->item, list->size,
			sizeof(tagItemType *),
			__tagitem_tag_compare_func);
		if (pos < list->size &&
			__tagitem_tag_compare_func(&tagItem,
			list->item + pos) == 0) {
			// Exact match, just an item replacement is okay
			__free_tagitem(list->item[pos]);
			list->item[pos] = tagItem;
			return 1;
		}
	} else {
		// if not sorted, we try remove if exist, then append to the end
		__remove_taglist(list, tagItem->tag);	// Remove if already exist
		pos = list->size;
	}
	return __insert_taglist_pos(list, pos, tagItem);
}

int update_taglist_ex(tagListType *list, const char *value, const char *fmt, ...)
{
	char buf[255];
	const char *tag = NULL;

printf("(billy)[RA=%p] tag(%s)/value(%s)\n", __builtin_return_address(0), fmt, value);

	__STRUTILS_PREP_TAG(tag, fmt, buf);
	return value && value[0] ?
		__replace_taglist(list, __create_tagitem(tag, value)) :
		__remove_taglist(list, tag);
}

// Check if the tag item is exist
int exist_taglist(const tagListType *list, const char *fmt, ...)
{
	char buf[255];
	const char *tag = NULL;

	__STRUTILS_PREP_TAG(tag, fmt, buf);
	return __search_taglist(list, tag) >= 0 ? 1 : 0;
}

/* Compare taglist content */
static int compare_tagitem(tagItemType *a, tagItemType *b)
{
	return strcmp(a->tag, b->tag) == 0 &&
		strcmp(a->value, b->value) == 0 ? 0 : 1;
}

int compare_taglist(const tagListType *a, const tagListType *b)
{
	int is_different = 0;
	int i;

	// NOTE: It doesn't work if we have lazy deletion (removed=1)
	//	(This is not implemented anyway!)
	if (a->size != b->size) {
		// They are NOT having same content!
		is_different = 1;
	} else if (a->sorted && b->sorted) {
		// Direct compare on sorted items
		for (i = a->size - 1; i >= 0; i--) {
			if (compare_tagitem(a->item[i], b->item[i]) != 0) {
				is_different = 1;	// Does not match!
				break;
			}
		}
	} else {
		// Slow compare on unsorted items
		for (i = a->size - 1; i >= 0; i--) {
			if (strcmp(get_taglist(b, a->item[i]->tag),
				a->item[i]->value) != 0) {
				is_different = 1;	// Does not match!
				break;
			}
		}
	}
	return is_different;
}

// Obtain the value of the tag item (output internal pointer, please read only!)
const char *get_taglist_default_ex(const tagListType *list, const char *default_value, const char *fmt, ...)
{
	char *result = NULL;
	char buf[255];
	const char *tag = NULL;

	__STRUTILS_PREP_TAG(tag, fmt, buf);
	{
		int pos = __search_taglist(list, tag);

		if (pos >= 0) {
			result = list->item[pos]->value;
		}
	}
	return result && result[0] ? result : default_value;
}

// Obsolete Function
tagItemType *create_tagitem(const char *tag, const char *value)
{
	return __create_tagitem(tag, value);
}
void free_tagitem(tagItemType *item)
{
	return __free_tagitem(item);
}
int insert_taglist(tagListType *list, tagItemType *tagItem)
{
	return __insert_taglist(list, tagItem);
}
int replace_taglist(tagListType *list, tagItemType *tagItem)
{
	return __replace_taglist(list, tagItem);
}


