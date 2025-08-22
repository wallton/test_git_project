/*----------------------------------------------------------------------*
 * Gatespace
 * Copyright 2005-2011 Gatespace. All Rights Reserved.
 * Gatespace Networks, Inc. confidential material.
 *----------------------------------------------------------------------*
 * File Name  : rpc.h
 *
 * Description: CWMP RPC definitions and data structures
 * $Revision: 1.15 $
 * $Id: rpc.h,v 1.15 2012/06/13 11:16:10 dmounday Exp $
 *----------------------------------------------------------------------*/
#ifndef _RPC_H_
#define _RPC_H_

//#include "time.h"
//#include "sys.h"



/* CPEState.eventMask - one or more may be present */
#define EVT_BOOTSTRAP	0x0000001	/* all zero bit is bootstrap */
#define EVT_BOOT		0X0000002	/*        */
#define EVT_VALUECHANGE	0x0000004
#define EVT_XFERCOMPL	0x0000008
#define EVT_ADDOBJECT	0x0000010
#define EVT_DELOBJECT	0x0000020
#define EVT_REBOOT		0x0000040
#define EVT_DOWNLOAD	0x0000080
#define EVT_UPLOAD		0x0000100
#define EVT_SETPARAMVAL 0x0000200
#define EVT_AUTOXFRCMPL	0x0000800

typedef enum {
	eNOTIFYOFF = 0,
	eNOTIFYPASSIVE =1,
	eNOTIFYACTIVE=2
}eNOTIFICATION;

/* The return value from the getter/setter functions are the values
 * CPE_OK thru CPE_REBOOT.
 * The Object Commit functions can return an additional indicator to
 * stop the post-order traversal of commit calls. This is a mask bit
 * that is or'ed with the CPE_xxx status value return by the
 * Commit function. It is masked off by the execCommit() function.
 */
#define COMMIT_STOP	0x04000000
#define STATUS_MASK	0x0000ffff
typedef enum {
	CPE_OK = 0,
	CPE_REBOOT,
	CPE_ERR,
    CPE_9000 = 9000,
    CPE_9001,
    CPE_9002,
    CPE_9003,
    CPE_9004,
    CPE_9005,
    CPE_9006,
    CPE_9007,
    CPE_9008,
    CPE_9009,
    CPE_9010,
    CPE_9011,
    CPE_9012,
    CPE_9013,
    CPE_9014,				/* 9014.. 9019 added in v1.1 schema*/
    CPE_9015,
    CPE_9016,
    CPE_9017,
    CPE_9018,
    CPE_9019,
    CPE_9020,
    CPE_9021,
    CPE_9022,
    CPE_9023,
    CPE_9024,
    CPE_9025,
    CPE_9026,
    CPE_9027,
    CPE_9028,
    CPE_9029,
    CPE_9030,
    CPE_9031,
    CPE_9032,
    CPE_9800 = 9800,
    CPE_9801 = 9801,
    CPE_9802 = 9802,
    CPE_9803 = 9803,
    CPE_9804 = 9804,
    CPE_9805 = 9805,
    CPE_9806 = 9806,
    CPE_9807 = 9807,
    CPE_9842 = 9842,
    CPE_VNDR_END = 9899
} CPE_STATUS;



#endif   /*** _RPC_H_******/







