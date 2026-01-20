/* programs/kedit.c */
#include "stdlib.h"

#define MAX_BUFFER 4096 

char* file_buffer;
int buffer_size = 0;
int cursor_pos = 0;
char* filename;

void draw_ui() {
    clear_screen();
    print("--- KEDIT: %s (Ctrl+S/tilde to Save, Ctrl+Q/backtick to Quit) ---\n\n", filename);
    for(int i=0; i <= buffer_size; i++) {
        if (i == cursor_pos) print("|");
        if (i < buffer_size) {
            char c[2] = {file_buffer[i], 0};
            print(c);
        }
    }
    print("\n\n------------------------------\n");
}

void save_file() {
    print("\nSaving...");
    
    // FIX: Do NOT unlink before ensuring we can open/write.
    // In this OS, open() returns -1 if file doesn't exist (no O_CREAT yet).
    // The safest way is to try opening. 
    int fd = open(filename);
    
    if (fd == -1) {
        // If file doesn't exist, we can't create it from here yet without 'touch' syscall.
        print(" Error: File not found! Use 'touch' in shell first.");
        return;
    }
    
    // We successfully opened it. Now we write.
    // Note: sys_write_file implementation currently overwrites at the current offset.
    // To ensure clean overwrite, we assume the FS handles the size update.
    seek(fd, 0, 0);
    write(fd, file_buffer, buffer_size);
    close(fd);
    print(" Done!");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print("Usage: kedit <filename>\n");
        return 1;
    }
    filename = argv[1];

    file_buffer = (char*)malloc(MAX_BUFFER);
    if (!file_buffer) return 1;

    int fd = open(filename);
    if (fd != -1) {
        buffer_size = read(fd, file_buffer, MAX_BUFFER);
        close(fd);
    } else {
        // New file
        buffer_size = 0;
    }

    while(1) {
        draw_ui();
        char c = get_char();
        if (c == '`') break; // Quit
        if (c == '~') { save_file(); get_char(); continue; } // Save

        if (c == '\b') {
            if (cursor_pos > 0) {
                for(int i=cursor_pos; i<buffer_size; i++) file_buffer[i-1] = file_buffer[i];
                buffer_size--;
                cursor_pos--;
            }
        }
        else if (buffer_size < MAX_BUFFER - 1 && c >= 32 && c <= 126) {
            for(int i=buffer_size; i>cursor_pos; i--) file_buffer[i] = file_buffer[i-1];
            file_buffer[cursor_pos] = c;
            buffer_size++;
            cursor_pos++;
        }
        else if (c == '\n') {
            if (buffer_size < MAX_BUFFER - 1) {
                for(int i=buffer_size; i>cursor_pos; i--) file_buffer[i] = file_buffer[i-1];
                file_buffer[cursor_pos] = '\n';
                buffer_size++;
                cursor_pos++;
            }
        }
    }
    
    free(file_buffer);
    clear_screen();
    return 0;
}