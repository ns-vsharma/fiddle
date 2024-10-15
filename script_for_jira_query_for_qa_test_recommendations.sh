#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 date<yyyy-mm-dd>"
    exit 1
fi

date=$1
input_file=temp_file_1
output_file=temp_file_2

#git log --since="2024-09-05" | grep -oE 'ENG-[0-9]+' > "$input_file"
git log --since=$date | grep -oE 'ENG-[0-9]+' > "$input_file"

# Clear the output file if it exists
> "$output_file"

echo ""

#prepare for JIRA Query
echo -n "Key in (" >> "$output_file"  

# Read words from the input file and append a comma to each, then write to the output file in a single line
{
    while read -r word; do
        echo -n "${word},"  # Use -n to avoid new line
    done < "$input_file"
} >> "$output_file"

# Optionally remove the last comma
sed -i '.bak' '$ s/, *$//' "$output_file"

#prepare for JIRA Query
echo -n ")" >> "$output_file"  

echo -n "AND \"Fix QA Test Recommendations[Paragraph]\" IS EMPTY AND type NOT IN (Escalation)" >> "$output_file"


#echo "Words with commas appended have been written to $output_file"

cat $output_file

echo ""
echo ""

#Delete the temp files
rm -f $input_file $output_file $output_file.bak
