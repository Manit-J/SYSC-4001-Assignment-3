#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <stdbool.h>
#include "interrupts.h"

int pTime = 0;

Partition memory[6] = {
    {1, 40, -1},
    {2, 25, -1},
    {3, 15, -1},
    {4, 10, -1},
    {5, 8, -1},
    {6, 2, -1}
};


process* PCB = NULL; // PCB Queue
process* new = NULL; // New Queue
process* ready = NULL; // Ready Queue
process* waiting = NULL; // Waiting Queue

process* running = NULL; // Process currently running

bool isEP = false; // true if scheduler uses EP algo, false otherwise
bool isRR = true; // true if scheduler uses RR algo, false otherwise

bool cpuBusy = false; // CPU Busy/Free state
int totalMemoryUsed = 0;
int totalFreeMemory = 100;
int usableMemory = 100;

int cpuOccupied = 0;



int main(int argc, char const *argv[])
{	

	FILE *file = fopen("input_data_101215842_101208175.txt", "r");
	FILE *output = fopen("execution_101215842_101272272.txt", "w");
	FILE *memoryOutput = fopen("memory_status_101215842_101272272.txt", "w");
    
    fprintf(output, "+------------------------------------------------+\n");
    fprintf(output, "Time of Transition |PID | Old State | New State |\n");
    fprintf(output, "+------------------------------------------------+\n");

    fprintf(memoryOutput, "+------------------------------------------------------------------------------------------+\n");
    fprintf(memoryOutput, "|Time of Event |Memory Used |      Partitions State |Total Free Memory |Usable Free Memory |\n");
    fprintf(memoryOutput, "+------------------------------------------------------------------------------------------+\n");
    fprintf(memoryOutput, "|            0 |          0 |-1, -1, -1, -1, -1, -1 |              100 |               100 |\n");


	char line[256];
	if (file != NULL)
    {
        while (fgets(line, sizeof(line), file)){
            char *pch;
            pch = strtok(line, " ,");
            char thisLine[6][100];

            int i = 0;
            while (pch != NULL)
            {
                strcpy(thisLine[i], pch);
                i += 1;
                pch = strtok(NULL, " ,");
            }

            // load the PCB will all the processes
            if (PCB == NULL){
				PCB = malloc(sizeof(process));
				PCB->pid = atoi(thisLine[0]); 
				PCB->memorySize = atoi(thisLine[1]); 
				PCB->arrivalTime = atoi(thisLine[2]); 
				PCB->totalCPUTime = atoi(thisLine[3]); 
				PCB->iof = atoi(thisLine[4]); 
				PCB->iod = atoi(thisLine[5]); 
				PCB->partitionNum = 7; 
				PCB->runTime = 0; 
				PCB->iocount = 0; 
				PCB->iorunTime = 0;
				PCB->next = NULL; 
            }
            else{
            	process* this = PCB;
            	while (this->next != NULL){
            		this = this->next;
            	}
            	this->next = malloc(sizeof(process));
				this->next->pid = atoi(thisLine[0]); 
				this->next->memorySize = atoi(thisLine[1]); 
				this->next->arrivalTime = atoi(thisLine[2]); 
				this->next->totalCPUTime = atoi(thisLine[3]); 
				this->next->iof = atoi(thisLine[4]); 
				this->next->iod = atoi(thisLine[5]); 
				this->next->partitionNum = 7; 
				this->next->runTime = 0; 
				this->next->iocount = 0;
				this->next->iorunTime = 0; 
				this->next->next = NULL; 
            }
        }

    }
    else
    {
        fprintf(stderr, "Unable to open file!\n");
    }
    fclose(file);

    while (!allTerminated()){
   // while (pTime < 200){
    	process* n = PCB;
    	while (n != NULL){
    		// Add process to new queue if arrival time = system time
    		if (n->arrivalTime == pTime){
	    		new = addAtEnd(new, n->pid, n->memorySize, n->arrivalTime, n->totalCPUTime, n->iof, n->iod, n->partitionNum, n->runTime, n->iocount, n->iorunTime);
        	}
            n = n->next;
    	}

    	process* r = new;
    	while (r != NULL){
    		// Add process to ready queue if memory available
    		// the function assignMemory() checks for memory space and returns true if memory is assigned to a process
    		if (assignMemory(r)){
    			totalMemoryUsed += r->memorySize;
    			totalFreeMemory -= r->memorySize;
    			usableMemory -= getPartitionSize(r->partitionNum);
	            fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, r->pid, "NEW", "READY");
	            fprintf(memoryOutput, "|%13d |%11d |%22s |%17d |%18d |\n", pTime, totalMemoryUsed, memoryAsString(), totalFreeMemory, usableMemory);
	            ready = addAtEnd(ready, r->pid, r->memorySize, r->arrivalTime, r->totalCPUTime, r->iof, r->iod, r->partitionNum, r->runTime, r->iocount, r->iorunTime);
	            //remove process from new queue
	            new = removeProcess(new, r);
    		}
    		r = r->next;
    	}


    	process* p = ready;
    	if (isEP){
    		// find largest processing time 
    		int largestPTime = 0;
    		while (p != NULL){
    			if (p->totalCPUTime > largestPTime){
    				largestPTime = p->totalCPUTime;
    			}
    			p = p->next;
    		}

    		process* q = ready;
    		while (q != NULL){
    			if (q->totalCPUTime == largestPTime && !cpuBusy && (q->runTime < q->totalCPUTime)){
    				running = addAtEnd(running, q->pid, q->memorySize, q->arrivalTime, q->totalCPUTime, q->iof, q->iod, q->partitionNum, q->runTime, q->iocount, q->iorunTime);
	    			fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, running->pid, "READY", "RUNNING");
	    			// remove process from ready queue
	    			ready = removeProcess(ready, q);

	    			// Cpu is now busy
	    			cpuBusy = true;
	    			break;
    			}
    			q = q->next;
    		}

    	}
    	else{

	    	while (p != NULL){
	    		// move process from ready to running
	    		if (!cpuBusy && (p->runTime < p->totalCPUTime)){
	    			if (isRR){
	    				cpuOccupied = 0;
	    			}
	    			running = addAtEnd(running, p->pid, p->memorySize, p->arrivalTime, p->totalCPUTime, p->iof, p->iod, p->partitionNum, p->runTime, p->iocount, p->iorunTime);
	    			fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, running->pid, "READY", "RUNNING");
	    			// remove process from ready queue
	    			ready = removeProcess(ready, p);

	    			// Cpu is now busy
	    			cpuBusy = true;
	    			break;
	    		}
	    		p = p->next;
	    	}
   		}


    	if (running != NULL){

    		if (isRR){
    			cpuOccupied += 1;
    			if (running->iocount == running->iof && running->runTime < running->totalCPUTime){
		    		running->iocount = 0;
		    		running->iorunTime = 0;
		    		cpuBusy = false;
		    		// join waiting queue
		    		fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, running->pid, "RUNNING", "WAITING");
		    		waiting = addAtEnd(waiting, running->pid, running->memorySize, running->arrivalTime, running->totalCPUTime, running->iof, running->iod, running->partitionNum, running->runTime, running->iocount, running->iorunTime);
			        running = NULL;

		    	}

		    	else if (running->runTime == running->totalCPUTime){
		    		cpuBusy = false;
		    		// find process in PCB and put run time to totalCpuTime
		    		
		    		fprintf(output, "| %16d | %2d | %9s |%9s |\n", pTime, running->pid, "RUNNING", "TERMINATED");
		    		process* p = PCB;
		    		while (p != NULL){
		    			if (p->pid == running->pid){
		    				p->runTime = p->totalCPUTime + 1;
		    			}
		    			p = p->next;
		    		}

		    		// free memory
		    		for (int i = 0; i < 6; i++){
		    			if (memory[i].pid == running->pid){
		    				memory[i].pid = -1;
		    				break;
		    			}
		    		}
		    		totalMemoryUsed -= running->memorySize;
	    			totalFreeMemory += running->memorySize;
	    			usableMemory += getPartitionSize(running->partitionNum);

		    		fprintf(memoryOutput, "|%13d |%11d |%22s |%17d |%18d |\n", pTime, totalMemoryUsed, memoryAsString(), totalFreeMemory, usableMemory);

		    		running = NULL;
		    	}
		    	else if (cpuOccupied % 100 == 0 && running->runTime < running->totalCPUTime){
		    		ready = addAtEnd(ready, running->pid, running->memorySize, running->arrivalTime, running->totalCPUTime, running->iof, running->iod, running->partitionNum, running->runTime, running->iocount, running->iorunTime);
		    		cpuBusy = false;
		    		fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, running->pid, "RUNNING", "READY");
		    		running = NULL;
		    	}
		    	else if (running->runTime < running->totalCPUTime){
		    		running->runTime += 1;
		    		running->iocount += 1;
		    	}
		    	else{
		    		;
		    	}
    		}
    		else{
    			if (running->iocount == running->iof && running->runTime < running->totalCPUTime){
	    		running->iocount = 0;
	    		running->iorunTime = 0;
	    		cpuBusy = false;
	    		// join waiting queue
	    		fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, running->pid, "RUNNING", "WAITING");
	    		waiting = addAtEnd(waiting, running->pid, running->memorySize, running->arrivalTime, running->totalCPUTime, running->iof, running->iod, running->partitionNum, running->runTime, running->iocount, running->iorunTime);
		        running = NULL;

		    	}
		    	else if (running->runTime == running->totalCPUTime){
		    		cpuBusy = false;
		    		// find process in PCB and put run time to totalCpuTime
		    		
		    		fprintf(output, "| %16d | %2d | %9s |%9s |\n", pTime, running->pid, "RUNNING", "TERMINATED");
		    		process* p = PCB;
		    		while (p != NULL){
		    			if (p->pid == running->pid){
		    				p->runTime = p->totalCPUTime + 1;
		    			}
		    			p = p->next;
		    		}

		    		// free memory
		    		for (int i = 0; i < 6; i++){
		    			if (memory[i].pid == running->pid){
		    				memory[i].pid = -1;
		    				break;
		    			}
		    		}
		    		totalMemoryUsed -= running->memorySize;
	    			totalFreeMemory += running->memorySize;
	    			usableMemory += getPartitionSize(running->partitionNum);

		    		fprintf(memoryOutput, "|%13d |%11d |%22s |%17d |%18d |\n", pTime, totalMemoryUsed, memoryAsString(), totalFreeMemory, usableMemory);

		    		running = NULL;
		    	}
		    	else if (running->runTime < running->totalCPUTime){
		    		running->runTime += 1;
		    		running->iocount += 1;
		    	}
		    	else{
		    		;
		    	}
    		}
    	}



	    	

    	
    	process* w = waiting;
    	while (w != NULL){
    		if (w->iorunTime == w->iod){
    			if (ready == NULL && !cpuBusy){
    				running = addAtEnd(running, w->pid, w->memorySize, w->arrivalTime, w->totalCPUTime, w->iof, w->iod, w->partitionNum, w->runTime, w->iocount, w->iorunTime);
    				fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, w->pid, "WAITING", "READY");
    				fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, running->pid, "READY", "RUNNING");
    				pTime -= 1;
    				cpuBusy = true;
    			}
    			else{
    				ready = addAtEnd(ready, w->pid, w->memorySize, w->arrivalTime, w->totalCPUTime, w->iof, w->iod, w->partitionNum, w->runTime, w->iocount, w->iorunTime);
    				fprintf(output, "| %16d | %2d | %9s | %9s |\n", pTime, w->pid, "WAITING", "READY");
    			}
    			waiting = removeProcess(waiting, w); 

    			w = w->next;
    		}
    		else{
	    		w->iorunTime += 1;
	    		w = w->next;
    		}
    	}



    	// print all lists at every second
    	process* a = new;
	    printf("New At %d: ", pTime);
	    while (a != NULL) {
	        printf("%d -> ", a->pid);
	        a = a->next;
	    }
	    printf("NULL\n\n");

	    process* b = ready;
	    printf("Ready At %d: ", pTime);
	    while (b != NULL) {
	        printf("%d -> ", b->pid);
	        b = b->next;
	    }
	    printf("NULL\n\n");

	    process* c = waiting;
	    printf("Waiting At %d: ", pTime);
	    while (c != NULL) {
	        printf("%d -> ", c->pid);
	        c = c->next;
	    }
	    printf("NULL\n\n");

	    process* d = running;
	    printf("Running At %d: ", pTime);
	    while (d != NULL) {
	        printf("%d -> ", d->pid);
	        d = d->next;
	    }
	    printf("NULL\n\n\n");


    		pTime += 1;
}	

	fprintf(output, "+------------------------------------------------+");
    fclose(output);

    fprintf(memoryOutput,"+------------------------------------------------------------------------------------------+");
    fclose(memoryOutput);
	/* code */
	return 0;
}




