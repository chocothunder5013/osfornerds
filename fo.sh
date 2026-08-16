#!/bin/bash
set -e
echo "Starting Safe ASM/LD Code Formatting..."
find . -type f -name "*.ld" | while read -r file; do
    awk '
    BEGIN { indent = 0 }
    {
        sub(/^[ \t]+/, "");
        sub(/[ \t]+$/, "");
        if (length($0) == 0) {
            print "";
            next;
        }
        if ($0 ~ /^}/) {
            indent -= 4;
            if (indent < 0) indent = 0;
        }
        printf "%*s%s\n", indent, "", $0;
        if ($0 ~ /{$/) {
            indent += 4;
        }
    }' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done
find src programs -type f -name "*.S" | while read -r file; do
    awk '
    {
        if ($0 ~ /^[ \t]*$/) {
            print "";
            next;
        }
        sub(/^[ \t]+/, "");
        sub(/[ \t]+$/, "");
        if ($0 ~ /:$/ || $0 ~ /^(section|global|extern|MBALIGN|MEMINFO|VIDEO|FLAGS|MAGIC|CHECKSUM)/) {
            print $0;
        } else {
            print "    " $0;
        }
    }' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done
echo "Formatting complete! Run 'git diff' to ensure your logic is untouched."