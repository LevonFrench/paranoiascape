#include "game_extras.h"

const char *game_get_name(void) { return "Paranoiascape"; }
uint32_t    game_get_display_entry(void) { return 0; }
const char *game_get_exe_filename(void) { return "SLPS_013.75"; }
uint32_t    game_get_expected_crc32(void) { return 0; }
void        game_on_init(void) {}
void        game_on_frame(uint32_t frame) {}
int         game_handle_arg(const char *key, const char *val) { return 0; }
const char *game_arg_usage(void) { return ""; }
void        game_fill_frame_record(void *record) {}
int         game_handle_debug_cmd(const char *cmd, int id, const char *json) { return 0; }
void        game_post_frame(uint32_t frame_count) {}
uint32_t    game_get_entry_point(void) { return 0; }