bool assignMemory(process* this){
	int minDiff = 40;
	bool memoryAssigned = false;
	for (int i = 0 ; i < 6; i++){
		if (memory[i].size >= this->memorySize && memory[i].pid == -1) {
            int diff = memory[i].size - this->memorySize;
            if (diff < minDiff) {
                minDiff = diff;
                this->partitionNum = memory[i].pNum;
            }
            memoryAssigned = true;
        }
	}

	for (int i = 0 ; i < 6; i++){
		if (this->partitionNum == memory[i].pNum){
			memory[i].pid = this->pid;
		}
	}

	return memoryAssigned;
}



bool allTerminated(){
	if (PCB != NULL){
		process* current = PCB;
		while (current != NULL){
			if (current->runTime >= current->totalCPUTime){
				current = current->next;
			}
			else{
				return false; 
			}
		}
		return true;
	}
	else{
		return false;
	}
}



process* removeProcess(process* head, process* p) {
    if (head == NULL || p == NULL) {
        return head; // Return the head unchanged if invalid input
    }

    // If the node to be removed is the head
    if (head == p) {
        process* newHead = head->next;
        free(p);
        return newHead; // Return the new head
    }

    // Traverse the list to find the process before the one to remove
    process* current = head;
    while (current->next != NULL && current->next != p) {
        current = current->next;
    }

    // If the process was found, remove it
    if (current->next == p) {
        current->next = p->next;
        free(p);
    }

    return head; // Return the unchanged head
}

