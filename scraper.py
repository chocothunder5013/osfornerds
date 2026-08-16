import os

OUTPUT_FILE = "os_source_dump.txt"

INCLUDE_FILENAMES = {
    "Makefile",
    "limine.cfg",
    "limine.conf"
}
INCLUDE_EXTENSIONS = {
    ".c", ".h", ".S", ".asm", ".ld"
}
IGNORE_DIRS = {
    ".git",
    "iso_root",
    "limine",
    "__pycache__"
}
IGNORE_FILES = {
    "scraper.py",
    OUTPUT_FILE,
    "disk.img",
    "my-os.iso"
}

def is_relevant(filename):
    """
    Determines whether a file matches inclusion rules based on filename or extension.
    """
    # Exclude explicitly ignored files like the script itself or its output.
    if filename in IGNORE_FILES:
        return False
        
    # Include files with exact name matches.
    if filename in INCLUDE_FILENAMES:
        return True
        
    # Include files with recognized source code extensions.
    _, ext = os.path.splitext(filename)
    if ext in INCLUDE_EXTENSIONS:
        return True
        
    return False

def scrape_project():
    """
    Traverses the project tree and concatenates relevant source files into a single text dump.
    This generates a comprehensive code context for external review or analysis.
    """
    print(f"Starting scrape... Outputting to {OUTPUT_FILE}")
    try:
        with open(OUTPUT_FILE, "w", encoding="utf-8") as outfile:
            for root, dirs, files in os.walk("."):
                # Prune the directory list in-place so os.walk skips ignored directories.
                dirs[:] = [d for d in dirs if d not in IGNORE_DIRS]
                
                for file in files:
                    if is_relevant(file):
                        filepath = os.path.join(root, file)
                        
                        # Add a clear separator block for readability in the final text dump.
                        separator = "=" * 60
                        header = f"\n{separator}\nFILE PATH: {filepath}\n{separator}\n"
                        
                        try:
                            # Use errors="replace" so a bad character won't break the entire scrape operation.
                            with open(filepath, "r", encoding="utf-8", errors="replace") as infile:
                                content = infile.read()
                                
                            outfile.write(header)
                            outfile.write(content)
                            outfile.write("\n")
                            
                            print(f"Scraped: {filepath}")
                        except Exception as e:
                            print(f"Error reading {filepath}: {e}")
                            
        print(f"\n--- Success! All relevant files dumped into {OUTPUT_FILE} ---")
    except IOError as e:
        print(f"Error opening output file: {e}")

if __name__ == "__main__":
    scrape_project()
