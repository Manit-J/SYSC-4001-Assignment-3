#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
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

// Function to read student IDs from the file
void read_student_ids(const char *filename, int *student_ids) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open student database file");
        exit(EXIT_FAILURE);
    }
    int index = 0;
    while (index < NUM_STUDENTS && fscanf(file, "%d", &student_ids[index]) == 1) {
        index++;
    }
    fclose(file);
}

// Function to shuffle the student IDs
void shuffle_student_ids(int *student_ids, int num_students) {
    for (int i = num_students - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        // Swap student_ids[i] and student_ids[j]
        int temp = student_ids[i];
        student_ids[i] = student_ids[j];
        student_ids[j] = temp;
    }
}

// Function to get TA's student range
void get_ta_student_range(int ta_id, int *start_index, int *end_index) {
    int students_per_ta = NUM_STUDENTS / NUM_TAS;
    *start_index = (ta_id - 1) * students_per_ta;
    *end_index = *start_index + students_per_ta;
    if (ta_id == NUM_TAS) {
        // Last TA gets any remaining students
        *end_index = NUM_STUDENTS;
    }
}

int main() {
    srand(time(NULL));

    // Initialize semaphores
    int semid = semget(IPC_PRIVATE, NUM_TAS, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Failed to create semaphores");
        exit(EXIT_FAILURE);
    }
    // Set all semaphores to 1
    for (int i = 0; i < NUM_TAS; i++) {
        semctl(semid, i, SETVAL, 1);
    }

    // Read student IDs from the file
    int student_ids[NUM_STUDENTS];
    read_student_ids("student_database.txt", student_ids);

    // Shuffle the student IDs
    shuffle_student_ids(student_ids, NUM_STUDENTS);

    // Create 5 TA processes
    for (int i = 1; i <= NUM_TAS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process (TA)
            char filename[10];
            snprintf(filename, sizeof(filename), "TA%d.txt", i);
            FILE *ta_file = fopen(filename, "w");
            if (!ta_file) {
                perror("Failed to open TA file");
                exit(EXIT_FAILURE);
            }

            // Determine the range of students for this TA
            int start_index, end_index;
            get_ta_student_range(i, &start_index, &end_index);

            int index = start_index;

            while (index < end_index) {
                // Access database
                printf("TA%d is trying to access the database.\n", i);
                sem_wait(semid, i - 1); // Lock own semaphore
                sem_wait(semid, i % NUM_TAS); // Lock next semaphore
                printf("TA%d has accessed the database.\n", i);

                // Pick next student
                int student_id = student_ids[index];
                index++;

                // Simulate delay between 1 and 4 seconds
                sleep(rand() % 4 + 1);

                // Release semaphores
                sem_signal(semid, i - 1);
                sem_signal(semid, i % NUM_TAS);
                printf("TA%d has released the database.\n", i);

                // Start marking
                printf("TA%d is marking student %d.\n", i, student_id);
                int mark = rand() % (MAX_MARK + 1);
                fprintf(ta_file, "Student %d: %d\n", student_id, mark);
                fflush(ta_file);
                // Simulate marking delay between 1 and 10 seconds
                sleep(rand() % 10 + 1);
                printf("TA%d has finished marking student %d.\n", i, student_id);
            }

            fclose(ta_file);
            printf("TA%d has finished marking all students.\n", i);
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

    // Remove semaphores
    semctl(semid, 0, IPC_RMID);
    printf("All TAs have finished marking.\n");
    return 0;
}

