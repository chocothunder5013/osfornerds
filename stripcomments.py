import os
import re

C_STYLE_EXTS = {'.c', '.h', '.ld', '.S'}
HASH_STYLE_EXTS = {'.py', '.sh', '.cfg', '.conf'}

def strip_c_style(text):
    """
    Strips // and /* */ comments from C-style code while preserving string literals.
    """
    # This regex matches C-style comments, as well as single and double-quoted string literals.
    # Matching string literals prevents the regex from incorrectly stripping comments embedded within them.
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\'\'])*\'|"(?:\\.|[^\\"\"])*"',
        re.DOTALL | re.MULTILINE
    )
    def replacer(match):
        s = match.group(0)
        # If the match starts with a slash, it's a comment, so replace it with an empty string.
        # Otherwise, it's a string literal, so keep it as is.
        if s.startswith('/') or s.startswith('/*'):
            return ""
        else:
            return s
    return re.sub(pattern, replacer, text)

def strip_hash_style(text, is_script=False):
    """
    Strips # comments from script files while preserving string literals and shebangs.
    """
    lines = text.split('\n')
    out = []
    
    for i, line in enumerate(lines):
        # Keep the shebang line if this is an executable script.
        if is_script and i == 0 and line.startswith('#!'):
            out.append(line)
            continue
            
        in_string = False
        string_char = ''
        stripped_line = ''
        
        # Iterate character by character to handle inline comments accurately.
        for char in line:
            if char in '"\'':
                if not in_string:
                    in_string = True
                    string_char = char
                elif string_char == char:
                    in_string = False
            elif char == '#' and not in_string:
                break
            stripped_line += char
            
        out.append(stripped_line.rstrip())
        
    return '\n'.join(out)

def process_directory(root_dir):
    """
    Recursively walks through the directory tree and strips comments from recognized source files.
    """
    for dirpath, _, filenames in os.walk(root_dir):
        # Skip specific vendor and build directories.
        if any(skip in dirpath for skip in ['/limine', '/.git', '/iso_root']):
            continue
            
        for file in filenames:
            # Special case for Makefiles which use hash-style comments but lack a file extension.
            if file == 'Makefile':
                ext = '.mk'
            else:
                _, ext = os.path.splitext(file)
                
            filepath = os.path.join(dirpath, file)
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                if ext in C_STYLE_EXTS:
                    new_content = strip_c_style(content)
                elif ext in HASH_STYLE_EXTS or ext == '.mk':
                    new_content = strip_hash_style(content, is_script=(ext in {'.py', '.sh'}))
                else:
                    continue
                    
                # Clean up any leftover blank lines caused by removing full-line comments.
                new_content = os.linesep.join([s for s in new_content.splitlines() if s.strip()])
                
                with open(filepath, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                print(f"Stripped: {filepath}")
                
            except Exception as e:
                print(f"Failed to process {filepath}: {e}")

if __name__ == "__main__":
    process_directory(".")
    print("Done! All comments removed.")
