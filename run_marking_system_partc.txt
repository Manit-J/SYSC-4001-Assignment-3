#!/bin/bash

# Exit script on any error
set -e

# File and executable names
SOURCE_FILE="marking_system.c"
EXECUTABLE="marking_system"
STUDENT_DATABASE="student_database.txt"

# Compilation
echo "Compiling $SOURCE_FILE for Part C..."
gcc -o $EXECUTABLE $SOURCE_FILE -lpthread

# Check if the student database file exists
if [ ! -f "$STUDENT_DATABASE" ]; then
  echo "Error: $STUDENT_DATABASE not found!"
  exit 1
fi

# Run the program
echo "Running the marking system program for Part C..."
./$EXECUTABLE

# Clean up shared memory and semaphores (in case of any leftover resources)
echo "Cleaning up shared memory and semaphores..."
ipcs -m | grep '0x' | awk '{print $2}' | xargs -r -I {} ipcrm -m {}
ipcs -s | grep '0x' | awk '{print $2}' | xargs -r -I {} ipcrm -s {}

echo "Program execution for Part C completed."

