#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define NUM_TAS 5
#define NUM_STUDENTS 80 // Total number of student IDs
#define MAX_MARK 10

// Semaphore operations
void sem_wait(int semid, int semnum) {
    struct sembuf sb = {semnum, -1, 0};
    semop(semid, &sb, 1);
}

void sem_signal(int semid, int semnum) {
    struct sembuf sb = {semnum, 1, 0};
    semop(semid, &sb, 1);
}

// Structure to hold student ID and mark
typedef struct {
    int student_id;
    int mark;
    int marked_by; // TA number who marked the student
} student_record;

// Function to read student IDs from the file into shared memory
void read_student_ids(const char *filename, student_record *students) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open student database file");
        exit(EXIT_FAILURE);
    }
    int index = 0;
    while (index < NUM_STUDENTS && fscanf(file, "%d", &students[index].student_id) == 1) {
        students[index].mark = -1; // Initialize mark to -1 (unmarked)
        students[index].marked_by = 0; // No TA has marked yet
        index++;
    }
    fclose(file);
}

// Function to shuffle the student IDs
void shuffle_student_ids(student_record *students, int num_students) {
    for (int i = num_students - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        // Swap students[i] and students[j]
        student_record temp = students[i];
        students[i] = students[j];
        students[j] = temp;
    }
}

int main() {
    srand(time(NULL));

    // Initialize semaphores
    int semid = semget(IPC_PRIVATE, NUM_TAS + 1, IPC_CREAT | 0666); // Additional semaphore for shared index
    if (semid == -1) {
        perror("Failed to create semaphores");
        exit(EXIT_FAILURE);
    }
    // Set all semaphores to 1
    for (int i = 0; i < NUM_TAS + 1; i++) {
        semctl(semid, i, SETVAL, 1);
    }

    // Create shared memory for student records
    int shm_id = shmget(IPC_PRIVATE, sizeof(student_record) * NUM_STUDENTS, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Failed to create shared memory segment for students");
        exit(EXIT_FAILURE);
    }
    student_record *students = shmat(shm_id, NULL, 0);
    if (students == (void *) -1) {
        perror("Failed to attach shared memory segment for students");
        exit(EXIT_FAILURE);
    }

    // Load student IDs into shared memory
    read_student_ids("student_database.txt", students);

    // Shuffle the student IDs
    shuffle_student_ids(students, NUM_STUDENTS);

    // Create shared memory for the shared index
    int shm_index_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shm_index_id == -1) {
        perror("Failed to create shared memory segment for index");
        exit(EXIT_FAILURE);
    }
    int *shared_index = shmat(shm_index_id, NULL, 0);
    if (shared_index == (void *) -1) {
        perror("Failed to attach shared memory segment for index");
        exit(EXIT_FAILURE);
    }
    *shared_index = 0; // Initialize shared index

    // Create 5 TA processes
    for (int i = 1; i <= NUM_TAS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process (TA)
            int ta_id = i;

            // Attach shared memory segments
            student_record *students = shmat(shm_id, NULL, 0);
            if (students == (void *) -1) {
                perror("TA: Failed to attach shared memory segment for students");
                exit(EXIT_FAILURE);
            }
            int *shared_index = shmat(shm_index_id, NULL, 0);
            if (shared_index == (void *) -1) {
                perror("TA: Failed to attach shared memory segment for index");
                exit(EXIT_FAILURE);
            }

            while (1) {
                // Access database
                printf("TA%d is trying to access the database.\n", ta_id);
                sem_wait(semid, ta_id - 1); // Lock own semaphore
                sem_wait(semid, ta_id % NUM_TAS); // Lock next semaphore
                sem_wait(semid, NUM_TAS); // Lock shared index semaphore
                printf("TA%d has accessed the database.\n", ta_id);

                // Check if all students have been marked
                if (*shared_index >= NUM_STUDENTS) {
                    // Release semaphores
                    sem_signal(semid, NUM_TAS);
                    sem_signal(semid, ta_id - 1);
                    sem_signal(semid, ta_id % NUM_TAS);
                    printf("TA%d found no more students to mark.\n", ta_id);
                    break;
                }

                // Pick next student
                int index = *shared_index;
                (*shared_index)++;

                // Release shared index semaphore
                sem_signal(semid, NUM_TAS);

                student_record *student = &students[index];

                // Simulate delay between 1 and 4 seconds
                sleep(rand() % 4 + 1);

                // Release semaphores
                sem_signal(semid, ta_id - 1);
                sem_signal(semid, ta_id % NUM_TAS);
                printf("TA%d has released the database.\n", ta_id);

                // Mark the student
                printf("TA%d is marking student %d.\n", ta_id, student->student_id);
                int mark = rand() % (MAX_MARK + 1);
                student->mark = mark;
                student->marked_by = ta_id;

                // Simulate marking delay between 1 and 10 seconds
                sleep(rand() % 10 + 1);
                printf("TA%d has finished marking student %d with mark %d.\n", ta_id, student->student_id, mark);
            }

            // Detach shared memory
            shmdt(students);
            shmdt(shared_index);
            printf("TA%d has finished marking all students.\n", ta_id);
            exit(0);
        } else if (pid < 0) {
            perror("Failed to fork process");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all TA processes to finish
    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // Print the results
    printf("\nFinal Marks:\n");
    for (int i = 0; i < NUM_STUDENTS; i++) {
        if (students[i].student_id != 9999) {
            printf("Student %d: Mark %d (Marked by TA%d)\n",
                   students[i].student_id, students[i].mark, students[i].marked_by);
        }
    }

    // Detach and remove shared memory
    shmdt(students);
    shmctl(shm_id, IPC_RMID, NULL);

    shmdt(shared_index);
    shmctl(shm_index_id, IPC_RMID, NULL);

    // Remove semaphores
    semctl(semid, 0, IPC_RMID);
    printf("All TAs have finished marking.\n");
    return 0;
}

