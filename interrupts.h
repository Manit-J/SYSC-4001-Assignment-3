#ifndef INTERRUPTS_H
#define INTERRUPTS_H

typedef struct {
    int pNum; 
    int size;  
    int pid; 
} Partition;


typedef struct process{
    int pid; 
    int memorySize;
    int arrivalTime;
    int totalCPUTime;
    int iof;
    int iod;
    int partitionNum;
    int runTime;
    int iocount;
    int iorunTime;
    struct process* next;
} process;


bool allTerminated();

bool assignMemory(process* this);

process* removeProcess(process* head, process* p);

process* addAtEnd(process* head, int pid, int memorySize, int arrivalTime, int totalCPUTime, int iof, int iod, int partitionNum, int runTime, int iocount, int iorunTime);

process* createP(int pid, int memorySize, int arrivalTime, int totalCPUTime, int iof, int iod, int partitionNum, int runTime, int iocount, int iorunTime);

char* memoryAsString();

int getPartitionSize(int partitionNum);

#endif
