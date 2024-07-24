#!/bin/bash

LOG_DIR="/home/oskar/.trashsys/log"

if [ -z "$1" ]; then
  echo "Usage: $0 <replacement_string>"
  exit 1
fi

REPLACEMENT="$1"

if [ ! -d "$LOG_DIR" ]; then
  echo "Directory $LOG_DIR does not exist."
  exit 1
fi

for file in "$LOG_DIR"/*; do
  if [ -f "$file" ]; then
    echo "replacing trashed time in: $file"
    awk -v replacement="$REPLACEMENT" 'NR % 5 == 0 {print replacement} NR % 5 != 0 {print $0}' "$file" > "$file.tmp" && mv "$file.tmp" "$file"
  fi
done

echo "Done."
