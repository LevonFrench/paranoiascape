#include "game_extras.h"
#include "debug_server.h"
#include "opengl_renderer.h"
#include <string.h>

extern PS1::OpenGLRenderer* g_renderer;

const char *game_get_name(void) { return "Paranoiascape"; }
uint32_t    game_get_display_entry(void) { return 0; }
const char *game_get_exe_filename(void) { return "SLPS_013.75"; }
uint32_t    game_get_expected_crc32(void) { return 0; }
void        game_on_init(void) {}
void        game_on_frame(uint32_t frame) {}
int         game_handle_arg(const char *key, const char *val) { return 0; }
const char *game_arg_usage(void) { return ""; }
void        game_fill_frame_record(void *record) {}

int         game_handle_debug_cmd(const char *cmd, int id, const char *json) {
    if (strcmp(cmd, "screenshot") == 0) {
        const char *p = strstr(json, "\"path\"");
        if (!p) {
            debug_server_send_fmt("{\"id\":%d,\"ok\":false,\"error\":\"missing path\"}", id);
            return 1;
        }
        p += 6;
        while (*p == ' ' || *p == ':') p++;
        if (*p == '"') {
            p++;
            char path[256];
            int i = 0;
            while (*p && *p != '"' && i < 255) {
                path[i++] = *p++;
            }
            path[i] = '\0';
            
            if (g_renderer) {
                g_renderer->FlushPrimitives();
                g_renderer->SaveScreenshotBMP(path);
                debug_server_send_fmt("{\"id\":%d,\"ok\":true,\"path\":\"%s\"}", id, path);
            } else {
                debug_server_send_fmt("{\"id\":%d,\"ok\":false,\"error\":\"renderer not initialized\"}", id);
            }
        } else {
            debug_server_send_fmt("{\"id\":%d,\"ok\":false,\"error\":\"invalid path format\"}", id);
        }
        return 1;
    }
    return 0;
}

void        game_post_frame(uint32_t frame_count) {}
uint32_t    game_get_entry_point(void) { return 0; }
