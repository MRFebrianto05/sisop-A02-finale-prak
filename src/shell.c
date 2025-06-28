#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
    char buf[64];
    char cmd[64];
    char arg[2][64];

    byte cwd = FS_NODE_P_ROOT;

    while (true) {
        printString("MengOS:");
        printCWD(cwd);
        printString("$ ");
        readString(buf);
        parseCommand(buf, cmd, arg);

        if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
        else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
        else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
        else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
        else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
        else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
        else if (strcmp(cmd, "clear")) clearScreen();
        else printString("Invalid command\n");
    }
}

// FUNGSI-FUNGSI PEMBANTU
void printPathRecursive(struct node_fs *node_fs_buf, byte cwd_idx);

// TODO: 4. Implement printCWD function [DONE]
void printCWD(byte cwd) {
    struct node_fs node_fs_buf;

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    if (cwd == FS_NODE_P_ROOT) {
        printString("/");

    } else {
        printPathRecursive(&node_fs_buf, cwd);
    }
}

void printPathRecursive(struct node_fs *node_fs_buf, byte cwd_idx) {
    byte parent_idx;

    if (cwd_idx == FS_NODE_P_ROOT) {
        printString("/");
        return;
    }

    parent_idx = node_fs_buf->nodes[cwd_idx].parent_index;

    printPathRecursive(node_fs_buf, parent_idx);

    printString(node_fs_buf->nodes[cwd_idx].node_name);
    printString("/");
}

// TODO: 5. Implement parseCommand function [DONE]
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0, j = 0;
    int arg_idx = 0;

    clear(cmd, 64);
    clear(arg[0], 64);
    clear(arg[1], 64);

    while (buf[i] == ' ') i++;

    while (buf[i] != ' ' && buf[i] != '\0') cmd[j++] = buf[i++];
    cmd[j] = '\0';

    while (buf[i] == ' ') i++;

    while (buf[i] == '\0' && arg_idx < 2) {
        j = 0;

        while (buf[i] != ' ' && buf[i] != '\0') arg[arg_idx][j++] = buf[i++];
        arg[arg_idx][j] = '\0';

        while (buf[i] == ' ') i++;

        arg_idx++;
    }
}

// TODO: 6. Implement cd function [DONE]
void cd(byte* cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    if (strcmp(dirname, "/") == 0) {
        *cwd = FS_NODE_P_ROOT;
        return;
    }

    if (strcmp(dirname, "..") == 0) {
        if (*cwd == FS_NODE_P_ROOT) return;

        *cwd = node_fs_buf.nodes[*cwd].parent_index;
        return;
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == *cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
               *cwd = i;
               return;
            } else {
               printString("Not a directory");
               return;
            }
        }
    }

    printString("No such file or directory");
}

// TODO: 7. Implement ls function [DONE]
void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    byte target_dir;

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    if (strcmp(dirname, "") == 0 || strcmp(dirname, ".") == 0) {
        target_dir = cwd;
    } else if (dirname != 0 && strlen(dirname) > 0) {
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd &&
                strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0) {
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    target_dir = i;
                    break;
                } else {
                    printString("Not a directory");
                    return;
                }
            }
        }

        if (i == FS_MAX_NODE) {
            printString("No such file or directory");
            return;
        }
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index = target_dir && node_fs_buf.nodes[i].node_name != '\0') {
            printString(node_fs_buf.nodes[i].node_name);
            printString("\t");
        }
    }

    printString("\n");
}

// TODO: 8. Implement mv function [DONE]
void mv(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    int i;
    int src_node_idx = -1;
    int dst_parent_idx = -1;
    int slash_pos = -1;
    char basename[MAX_FILENAME];
    char dirname[MAX_FILENAME];

    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, src) == 0) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                printString("Cannot move directory");
                return;
            } else {
                src_node_idx = i;
                break;
            }
        }
    }

    if (i == FS_MAX_NODE) {
        printString("No such file or directory");
        return;
    }

    if (dst[0] == '/') {
        dst_parent_idx = FS_NODE_P_ROOT;
        strcpy(basename, dst + 1);
    } else if (dst[0] == '.' && dst[1] == '.' && dst[2] == '/') {
        if (cwd == FS_NODE_P_ROOT) {
            dst_parent_idx = FS_NODE_P_ROOT;
        } else {
            dst_parent_idx = node_fs_buf.nodes[cwd].parent_index;
        }
        strcpy(basename, dst + 3);
    } else {
        for (i = 0; dst[i] != '\0'; i++) {
            if (dst[i] == '/') {
                slash_pos = i;
                break;
            }
        }

        if (slash_pos == -1) {
            printString("invalid destination format");
            return;
        }

        for (i = 0; i < slash_pos; i++) {
            dirname[i] = dst[i];
        }
        dirname[slash_pos] = '\0';
        strcpy(basename, dst + slash_pos + 1);

        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd &&
                strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0) {

                if (node_fs_buf.nodes[i].data_index != FS_NODE_D_DIR) {
                    printString("Not a directory");
                    return;
                }
                dst_parent_idx = i;
                break;
            }
        }

        if (dst_parent_idx == -1) {
            printString("No such file or directory");
            return;
        }
    }

    for (i = 0; i < FS_MAX_NODE; i++) {
        if (i != src_node_idx && node_fs_buf.nodes[i].parent_index == dst_parent_idx &&
            strcmp(node_fs_buf.nodes[i].node_name, basename) == 0) {
            printString("file already exist");
            return;
        }
    }

    node_fs_buf.nodes[src_node_idx].parent_index = dst_parent_idx;
    strcpy(node_fs_buf.nodes[src_node_idx].node_name, basename);

    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    struct file_metadata src_metadata;
    struct file_metadata dst_metadata;
    enum fs_return status;

    src_metadata.parent_index = cwd;
    strcpy(src_metadata.node_name, src);

    fsRead(&src_metadata, &status);
    if (status != FS_SUCCESS) {
        if (status == FS_R_TYPE_IS_DIRECTORY) printString("Not a file\n");
        else if (status == FS_R_NODE_NOT_FOUND) printString("No such file\n");
        else printString("Error reading source\n");

        return;
    }

    dst_metadata.parent_index = cwd;
    strcpy(dst_metadata.node_name, dst);

    memcpy(dst_metadata.buffer, src_metadata.buffer, src_metadata.filesize);
    dst_metadata.filesize = src_metadata.filesize;

    fsWrite(&dst_metadata, &status);
    if (status != FS_SUCCESS) {
        if (status == FS_W_NODE_ALREADY_EXISTS) printString("File already exists\n");
        else if (status == FS_W_NO_FREE_DATA) printString("Not available free data blocks\n");
        else if (status == FS_W_NO_FREE_NODE) printString("Not available free node\n");
        else if (status == FS_W_NOT_ENOUGH_SPACE) printString("Not available free space\n");
        else printString("Error writing destination");

        return;
    }
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {}
