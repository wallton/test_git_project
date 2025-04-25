#ifndef __TAGLIST_H__
#define __TAGLIST_H__

#include "iplist.h"

// Available mode for function save_taglist_ex()
#define	LIBSTRUTILS_TAGLIST_AUTO	0	// Try automic save, and switch to legacy way if failed to save automically
#define	LIBSTRUTILS_TAGLIST_ATOMIC	1	// Atomic save by writing to temp file, requires write permission to the destination directory
#define	LIBSTRUTILS_TAGLIST_QUICK	2	// Legacy way, simple open/write (overwrite) existing file
#define	LIBSTRUTILS_TAGLIST_STDOUT	3	// Simply write into stdout

// Tagged String List ADT
typedef struct tagItemType
{
	char *tag;
	char *value;
	int removed;	// for future support
} tagItemType;

typedef struct tagListType
{
	int size;
	int memsize;
	int sorted;
	tagItemType **item;
	char *restricted_prefix;
	char *filename;
	time_t file_timestamp;
	time_t read_timestamp;
} tagListType;

///	Create a taglist with content from the specified file, which will be pre-sorted if should_sort is 1.
///	If pfx is defined, load only the tags with this specific prefix string
tagListType *load_taglist_ex(const char *filename, int should_sort, const char *pfx, ...);
#define load_taglist(filename)	load_taglist_ex((filename), 1, NULL)
#define load_taglist_prefix(filename, ...)	load_taglist_ex((filename), 1, __VA_ARGS__)

///	Load taglist from bare string (in case the source is not load from file)
tagListType *unserialize_taglist_ex(char *tagline_str, int should_sort, const char *pfx, ...);
#define unserialize_taglist(tagline_str)	\
		unserialize_taglist_ex((tagline_str), 1, NULL)
#define unserialize_taglist_prefix(tagline_str, ...)	\
		unserialize_taglist_ex((tagline_str), 1, __VA_ARGS__)

//	Save the taglist with the specified filename
int save_taglist_ex(const tagListType *list, const char *filename, int mode);
#define save_taglist(list, filename)	save_taglist_ex((list), (filename), LIBSTRUTILS_TAGLIST_ATOMIC)

///	Create an empty taglist
tagListType *create_taglist(void);

///	Free the taglist object
void free_taglist(tagListType *list);

///	Reload the taglist from the source file (on demand) if the file is updated according to modified timestamp
///	If the file is not modified, no actual reload will be done
int reload_taglist(tagListType *list);

///	Explicit perform sorting on the taglist. The taglist is remain sorted on subsequent changes
int sort_taglist(tagListType *list);

///	Explicit mark this taglist as unsorted. The taglist will no longer remain sorted on subsequent changes
int unsort_taglist(tagListType *list);

///	Update the taglist with value on the specified tag, denoted in printf liked format
///	If the tag is already exists, the value of the existing tag will be replaced
///	If the value is NULL or empty string, the tag (if exists) will be removed right away
int update_taglist_ex(tagListType *list, const char *value, const char *fmt, ...);

///	Check the taglist of the specified tag, denoted in printf liked format, exists
int exist_taglist(const tagListType *list, const char *fmt, ...);

///	Fetch the pointer to value of the taglist's specified tag, denoted in printf liked format.
///	Default value will be used if the tag does not exist, or its value is empty
const char *get_taglist_default_ex(const tagListType *list, const char *default_value, const char *fmt, ...);

///	Short form of get_taglist_default_ex(), when no need to customize the default value
#define get_taglist(list, ...)	get_taglist_default_ex((list), "", __VA_ARGS__)

/*	Compare two taglist content.
 *	return 0 = no diff, return 1 = diff
 */
int compare_taglist(const tagListType *list1, const tagListType *list2);

///	(deprecated) You are not required to call this function normally
tagItemType *create_tagitem(const char *tag, const char *value) __attribute__ ((deprecated));
///	(deprecated) You are not required to call this function normally
void free_tagitem(tagItemType *item) __attribute__ ((deprecated));
///	(deprecated) You are not required to call this function normally
int insert_taglist(tagListType *list, tagItemType *tagItem) __attribute__ ((deprecated));
///	(deprecated) Replaced by update_taglist_ex()
int replace_taglist(tagListType *list, tagItemType *tagItem) __attribute__ ((deprecated));
///	(deprecated) Replaced by update_taglist_ex()
#define	remove_taglist(list, ...)		update_taglist_ex((list), "", __VA_ARGS__)
///	(deprecated) Replaced by update_taglist_ex()
static int update_taglist(tagListType *list, const char *tag, const char *value) __attribute__ ((deprecated));
inline static int update_taglist(tagListType *list, const char *tag, const char *value)
{
	return update_taglist_ex(list, value, tag);
}
///	(deprecated) You are not required to call this function normally
char *serialize_taglist(const tagListType *list) __attribute__ ((deprecated));

//	(obsolete) You should not use dup_xxx in this library anymore!
///	If you need a new reference copy of the retrieved value, please do the strdup() yourself
#define dup_taglist_default_ex(list, def_val, ...)	strdup(get_taglist_default_ex((list), (def_val), __VA_ARGS__))
#define dup_taglist(list, ...)	dup_taglist_default_ex((list), "", __VA_ARGS__)
#define dup_taglist_default(list, tag, def_val)	dup_taglist_default_ex((list), (def_val), tag)
//	(obsolete) Replaced by get_taglist_default_ex()
#define	get_taglist_default(list, tag, def_val)	get_taglist_default_ex((list), (def_val), tag)

#endif	/* __TAGLIST_H__ */
