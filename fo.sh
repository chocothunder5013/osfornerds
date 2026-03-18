#!/bin/bash
set -e

echo "Starting Safe ASM/LD Code Formatting..."

# 1. Format Linker Scripts (.ld)
find . -type f -name "*.ld" | while read -r file; do
    awk '
    BEGIN { indent = 0 }
    {
        # Trim leading and trailing whitespace
        sub(/^[ \t]+/, "");
        sub(/[ \t]+$/, "");
        
        if (length($0) == 0) {
            print "";
            next;
        }

        # Decrease indent if line starts with closing brace
        if ($0 ~ /^}/) {
            indent -= 4;
            if (indent < 0) indent = 0;
        }

        # Print with current indent (using spaces)
        printf "%*s%s\n", indent, "", $0;

        # Increase indent if line ends with opening brace
        if ($0 ~ /{$/) {
            indent += 4;
        }
    }' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done

# 2. Format Assembly Files (.S)
find src programs -type f -name "*.S" | while read -r file; do
    awk '
    {
        # Preserve empty lines
        if ($0 ~ /^[ \t]*$/) {
            print "";
            next;
        }
        
        # Trim leading and trailing whitespace
        sub(/^[ \t]+/, "");
        sub(/[ \t]+$/, "");

        # If it is a label (ends with :), a section, or an extern/global declaration -> NO INDENT
        if ($0 ~ /:$/ || $0 ~ /^(section|global|extern|MBALIGN|MEMINFO|VIDEO|FLAGS|MAGIC|CHECKSUM)/) {
            print $0;
        } else {
            # Standard instruction or directive -> 4 SPACES INDENT
            print "    " $0;
        }
    }' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done

echo "Formatting complete! Run 'git diff' to ensure your logic is untouched."
