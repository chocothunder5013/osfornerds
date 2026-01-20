import os

# --- Configuration ---
OUTPUT_FILE = "os_source_dump.txt"

# Files to specificially include (exact names)
INCLUDE_FILENAMES = {
    "Makefile",
    "limine.cfg",
    "limine.conf"
}

# Extensions to include (Source code and linker scripts)
INCLUDE_EXTENSIONS = {
    ".c", ".h", ".S", ".asm", ".ld"
}

# Directories to strictly ignore
IGNORE_DIRS = {
    ".git",
    "iso_root",
    "limine",    # The limine binary folder
    "__pycache__"
}

# Specific files to ignore (The script itself and the output)
IGNORE_FILES = {
    "scraper.py",
    OUTPUT_FILE,
    "disk.img",
    "my-os.iso"
}

def is_relevant(filename):
    """Checks if a file should be scraped based on extension or name."""
    if filename in IGNORE_FILES:
        return False
    
    # Check exact filenames
    if filename in INCLUDE_FILENAMES:
        return True
        
    # Check extensions
    _, ext = os.path.splitext(filename)
    if ext in INCLUDE_EXTENSIONS:
        return True
        
    return False

def scrape_project():
    print(f"Starting scrape... Outputting to {OUTPUT_FILE}")
    
    try:
        with open(OUTPUT_FILE, "w", encoding="utf-8") as outfile:
            # os.walk traverses the tree top-down
            for root, dirs, files in os.walk("."):
                # Modify dirs in-place to skip ignored directories
                dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
                
                for file in files:
                    if is_relevant(file):
                        filepath = os.path.join(root, file)
                        
                        # Create a nice header for each file
                        separator = "=" * 60
                        header = f"\n{separator}\nFILE PATH: {filepath}\n{separator}\n"
                        
                        try:
                            with open(filepath, "r", encoding="utf-8", errors="replace") as infile:
                                content = infile.read()
                                
                            outfile.write(header)
                            outfile.write(content)
                            outfile.write("\n") # Ensure newline at end of file content
                            print(f"Scraped: {filepath}")
                            
                        except Exception as e:
                            print(f"Error reading {filepath}: {e}")

        print(f"\n--- Success! All relevant files dumped into {OUTPUT_FILE} ---")

    except IOError as e:
        print(f"Error opening output file: {e}")

if __name__ == "__main__":
    scrape_project()