// Function to add a node at the end of the linked list
process* addAtEnd(process* head, int pid, int memorySize, int arrivalTime, int totalCPUTime, int iof, int iod, int partitionNum, int runTime, int iocount, int iorunTime) {
    process* newP = malloc(sizeof(process));
    newP->pid = pid; 
	newP->memorySize = memorySize; 
	newP->arrivalTime = arrivalTime; 
	newP->totalCPUTime = totalCPUTime; 
	newP->iof = iof; 
	newP->iod = iod; 
	newP->partitionNum = partitionNum;
	newP->runTime = runTime; 
	newP->iocount = iocount; 
	newP->iorunTime = iorunTime; 
    newP->next = NULL;

    if (head == NULL) {
        return newP; // If the list is empty, the new node is the head
    }

    process* current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = newP;
    return head; // Return the unchanged head
}

char* memoryAsString(){
	char* s = malloc(1000); 
    if (s == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    s[0] = '\0'; 

    char str[12];
    for (int i = 0; i < 6; i++) {
        sprintf(str, "%d", memory[i].pid); 
        strcat(s, str);

        if (i < 5) {
            strcat(s, ", ");
        }
    }

    return s; // Return the dynamically allocated string;
}

int getPartitionSize(int partitionNum){
	for (int i = 0; i < 6; i++){
		if (memory[i].pNum == partitionNum){
			return memory[i].size;
		}
	}
	return 0;
}
