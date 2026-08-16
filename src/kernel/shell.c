#include "../mm/heap.h"
extern void term_print(const char *str);
extern void term_clear();
extern int  strcmp(const char *s1, const char *s2);
extern int  strlen(const char *str);
extern void strcpy_safe(char *dest, const char *src);
extern int  elf_load_file(const char *name, char *args);
extern void fs_list(const char *path);
extern void fs_cat(const char *path);
extern void fs_touch(const char *path);
extern void fs_mkdir(const char *path);
extern void fs_delete(const char *path);
extern int  sys_chdir(const char *path);
extern void sys_getcwd(char *buf, int size);
extern int  process_wait(int pid, int *status_ptr);
int         str_starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*prefix++ != *str++)
            return 0;
    }
    return 1;
}
int atoi(const char *str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}
/*
 * Parses the user input string and executes the corresponding shell command.
 * It checks against a list of built-in commands (like cd, ls, or clear).
 * If the command is not built-in, it attempts to load and execute an ELF binary from the filesystem.
 * It also handles trailing ampersands ('&') to run processes in the background asynchronously.
 */
void execute_command(char *input) {
    if (input[0] == 0)
        return;
    if (strcmp(input, "ls") == 0 || str_starts_with(input, "ls ")) {
        char *path = 0;
        if (input[2] == ' ')
            path = input + 3;
        fs_list(path);
    } else if (strcmp(input, "pwd") == 0) {
        char buf[64];
        sys_getcwd(buf, 64);
        term_print(buf);
        term_print("\n");
    } else if (str_starts_with(input, "mkdir ")) {
        fs_mkdir(input + 6);
    } else if (str_starts_with(input, "cat ")) {
        fs_cat(input + 4);
    } else if (str_starts_with(input, "touch ")) {
        fs_touch(input + 6);
    } else if (str_starts_with(input, "kill ")) {
        extern int kill_process(int pid);
        int        target_pid = atoi(input + 5);
        if (kill_process(target_pid) == 0) {
            term_print("Process terminated.\n");
        } else {
            term_print("Failed to kill process (invalid PID or access denied).\n");
        }
    } else if (str_starts_with(input, "rm ")) {
        fs_delete(input + 3);
    } else if (strcmp(input, "clear") == 0) {
        term_clear();
    } else if (strcmp(input, "help") == 0) {
        term_print("\n--- MyOS Commands ---\n");
        term_print("  ls [path]       - List directory\n");
        term_print("  cd <path>       - Change directory\n");
        term_print("  pwd             - Print working directory\n");
        term_print("  cat <file>      - Print file content\n");
        term_print("  touch <file>    - Create file\n");
        term_print("  mkdir <name>    - Create directory\n");
        term_print("  rm <file>       - Delete file\n");
        term_print("  clear           - Clear screen\n");
        term_print("  <program>       - Run program (e.g. hello.elf)\n");
    } else if (str_starts_with(input, "cd ")) {
        if (sys_chdir(input + 3) != 0) {
            term_print("Directory not found.\n");
        }
    } else {
        char filename[32];
        char args[64];
        int  i = 0;
        while (input[i] && input[i] != ' ' && i < 31) {
            filename[i] = input[i];
            i++;
        }
        filename[i] = 0;
        if (input[i] == ' ')
            strcpy_safe(args, input + i + 1);
        else
            args[0] = 0;
        int is_bg   = 0;
        int arg_len = strlen(args);
        if (arg_len > 0 && args[arg_len - 1] == '&') {
            is_bg             = 1;
            args[arg_len - 1] = '\0';
            if (arg_len > 1 && args[arg_len - 2] == ' ') {
                args[arg_len - 2] = '\0';
            }
        }
        int pid = elf_load_file(filename, args);
        if (pid == -1) {
            term_print("Unknown command or file not found.\n");
        } else {
            if (!is_bg) {
                process_wait(pid, 0);
            } else {
                term_print(" -> Process running in background\n");
            }
        }
    }
}