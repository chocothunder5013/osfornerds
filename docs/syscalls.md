# System Call ABI

Userland applications in OSForNerds interact with hardware and kernel services through software interrupts, specifically `INT 0x80`. The custom standard library (`programs/stdlib.c`) wraps these interrupts into standard C functions.

## Register Convention

The system passes parameters via CPU registers to reduce stack overhead.

* `EAX`: System Call Number
* `EBX`: Argument 1
* `ECX`: Argument 2
* `EDX`: Argument 3
* `ESI`: Argument 4
* `EDI`: Argument 5

*Returns:* The kernel places the return value back into `EAX`.

## Syscall Table

| EAX | Name | EBX (Arg 1) | ECX (Arg 2) | EDX (Arg 3) | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0` | `SYS_PRINT` | `char *msg` | - | - | Prints a string to the attached terminal/window. |
| `1` | `SYS_YIELD` | - | - | - | Manually yields the CPU time slice to the scheduler. |
| `2` | `SYS_READ` | - | - | - | Blocks process until a character is available in the keyboard buffer. |
| `3` | `SYS_EXIT` | `int code` | - | - | Terminates the current process and flags it as a ZOMBIE. |
| `4` | `SYS_WAIT` | `int pid` | `int *status` | - | Blocks until the specified child PID exits. |
| `5` | `SYS_OPEN` | `char *path`| - | - | Opens a file and returns a File Descriptor (FD). |
| `6` | `SYS_CLOSE` | `int fd` | - | - | Closes an open File Descriptor. |
| `7` | `SYS_FREAD` | `int fd` | `char *buf` | `int size` | Reads `size` bytes from an open FD into a buffer. |
| `8` | `SYS_READDIR` | `int index` | `char *buf` | - | Reads the name of a file in the directory at `index`. |
| `9` | `SYS_SBRK` | `int incr` | - | - | Expands the program's heap space. Triggers VMM allocations. |
| `11`| `SYS_WRITE` | `int fd` | `char *buf` | `int size` | Writes `size` bytes to an FD and flushes to the ATA disk. |
| `12`| `SYS_SEEK` | `int fd` | `int offset`| `int whence`| Moves the read/write pointer of an open FD. |
| `13` | `SYS_CLEAR` | - | - | - | Clears the main console/terminal window. |
| `14` | `SYS_DELETE` | `char *path` | - | - | Deletes a file from the VFS. |
| `15` | `SYS_CHDIR` | `char *path` | - | - | Changes the current working directory. |
| `16` | `SYS_GETCWD` | `char *buf` | `int size` | - | Gets the current working directory string. |
| `17`| `SYS_SYSINFO` | `sysinfo_t *`| - | - | Populates a struct with uptime, memory, and process statistics. |
| `18`| `SYS_DRAW_RECT`| `rect_t *r` | - | - | Renders a solid rectangle to a specific window buffer. |
| `19` | `SYS_SET_CUR` | `int x` | `int y` | - | Sets the X/Y cursor coordinates of the main console. |
| `20`| `SYS_CREATE_WIN`| `char *title`| `int x` | `int y` | Spawns a new composited GUI window. Returns Window ID. |
| `21` | `SYS_WIN_PRT` | `int win_id` | `char *msg` | - | Prints a string to a specific composited window. |
| `22` | `SYS_WIN_CUR` | `int win_id` | `int x` | `int y` | Sets the cursor coordinates within a specific window. |
| `23` | `SYS_WIN_COL` | `int win_id` | `uint32_t c` | - | Sets the text color (`0xAARRGGBB`) for a specific window. |
| `24`| `SYS_KILL` | `int pid` | - | - | Sends a SIGKILL to forcefully terminate a process. |

> **Security Note:** `is_valid_user_ptr()` enforces Ring 3 memory safety. If a user application passes a pointer to Kernel space (e.g., `0xD0000000`), the syscall rejects it to stop privilege escalation.


