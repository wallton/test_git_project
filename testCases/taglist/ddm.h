#ifndef __DDM_H__
#define __DDM_H__

#include <limits.h>  /* CHAR_BIT */

// De-deuplication (Optimization) Map - DDM for Internal Use Suggested size of map (covered most general use case)
typedef	unsigned long ddm_t;
#define STRUTILS_NUMLIST_DDM_SIZE	1024
#define	BITS_PER_DDM_T	(sizeof(ddm_t) * CHAR_BIT)

enum DDM_STATE {
	DDM_OOB = -1,
	DDM_UNSET = 0,
	DDM_SET = 1
};
struct DDM {
	ddm_t map[STRUTILS_NUMLIST_DDM_SIZE];
};
static inline void __DDM_init(struct DDM *ddm) {
	memset(ddm->map, 0, sizeof(ddm->map));
}
static inline enum DDM_STATE __DDM_check_ex(struct DDM *ddm, int num, int should_set)
{
	int res = DDM_OOB;
	int shift = num / STRUTILS_NUMLIST_DDM_SIZE;

	if (shift < BITS_PER_DDM_T) {
		ddm_t *slot = ddm->map +
			(num % STRUTILS_NUMLIST_DDM_SIZE);

		res = *slot & (1 << shift) ? DDM_SET : DDM_UNSET;
		if (should_set) {
			*slot |= (1 << shift);
		}
	}
	return res;
}
static inline enum DDM_STATE __DDM_register(struct DDM *ddm, int num) {
	return __DDM_check_ex(ddm, num, 1);
}
static inline enum DDM_STATE __DDM_test(struct DDM *ddm, int num) {
	return __DDM_check_ex(ddm, num, 0);
}


#endif	/* __DDM_H__ */
