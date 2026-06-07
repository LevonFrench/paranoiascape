#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <GLFW/glfw3.h>
#include "psx_runtime.h"
#include "opengl_renderer.h"
#include "gpu_interpreter.h"
#include "debug_server.h"
#include "automation.h"
#include "game_extras.h"
#include "xa_audio.h"
#include "cdrom_stub.h"
#include <exception>
#include <iostream>

static void psx_terminate_handler() {
    std::cerr << "!!! FATAL: std::terminate called !!!" << std::endl;
    // We don't have a clean way to get a stack trace here without external libs,
    // but at least we can log that it happened.
    abort();
}

/* Bridge globals */
extern "C" {
    uint32_t g_ps1_frame = 0;
    int g_turbo = 0;
    int g_snap_inject_requested = 0;
    int g_debug_mode = 0;
    uint8_t g_ram[0x200000];
    uint8_t g_scratch[0x400];
}

static const char* s_script_path = NULL;
static const char* s_record_path = NULL;
static uint32_t s_snap_save_frame = 0;
static const char* s_snap_save_path = NULL;
static const char* s_snap_load_path = NULL;

static PS1::OpenGLRenderer* g_renderer = NULL;
static PS1::GPUInterpreter* g_gpu = NULL;

extern "C" void renderer_set_fmv_renderer(void* r);

