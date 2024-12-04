#!/bin/bash

# Set the script to exit on any error
set -e

# Compilation
echo "Compiling marking_system.c..."
gcc -o marking_system marking_system.c -lpthread

# Check if the student database file exists
if [ ! -f student_database.txt ]; then
  echo "Error: student_database.txt not found!"
  exit 1
fi

# Run the program
echo "Running the marking system program..."
./marking_system

# Clean up shared memory segments (in case of abnormal termination)
echo "Cleaning up shared memory and semaphores..."
ipcs -m | grep '0x' | awk '{print $2}' | xargs -I {} ipcrm -m {}
ipcs -s | grep '0x' | awk '{print $2}' | xargs -I {} ipcrm -s {}

echo "Program execution completed."

