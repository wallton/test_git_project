#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "writer2_common.h"

extern tPcapShareMemory   *_PcapShareTable;

#define PcapSHMLock() 
#define PcapSHMUnLock() 

int
SnPcapFreeNodeGet (int * u4Index)
{
    PcapSHMLock();

    if((*_PcapShareTable).free_queue.first == -1){
        PcapSHMUnLock();
        return (FAILURE);
    }
    *u4Index = (*_PcapShareTable).free_queue.first ;

    PcapSHMUnLock();

    return (SUCCESS);
}
int
SnPcapFreeNodeAdd (int * u4Index)
{
    PcapSHMLock();

    if ((*_PcapShareTable).free_queue.first == -1)
    {
        (*_PcapShareTable).free_queue.first = *u4Index;
    }
    if ((*_PcapShareTable).free_queue.end != -1)
    {
        (*_PcapShareTable).entryTable[(*_PcapShareTable).free_queue.end].next = *u4Index;
    }
    (*_PcapShareTable).entryTable[*u4Index].next = -1;
    (*_PcapShareTable).free_queue.end = *u4Index;
    (*_PcapShareTable).free_queue.count += 1;

    PcapSHMUnLock();
    return (SUCCESS);
}
int
SnPcapFreeNodeRemove (int * u4Index)
{
    int current_index = -1;

    PcapSHMLock();

    if((*_PcapShareTable).free_queue.end == -1)
    {
        PcapSHMUnLock();
        return (FAILURE);
    }

    current_index = (*_PcapShareTable).free_queue.first;

    if (current_index != *u4Index)
    {
        printf("(%s %d) node %d is not first node\n",__func__,__LINE__, *u4Index);
        PcapSHMUnLock();
        return (SUCCESS);
    }

    if ((*_PcapShareTable).entryTable[current_index].next != -1)
    {
        (*_PcapShareTable).free_queue.first = (*_PcapShareTable).entryTable[current_index].next;
    }
    else
    {
        (*_PcapShareTable).free_queue.first = -1;
        (*_PcapShareTable).entryTable[current_index].next = -1;
    }

    if ((*_PcapShareTable).free_queue.end == *u4Index)
    {
        (*_PcapShareTable).free_queue.end = -1;
    }

    (*_PcapShareTable).free_queue.count -= 1;

    PcapSHMUnLock();
    return (SUCCESS);
}


int
SnPcapUsedNodeGet (int * u4Index)
{
    PcapSHMLock();

    if((*_PcapShareTable).allocated_queue.first == -1)
    {
        PcapSHMUnLock();
        return (FAILURE);
    }
    *u4Index = (*_PcapShareTable).allocated_queue.first ;

    PcapSHMUnLock();

    return (SUCCESS);
}
int
SnPcapUsedNodeAdd (int * u4Index)
{
    PcapSHMLock();

    if ((*_PcapShareTable).allocated_queue.first == -1){
        (*_PcapShareTable).allocated_queue.first = *u4Index;
    }
    if ((*_PcapShareTable).allocated_queue.end != -1){
        (*_PcapShareTable).entryTable[(*_PcapShareTable).allocated_queue.end].next = *u4Index;
    }
    (*_PcapShareTable).entryTable[*u4Index].next = -1;
    (*_PcapShareTable).allocated_queue.end = *u4Index;
    (*_PcapShareTable).allocated_queue.count += 1;

    PcapSHMUnLock();

    return (SUCCESS);
}
int
SnPcapUsedNodeRemove (int * u4Index)
{
    int current_index = -1;

    PcapSHMLock();

    if((*_PcapShareTable).allocated_queue.end == -1)
    {
        PcapSHMUnLock();
        return (FAILURE);
    }
    current_index = (*_PcapShareTable).allocated_queue.first;
    if (current_index != *u4Index)
    {
        printf("(%s %d) node %d is not first node\n",__func__,__LINE__, *u4Index);
        PcapSHMUnLock();
        return (SUCCESS);
    }
    if ((*_PcapShareTable).entryTable[current_index].next != -1)
    {
        (*_PcapShareTable).allocated_queue.first = (*_PcapShareTable).entryTable[current_index].next;
    }
    else
    {
        (*_PcapShareTable).allocated_queue.first = -1;
        (*_PcapShareTable).entryTable[current_index].next = -1;
    }
    if ((*_PcapShareTable).allocated_queue.end == *u4Index){
        (*_PcapShareTable).allocated_queue.end = -1;
    }
    (*_PcapShareTable).allocated_queue.count -= 1;

    PcapSHMUnLock();

    return (SUCCESS);
}

int
SnPcapSenderNodeGet (int * u4Index)
{
    PcapSHMLock();

    if((*_PcapShareTable).send_queue.first == -1)
    {
        PcapSHMUnLock();
        return (FAILURE);
    }
    *u4Index = (*_PcapShareTable).send_queue.first ;

    PcapSHMUnLock();

    return (SUCCESS);
}
int
SnPcapSenderNodeAdd (int * u4Index)
{
    PcapSHMLock();

    if ((*_PcapShareTable).send_queue.first == -1)
    {
        (*_PcapShareTable).send_queue.first = *u4Index;
    }
    if ((*_PcapShareTable).send_queue.end != -1)
    {
        (*_PcapShareTable).entryTable[(*_PcapShareTable).send_queue.end].next = *u4Index;
    }
    (*_PcapShareTable).entryTable[*u4Index].next = -1;
    (*_PcapShareTable).send_queue.end = *u4Index;
    (*_PcapShareTable).send_queue.count += 1;

    PcapSHMUnLock();

    return (SUCCESS);
}
int
SnPcapSenderNodeRemove (int * u4Index)
{
    int current_index = -1;

    PcapSHMLock();

    if((*_PcapShareTable).send_queue.end == -1)
    {
        PcapSHMUnLock();
        return (FAILURE);
    }
    current_index = (*_PcapShareTable).send_queue.first;
    if (current_index != *u4Index)
    {
        printf("(%s %d) node %d is not first node\n",__func__,__LINE__, *u4Index);
        PcapSHMUnLock();
        return (SUCCESS);
    }
    if ((*_PcapShareTable).entryTable[current_index].next != -1)
    {
        (*_PcapShareTable).send_queue.first = (*_PcapShareTable).entryTable[current_index].next;
    }
    else
    {
        (*_PcapShareTable).send_queue.first = -1;
        (*_PcapShareTable).entryTable[current_index].next = -1;
    }
    if ((*_PcapShareTable).send_queue.end == *u4Index)
    {
        (*_PcapShareTable).send_queue.end = -1;
    }
    (*_PcapShareTable).send_queue.count -= 1;

    PcapSHMUnLock();

    return (SUCCESS);
}




