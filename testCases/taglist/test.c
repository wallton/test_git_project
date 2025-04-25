#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  /* access , F_OK */
#include "taglist.h"

/*** --------------------------------------------------***/
#define ARRAY_SIZE(x)        (sizeof(x)/sizeof(x[0]))
#define WTP_CONFIG_FILE       "peplink_config_tx.txt"
/*** --------------------------------------------------***/

static int check_taglist(tagListType *t)
{
	int i = 0;
	const char * const list[] = {
		"SUBSR_LICENSE",
		"SUBSR_LICENSE_DUE",
		"SUBSR_LICENSE_DUE_EXPIRE",
		"SUBSR_LICENSE_DUE_TIMER",
		"SERVICE_CARE_DUE",
		"SERVICE_CARE_DUE_EXPIRE",
		"SERVICE_CARE_DUE_TIMER",
		"SERVICE_CARE_NAME",
		"SERVICE_CARE_TYPE",
		"SERVICE_SFWAN",
		"SERVICE_SFWAN_CP",
		"SERVICE_SFWAN_CP_SUSPEND",
		"SERVICE_SFWAN_SERVER",
		"SERVICE_SFWAN_SUSPEND",
		"ADMIN_PASSWORD",
		"ADMIN_NAME",
		"ADMIN_LANONLY"
	};
    char portlan[64] = {0};



	for (i = 0; i < ARRAY_SIZE(list); i++) {
		if (exist_taglist(t, list[i])) {
            printf("Tag [%s] = %s\n", list[i], get_taglist(t, list[i]));
		}
	}

	for (i = 1; i <= 8; i++) {
        snprintf(portlan, sizeof(portlan), "EXTSW_PORT%d_LAN_LIST", i);
		if (exist_taglist(t, portlan)) {
            printf("Tag [%s] = %s\n", portlan, get_taglist(t, portlan));
		}
    }

	return 1;
}

int main(int argc, char *argv[]) 
{

	tagListType *l = (access("peplink_config_tx.txt", F_OK) == 0) ? load_taglist("peplink_config_tx.txt") : NULL;

	if (l)
    {
		check_taglist(l);
    }

	return 0;
}
