#!/bin/bash
set -e

echo "[*] Starting OSForNerds Codebase Formatter..."

# 1. Dependency Checks
if ! command -v clang-format &> /dev/null; then
    echo "[-] Error: clang-format not found. Install it (e.g., sudo apt install clang-format)"
    exit 1
fi
if ! command -v black &> /dev/null; then
    echo "[-] Warning: black (Python formatter) not found. Python files will be skipped."
    echo "    Install with: pip3 install black"
fi

# 2. Format C / C++ Headers (Kernel & Userland)
echo "[*] Formatting C/C++ files..."
find src programs -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +

# 3. Format Python Scripts (Build & Scraper tools)
if command -v black &> /dev/null; then
    echo "[*] Formatting Python scripts..."
    find . -maxdepth 1 -type f -name "*.py" -exec black -q {} +
fi

# 4. Format Linker Scripts (.ld)
echo "[*] Formatting Linker scripts..."
find . -type f -name "*.ld" | while read -r file; do
    awk '
    BEGIN { indent = 0 }
    {
        sub(/^[ \t]+/, ""); sub(/[ \t]+$/, "");
        if (length($0) == 0) { print ""; next; }
        if ($0 ~ /^}/) { indent -= 4; if (indent < 0) indent = 0; }
        printf "%*s%s\n", indent, "", $0;
        if ($0 ~ /{$/) { indent += 4; }
    }' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done

# 5. Format Assembly (.S)
echo "[*] Formatting Assembly files..."
find src programs -type f -name "*.S" | while read -r file; do
    awk '
    {
        if ($0 ~ /^[ \t]*$/) { print ""; next; }
        sub(/^[ \t]+/, ""); sub(/[ \t]+$/, "");
        if ($0 ~ /:$/ || $0 ~ /^(section|global|extern|MBALIGN|MEMINFO|VIDEO|FLAGS|MAGIC|CHECKSUM)/) {
            print $0;
        } else {
            print "    " $0;
        }
    }' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
done

echo "[+] Formatting complete! Run 'git diff' to review the changes."
