#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <commdlg.h>
#  pragma comment(lib, "comdlg32.lib")
#endif

#include "game_extras.h"
#include "crc32.h"

extern "C" void psxrecomp_runner_run(int argc, char **argv);

static void get_disc_cfg_path(char *out, int max_len) {
#ifdef _WIN32
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    char *last_sep = strrchr(exe_path, '\\');
    if (last_sep) *(last_sep + 1) = '\0';
    snprintf(out, max_len, "%sdisc.cfg", exe_path);
#else
    snprintf(out, max_len, "disc.cfg");
#endif
}

static void disc_cfg_read(char *path_out, int max_len) {
    char cfg_path[512];
    get_disc_cfg_path(cfg_path, sizeof(cfg_path));
    FILE *f = fopen(cfg_path, "r");
    if (!f) { path_out[0] = '\0'; return; }
    if (!fgets(path_out, max_len, f)) path_out[0] = '\0';
    fclose(f);
    int len = (int)strlen(path_out);
    while (len > 0 && (path_out[len-1] == '\n' || path_out[len-1] == '\r'))
        path_out[--len] = '\0';
}

static void disc_cfg_write(const char *cue_path) {
    char cfg_path[512];
    get_disc_cfg_path(cfg_path, sizeof(cfg_path));
    FILE *f = fopen(cfg_path, "w");
    if (!f) return;
    fprintf(f, "%s\n", cue_path);
    fclose(f);
}

static int pick_disc_file(char *out, int max_len) {
#ifdef _WIN32
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    out[0] = '\0';
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = NULL;
    ofn.lpstrFilter = "PS1 Disc Images (*.cue)\0*.cue\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = out;
    ofn.nMaxFile    = (DWORD)max_len;
    ofn.lpstrTitle  = "Select PS1 Disc Image";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    return GetOpenFileNameA(&ofn) ? 1 : 0;
#else
    return 0;
#endif
}

static int verify_disc(const char *path, uint32_t expected_crc) {
    if (expected_crc == 0) return 1;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    uint8_t *data = (uint8_t *)malloc((size_t)sz);
    if (!data) { fclose(f); return 0; }
    fread(data, 1, (size_t)sz, f);
    fclose(f);
    uint32_t actual = crc32_compute(data, (size_t)sz);
    free(data);
    return (actual == expected_crc);
}

static void derive_exe_path(const char *cue_path, char *exe_out, int max_len) {
    const char *last_sep = NULL;
    for (const char *p = cue_path; *p; p++) {
        if (*p == '/' || *p == '\\') last_sep = p;
    }
    if (last_sep) {
        int dir_len = (int)(last_sep - cue_path + 1);
        snprintf(exe_out, max_len, "%.*s%s", dir_len, cue_path, game_get_exe_filename());
    } else {
        snprintf(exe_out, max_len, "%s", game_get_exe_filename());
    }
}

static int count_positional(int argc, char **argv) {
    int count = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') count++;
        else if (!strcmp(argv[i], "--script") || !strcmp(argv[i], "--record") || !strcmp(argv[i], "--load-snapshot")) i++;
        else if (!strcmp(argv[i], "--save-snapshot")) i += 2;
    }
    return count;
}

int main(int argc, char *argv[]) {
    int positional_count = count_positional(argc, argv);

    if (positional_count >= 2) {
        psxrecomp_runner_run(argc, argv);
        return 0;
    }

    char cue_path[512] = {0};
    char exe_path[512] = {0};
    uint32_t expected_crc = game_get_expected_crc32();

    if (positional_count == 1) {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] != '-') {
                strncpy(cue_path, argv[i], sizeof(cue_path) - 1);
                break;
            }
        }
    } else {
        disc_cfg_read(cue_path, sizeof(cue_path));
        if (cue_path[0] == '\0') {
            if (!pick_disc_file(cue_path, sizeof(cue_path))) return 1;
        }
        disc_cfg_write(cue_path);
    }

    derive_exe_path(cue_path, exe_path, sizeof(exe_path));
    
    char *new_argv[64];
    int new_argc = 0;
    new_argv[new_argc++] = argv[0];
    new_argv[new_argc++] = exe_path;
    new_argv[new_argc++] = cue_path;
    for (int i = 1; i < argc && new_argc < 63; i++) {
        if (argv[i][0] == '-') {
            new_argv[new_argc++] = argv[i];
            if ((!strcmp(argv[i], "--script") || !strcmp(argv[i], "--record") || !strcmp(argv[i], "--load-snapshot")) && i + 1 < argc) {
                new_argv[new_argc++] = argv[++i];
            } else if (!strcmp(argv[i], "--save-snapshot") && i + 2 < argc) {
                new_argv[new_argc++] = argv[++i];
                new_argv[new_argc++] = argv[++i];
            }
        }
    }
    new_argv[new_argc] = NULL;

    psxrecomp_runner_run(new_argc, new_argv);
    return 0;
}