/* Bridge functions for runtime.c and debug_server.c */
extern "C" {
    void gpu_submit_word(uint32_t word) { if (g_gpu) g_gpu->WriteGP0(word); }
    uint32_t gpu_read_word() { return g_gpu ? g_gpu->ReadGPUREAD() : 0; }
    void gpu_write_gp1(uint32_t word) { if (g_gpu) g_gpu->WriteGP1(word); }
    void gpu_abort_streaming() { if (g_gpu) g_gpu->AbortStreaming(); }
    static uint16_t keyboard_get_pad() {
        int input_override = debug_server_get_input_override();
        if (input_override != -1) return (uint16_t)input_override;

        GLFWwindow* win = (GLFWwindow*)g_renderer->GetWindow();
        if (!win) return 0;
        uint16_t buttons = 0;
        if (glfwGetKey(win, GLFW_KEY_UP))    buttons |= 0x0010;
        if (glfwGetKey(win, GLFW_KEY_DOWN))  buttons |= 0x0040;
        if (glfwGetKey(win, GLFW_KEY_LEFT))  buttons |= 0x0080;
        if (glfwGetKey(win, GLFW_KEY_RIGHT)) buttons |= 0x0020;
        if (glfwGetKey(win, GLFW_KEY_Z))     buttons |= 0x4000; // Cross
        if (glfwGetKey(win, GLFW_KEY_X))     buttons |= 0x2000; // Circle
        if (glfwGetKey(win, GLFW_KEY_C))     buttons |= 0x8000; // Square
        if (glfwGetKey(win, GLFW_KEY_V))     buttons |= 0x1000; // Triangle
        if (glfwGetKey(win, GLFW_KEY_ENTER)) buttons |= 0x0008; // Start
        if (glfwGetKey(win, GLFW_KEY_SPACE)) buttons |= 0x0001; // Select
        if (glfwGetKey(win, GLFW_KEY_A))     buttons |= 0x0400; // L1
        if (glfwGetKey(win, GLFW_KEY_S))     buttons |= 0x0100; // L2
        if (glfwGetKey(win, GLFW_KEY_D))     buttons |= 0x0800; // R1
        if (glfwGetKey(win, GLFW_KEY_F))     buttons |= 0x0200; // R2
        return buttons;
    }

    void psx_present_frame() { 
        if (g_renderer) {
            // Software 60 FPS throttle
            if (!g_turbo) {
                static LARGE_INTEGER freq = {0};
                static LARGE_INTEGER last = {0};
                if (freq.QuadPart == 0) {
                    QueryPerformanceFrequency(&freq);
                    QueryPerformanceCounter(&last);
                }
                while (1) {
                    LARGE_INTEGER now;
                    QueryPerformanceCounter(&now);
                    double elapsed = (double)(now.QuadPart - last.QuadPart) / freq.QuadPart;
                    if (elapsed >= (1.0 / 60.0)) {
                        last = now;
                        break;
                    }
                    Sleep(1);
                }
            }

            static uint32_t s_last_log = 0;
            static LARGE_INTEGER s_last_log_time = {0};
            if (g_ps1_frame - s_last_log >= 60) {
                LARGE_INTEGER now, freq;
                QueryPerformanceFrequency(&freq);
                QueryPerformanceCounter(&now);
                double elapsed_sec = 0;
                if (s_last_log_time.QuadPart != 0) {
                    elapsed_sec = (double)(now.QuadPart - s_last_log_time.QuadPart) / freq.QuadPart;
                }
                double fps = elapsed_sec > 0 ? (g_ps1_frame - s_last_log) / elapsed_sec : 60.0;
                s_last_log_time = now;

                PS1::DrawingArea da = g_renderer->GetDrawingArea();
                printf("[FRAME] %u  FPS=%.2f  g_turbo=%d  display=(%d,%d) %dx%d  draw_offset=(%d,%d)  draw_area=(%d,%d)-(%d,%d)\n",
                       g_ps1_frame, fps, g_turbo,
                       g_renderer->GetDisplayAreaX(), g_renderer->GetDisplayAreaY(),
                       g_renderer->GetDisplayWidth(), g_renderer->GetDisplayHeight(),
                       g_renderer->GetDrawOffsetX(), g_renderer->GetDrawOffsetY(),
                       da.x1, da.y1, da.x2, da.y2);
                fflush(stdout);
                s_last_log = g_ps1_frame;
            }
            g_renderer->Present(); 
            g_ps1_frame++;
            extern void psx_set_pad1(uint16_t buttons);
            psx_set_pad1(keyboard_get_pad());

            debug_server_record_frame();
            debug_server_poll();
            debug_server_wait_if_paused();

            /* --- Screenshot system --- */
            GLFWwindow* win = (GLFWwindow*)g_renderer->GetWindow();
            if (win) {
                /* F12: manual screenshot */
                static int s_f12_was_pressed = 0;
                int f12_now = glfwGetKey(win, GLFW_KEY_F12);
                if (f12_now && !s_f12_was_pressed) {
                    char path[256];
                    snprintf(path, sizeof(path), "sshots/shot_f%u.png", g_ps1_frame);
                    g_renderer->SaveScreenshotBMP(path);
                    printf("[SCREENSHOT] F12 → %s\n", path);
                    fflush(stdout);
                }
                s_f12_was_pressed = f12_now;

                /* F11: manual VRAM dump */
                static int s_f11_was_pressed = 0;
                int f11_now = glfwGetKey(win, GLFW_KEY_F11);
                if (f11_now && !s_f11_was_pressed) {
                    char path[256];
                    snprintf(path, sizeof(path), "sshots/vram_f%u.png", g_ps1_frame);
                    g_renderer->SaveVRAMDumpBMP(path);
                    printf("[VRAM-DUMP] F11 → %s\n", path);
                    fflush(stdout);
                }
                s_f11_was_pressed = f11_now;

                /* Auto-screenshot at milestones */
                static uint32_t s_auto_milestones[] = {10, 30, 50, 70, 100, 500, 855, 858, 860, 861, 862, 863, 864, 865, 866, 867, 868, 869, 870, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 3000, 5000, 10000, 13000, 13100, 13500, 14000, 15000, 18000, 20000, 25000};
                static int s_auto_idx = 0;
                int n_milestones = (int)(sizeof(s_auto_milestones) / sizeof(s_auto_milestones[0]));
                if (s_auto_idx < n_milestones && g_ps1_frame >= s_auto_milestones[s_auto_idx]) {
                    char path[256];
                    snprintf(path, sizeof(path), "sshots/auto_f%u.png", g_ps1_frame);
                    g_renderer->SaveScreenshotBMP(path);
                    /* Also dump VRAM */
                    {
                        snprintf(path, sizeof(path), "sshots/vram_auto_f%u.png", g_ps1_frame);
                        g_renderer->SaveVRAMDumpBMP(path);
                    }
                    s_auto_idx++;
                }
            }
        }
    }
    
    /* FMV Player bridge */
    void fmv_vram_upload(int x, int y, int w, int h, const uint16_t* data) {
        if (g_renderer) g_renderer->UploadToVRAM(x, y, w, h, data);
    }
    void fmv_vram_upload24(int x, int y, int w, int h, const uint8_t* data) {
        if (g_renderer) g_renderer->UploadToVRAM24(x, y, w, h, data);
    }
    void fmv_force_display_area(int x, int y, int w, int h) {
        if (g_renderer) {
            g_renderer->SetDisplayArea(x, y);
            g_renderer->SetDisplayMode(w, h, false, false);
        }
    }
    const uint16_t* psx_get_vram_pixels(void) {
        return g_renderer ? g_renderer->GetVRAMPixels() : nullptr;
    }

    /* Debug server bridge */
    void psx_debug_get_display_area(uint16_t* x, uint16_t* y) {
        if (g_renderer) { *x = (uint16_t)g_renderer->GetDisplayAreaX(); *y = (uint16_t)g_renderer->GetDisplayAreaY(); }
    }
    void psx_debug_get_draw_offset(int16_t* x, int16_t* y) {
        if (g_renderer) { *x = (int16_t)g_renderer->GetDrawOffsetX(); *y = (int16_t)g_renderer->GetDrawOffsetY(); }
    }
    void psx_debug_get_display_mode(uint8_t* h_res, uint8_t* v_res, uint8_t* enable) {
        /* simplified for now */
        *h_res = 1; *v_res = 0; *enable = 0;
    }
    uint32_t psx_debug_get_gpustat() {
        return g_gpu ? g_gpu->ReadGPUSTAT() : 0;
    }
    int psx_debug_read_vram(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *out, int max_pixels) {
        if (g_renderer) { g_renderer->DownloadFromVRAM(x, y, w, h, out); return w * h; }
        return 0;
    }
}

