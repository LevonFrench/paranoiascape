#include "cdrom_stub.h"
#include "iso_reader.h"
#include "xa_audio.h"
#include "cd_audio.h"
#include "fmv_player.h"
#include "spu.h"
#include <cstdio>

static PS1::ISOReader g_iso;

extern "C" {

int psx_cdrom_init(const char* cue_path) {
    if (!g_iso.Open(cue_path)) {
        fprintf(stderr, "[CDROM] Failed to open: %s\n", cue_path);
        return 0;
    }
    fprintf(stderr, "[CDROM] Opened disc: volume='%s'\n", g_iso.GetVolumeID().c_str());
    xa_audio_init(g_iso.GetBinPath().c_str());
    cd_audio_init(cue_path);
    fmv_player_init(g_iso.GetBinPath().c_str());
    spu_init();
    return 1;
}

int psx_cdrom_read_sector(uint32_t lba, uint8_t* buffer) {
    return g_iso.ReadSector(lba, buffer) ? 1 : 0;
}

int psx_cdrom_read_file(const char* filename, uint8_t* buffer) {
    std::string path(filename);
    size_t size = g_iso.GetFileSize(path);
    if (size == 0) return 0;
    return g_iso.ReadFile(path, buffer, size) == size ? 1 : 0;
}

int psx_cdrom_search_file(const char* filename, uint32_t* pos, uint32_t* size) {
    std::string path(filename);
    
    // ISOReader's FindFile takes the path and an ISOFileEntry
    PS1::ISOFileEntry entry;
    if (g_iso.FindFile(path, entry)) {
        *pos = entry.lba;
        *size = entry.size;
        return 1;
    }
    return 0;
}

int psx_cdrom_read_file_by_lba(uint32_t lba, uint32_t size, uint8_t* buffer) {
    if (size == 0) return 0;
    
    // Calculate number of sectors
    uint32_t num_sectors = (size + 2047) / 2048;
    
    // Read sectors
    for (uint32_t i = 0; i < num_sectors; i++) {
        if (!g_iso.ReadSector(lba + i, buffer + (i * 2048))) {
            printf("[CDROM] ReadSector failed at LBA %u (offset +%u)\n", lba + i, i);
            return 0;
        }
    }
    return 1;
}

void psx_cdrom_populate_file_table(uint8_t* g_ram, uint32_t table_addr) {
    uint32_t current_addr = table_addr;
    int populated_count = 0;
    while (true) {
        uint32_t name_ptr = 0;
        memcpy(&name_ptr, &g_ram[current_addr], 4);
        name_ptr &= 0x1FFFFF;
        
        if (name_ptr == 0 || name_ptr >= 2048 * 1024) break;
        
        char* filename = (char*)&g_ram[name_ptr];
        if (filename[0] == '\0') break;
        
        // Clean the filename
        char clean_name[64];
        int j = 0;
        for (int i = 0; filename[i] && j < 63; i++) {
            if (filename[i] == ';') break;
            if (filename[i] == '\\') clean_name[j++] = '/';
            else clean_name[j++] = filename[i];
        }
        clean_name[j] = 0;
        char* cn = clean_name;
        while (*cn == '/') cn++;
        
        uint32_t pos = 0, size = 0;
        if (psx_cdrom_search_file(cn, &pos, &size)) {
            memcpy(&g_ram[current_addr + 4], &pos, 4);
            memcpy(&g_ram[current_addr + 8], &size, 4);
            populated_count++;
        } else {
            printf("[CdInit] Warning: Could not find file '%s' for table entry\n", cn);
        }
        
        current_addr += 12;
    }
    printf("[CdInit] Populated %d entries in file table at 0x%08X\n", populated_count, table_addr);
}

} /* extern "C" */