extern "C" void psxrecomp_runner_run(int argc, char** argv) {
    timeBeginPeriod(1);
    printf("[Runner] Entry\n");
    fflush(stdout);

    const char* exe_path = NULL;
    const char* cue_path = NULL;
    int mmio_trace = 0;
    int debug_port = 4370;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--script") && i + 1 < argc) {
            s_script_path = argv[++i];
        } else if (!strcmp(argv[i], "--record") && i + 1 < argc) {
            s_record_path = argv[++i];
        } else if (!strcmp(argv[i], "--mmio-trace")) {
            mmio_trace = 1;
        } else if (!strcmp(argv[i], "--debug-port") && i + 1 < argc) {
            debug_port = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--debug")) {
            g_debug_mode = 1;
        } else if (!exe_path) {
            exe_path = argv[i];
        } else if (!cue_path) {
            cue_path = argv[i];
        }
    }

    if (!exe_path || !cue_path) {
        printf("Usage: recomp <exe> <cue> [options]\n");
        fflush(stdout);
        exit(1);
    }

    if (!psx_cdrom_init(cue_path)) {
        printf("ERROR: Failed to open CD image: %s\n", cue_path);
        fflush(stdout);
        exit(1);
    }

    FILE* f = fopen(exe_path, "rb");
    if (!f) { printf("ERROR: Failed to open EXE: %s\n", exe_path); fflush(stdout); exit(1); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* exe_data = (uint8_t*)malloc(size);
    fread(exe_data, 1, size, f);
    fclose(f);

    uint32_t load_addr = 0x80010000u;
    uint32_t entry_point = 0x80010000u;
    const uint8_t* load_data = exe_data;
    uint32_t load_size = (uint32_t)size;
    if (size > 2048 && memcmp(exe_data, "PS-X EXE", 8) == 0) {
        memcpy(&entry_point, exe_data + 0x10, 4);
        memcpy(&load_addr, exe_data + 0x18, 4);
        load_data = exe_data + 2048;
        load_size = (uint32_t)(size - 2048);
        printf("[Runner] PS-X Header: entry=0x%08X load=0x%08X size=%u\n", entry_point, load_addr, load_size);
    } else {
        printf("[Runner] No PS-X header found or invalid size, using defaults\n");
    }
    fflush(stdout);

    static CPUState cpu;
    std::set_terminate(psx_terminate_handler);
    psx_runtime_init(&cpu);
    psx_install_crash_handler();
    if (mmio_trace) psx_mmio_trace_enable(1);
    psx_runtime_load(load_addr, load_data, load_size);
    free(exe_data);

    static PS1::OpenGLRenderer renderer;
    static PS1::GPUInterpreter gpu(&renderer);
    g_renderer = &renderer;
    g_gpu      = &gpu;

    // Register renderer for FMV compositing (fmv_player -> renderer_upload_fmv_frame)
    renderer_set_fmv_renderer(&renderer);

    if (!renderer.Initialize()) {
        printf("ERROR: Failed to initialize renderer\n");
        fflush(stdout);
        exit(1);
    }

    debug_server_init(debug_port);
    debug_server_set_glfw_window((GLFWwindow*)renderer.GetWindow());

    printf("Calling entry point at 0x%08X...\n", entry_point);
    fflush(stdout);
    call_by_address(&cpu, entry_point);

    GLFWwindow* win = (GLFWwindow*)renderer.GetWindow();
    while (win && !glfwWindowShouldClose(win)) {
        renderer.Present();
        renderer.VSync();
        g_ps1_frame++;
        if (g_ps1_frame % 60 == 0) {
            printf("[frame %d]\n", g_ps1_frame);
            fflush(stdout);
        }
    }

    debug_server_shutdown();
    renderer.Shutdown();
    
    /* Clean up asynchronous subsystems to join threads */
    extern void spu_shutdown(void);
    extern void cd_audio_shutdown(void);
    extern void fmv_player_shutdown(void);
    extern void xa_audio_shutdown(void);
    
    spu_shutdown();
    xa_audio_shutdown();
    cd_audio_shutdown();
    fmv_player_shutdown();
    timeEndPeriod(1);
}
