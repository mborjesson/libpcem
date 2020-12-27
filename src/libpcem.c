#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <dirent.h>
#include <time.h>
#include <errno.h>

#include <sys/timerfd.h>

#if defined WIN32 || defined _WIN32
#include <windows.h>
#endif

#include "thread.h"
#include "plat-keyboard.h"
#include "plat-mouse.h"
#include "plat-joystick.h"
#include "plat-midi.h"
#include "video.h"
#include "sound.h"
#include "paths.h"
#include "ibm.h"
#include "mem.h"
#include "model.h"
#include "nvr.h"
#include "device.h"
#include "cdrom-image.h"
#include "disc.h"
#include "ide.h"
#include "scsi_zip.h"
#include "cassette.h"
#include "codegen.h"

#include "pcem.h"

// Callbacks

pcem_on_event_t pcem_on_event_func = NULL;

pcem_video_size_t pcem_video_size_func = NULL;
pcem_video_blit_draw_t pcem_video_blit_draw_func = NULL;

pcem_input_keyboard_poll_t pcem_input_keyboard_poll_func = NULL;
pcem_input_mouse_poll_t pcem_input_mouse_poll_func = NULL;

pcem_audio_stream_create_t pcem_audio_stream_create_func = NULL;
pcem_audio_stream_data_t pcem_audio_stream_data_func = NULL;

pcem_config_get_t pcem_config_get_func = NULL;
pcem_config_set_t pcem_config_set_func = NULL;
pcem_config_save_t pcem_config_save_func = NULL;

void pcem_callback_on_event(pcem_on_event_t func)
{
        pcem_on_event_func = func;
}

void pcem_callback_video_size(pcem_video_size_t func)
{
        pcem_video_size_func = func;
}

void pcem_callback_video_blit_draw(pcem_video_blit_draw_t func)
{
        pcem_video_blit_draw_func = func;
}

void pcem_callback_input_keyboard_poll(pcem_input_keyboard_poll_t func)
{
        pcem_input_keyboard_poll_func = func;
}

void pcem_callback_input_mouse_poll(pcem_input_mouse_poll_t func)
{
        pcem_input_mouse_poll_func = func;
}


void pcem_callback_audio_stream_create(pcem_audio_stream_create_t func)
{
        pcem_audio_stream_create_func = func;
}

void pcem_callback_audio_stream_data(pcem_audio_stream_data_t func)
{
        pcem_audio_stream_data_func = func;
}

void pcem_callback_config_get(pcem_config_get_t func)
{
        pcem_config_get_func = func;
}

void pcem_callback_config_set(pcem_config_set_t func)
{
        pcem_config_set_func = func;
}

void pcem_callback_config_save(pcem_config_save_t func)
{
        pcem_config_save_func = func;
}

// Implementation

int screen_width = 0;
int screen_height = 0;

// Timers

uint64_t timer_freq = 1000000000;

uint64_t timer_read()
{
        uint64_t ticks;
        struct timespec now;

        clock_gettime(CLOCK_MONOTONIC, &now);
        ticks = now.tv_sec;
        ticks *= 1000000000;
        ticks += now.tv_nsec;
        return ticks;
}

// Paths

int dir_exists(char *path)
{
        DIR* dir = opendir(path);
        if (dir)
        {
                closedir(dir);
                return 1;
        }
        return 0;
}

// Keyboard

uint8_t pcem_key[272];
//uint8_t rawinputkey[272];

void keyboard_init()
{
        memset(pcem_key, 0, sizeof(pcem_key));
}

void keyboard_close()
{
}

void keyboard_poll_host()
{
        if (pcem_input_keyboard_poll_func) pcem_input_keyboard_poll_func((void *) pcem_key);
}

// Mouse

int mouse_buttons;
int mouse[4];
static int mouse_x = 0, mouse_y = 0, mouse_z = 0;

void mouse_init()
{
}

void mouse_close()
{
}

void mouse_wheel_update(int dir)
{
}

void mouse_poll_host()
{
        if (pcem_input_mouse_poll_func)
        {
                pcem_input_mouse_poll_func(&mouse[0], &mouse[1], &mouse[2], &mouse[3]);

                mouse_x += mouse[0];
                mouse_y += mouse[1];
                mouse_z += mouse[2];

                mouse_buttons = mouse[3];
        }
}

void mouse_get_mickeys(int *x, int *y, int *z)
{
        *x = mouse_x;
        *y = mouse_y;
        *z = mouse_z;
        mouse_x = mouse_y = mouse_z = 0;
}

// Joystick

joystick_t joystick_state[MAX_JOYSTICKS];

void joystick_init()
{
}

void joystick_close()
{
}

void joystick_poll()
{
}

// MIDI

void midi_init()
{
}

void midi_close()
{
}

void midi_write(uint8_t val)
{
}

int midi_get_num_devs()
{
        return 0;
}

void midi_get_dev_name(int num, char *s)
{
}

// Video

void *blit_mutex;

void startblit()
{
        thread_lock_mutex(blit_mutex);
        if (pcem_on_event_func) pcem_on_event_func(PCEM_EVENT_BLIT_BEGIN);
}

void endblit()
{
        if (pcem_on_event_func) pcem_on_event_func(PCEM_EVENT_BLIT_END);
        thread_unlock_mutex(blit_mutex);
}

BITMAP *create_bitmap(int x, int y)
{
        BITMAP *b = malloc(sizeof(BITMAP) + (y * sizeof(uint8_t *)));
        int c;
        b->dat = malloc(x * y * 4);
        for (c = 0; c < y; c++)
                b->line[c] = b->dat + (c * x * 4);
        b->w = x;
        b->h = y;
        return b;
}

void destroy_bitmap(BITMAP *b)
{
        free(b);
}

void hline(BITMAP *b, int x1, int y, int x2, int col)
{
        if (y < 0 || y >= buffer32->h)
                return;

        for (; x1 < x2; x1++)
                ((uint32_t *)b->line[y])[x1] = col;
}

void updatewindowsize(int x, int y)
{
        if (screen_width != x || screen_height != y)
        {
                screen_width = x;
                screen_height = y;
                if (pcem_video_size_func) pcem_video_size_func(x, y);
        }
        
}

void set_window_title(const char *s)
{
        
}

// Sound

int SOUNDBUFLEN = 48000 / 20;

void initalmain(int argc, char *argv[])
{
}

void inital()
{
        if (pcem_audio_stream_create_func)
        {
                pcem_audio_stream_create_func(PCEM_AUDIO_STREAM_DEFAULT, 48000, 16, 2, SOUNDBUFLEN * 2 * 2);
                pcem_audio_stream_create_func(PCEM_AUDIO_STREAM_CD, CD_FREQ, 16, 2, CD_BUFLEN * 2 * 2);
        }
        if (pcem_on_event_func) pcem_on_event_func(PCEM_EVENT_AUDIO_INIT);
}

void givealbuffer(int32_t *buf)
{
        int c;
        int16_t buf16[SOUNDBUFLEN*2];

        if (pcem_audio_stream_data_func)
        {
                for (c = 0; c < SOUNDBUFLEN * 2; c++)
                {
                        if (buf[c] < -32768)
                                buf16[c] = -32768;
                        else if (buf[c] > 32767)
                                buf16[c] = 32767;
                        else
                                buf16[c] = buf[c];
                }

                pcem_audio_stream_data_func(PCEM_AUDIO_STREAM_DEFAULT, (void *) buf16, SOUNDBUFLEN * 2 * 2);
        }
}

void givealbuffer_cd(int16_t *buf)
{
        if (pcem_audio_stream_data_func) pcem_audio_stream_data_func(PCEM_AUDIO_STREAM_CD, (void *) buf, CD_BUFLEN * 2 * 2);
}

// Emulation

void stop_emulation_now(void)
{
        /*Deduct a sufficiently large number of cycles that no instructions will
                run before the main thread is terminated*/
        cycles -= 99999999;

        pcem_stop();

        if (pcem_on_event_func) pcem_on_event_func(PCEM_EVENT_EMULATION_HALTED);
}

void warning(const char *format, ...)
{
}

// Main

int romspresent[ROM_MAX];
int gfx_present[GFX_MAX];
int emulation_state = PCEM_EMULATION_STATE_STOPPED;
void* main_thread = NULL;
void* onesec_thread = NULL;
int drawits = 0;
uint64_t main_time = 0;

void pcem_set_roms_paths(const char* path)
{
        set_roms_paths(strdup(path));
}

void pcem_set_nvr_path(const char* path)
{
        set_nvr_path(strdup(path));
}

void pcem_set_logs_path(const char* path)
{
        set_logs_path(strdup(path));
}

extern void video_blit_complete();

void blit_memtoscreen(int x, int y, int y1, int y2, int w, int h)
{
        if (y1 == y2)
        {
                video_blit_complete();
                return; /*Nothing to do*/
        }

        if (pcem_video_blit_draw_func) pcem_video_blit_draw_func(0, w, y1, y2, x, y, buffer32->dat, buffer32->w, buffer32->h, 4);

        video_blit_complete();
}

uint64_t start_ms = 0;

uint64_t get_ms()
{
        struct timespec spec;

        clock_gettime(CLOCK_MONOTONIC, &spec);

        uint64_t ms = (spec.tv_sec * 1000) + (spec.tv_nsec / 1.0e6);

        return ms;
}

uint32_t get_ticks()
{
        return (uint32_t) (get_ms() - start_ms);
}

int msleep(long msec)
{
        #if defined WIN32 || defined _WIN32
        Sleep(msec);
        #else
        usleep(msec * 1000);
        #endif

}

void onesecthread(void* params)
{
        int timerfd, res;
        struct itimerspec timspec;
        uint64_t exp;

        timerfd = timerfd_create(CLOCK_MONOTONIC, 0);

        if (timerfd < 0)
        {
                perror("timerfd_create");
                return;
        }

        bzero(&timspec, sizeof(timspec));
        timspec.it_interval.tv_sec = 1;
        timspec.it_interval.tv_nsec = 0;
        timspec.it_value.tv_sec = 1;
        timspec.it_value.tv_nsec = 0;

        res = timerfd_settime(timerfd, 0, &timspec, 0);
        if (res < 0)
        {
                perror("timerfd_settime");
                return;
        }

        while (emulation_state != PCEM_EMULATION_STATE_STOPPED)
        {
                res = read(timerfd, &exp, sizeof(uint64_t));
                if (res < 0)
                {
                        perror("timerfd_read");
                        continue;
                }
                onesec();
        }
}

void mainthread(void* params)
{
        int frames;
        uint32_t old_time, new_time, elapsed;

        frames = 0;

        drawits = 0;
        old_time = get_ticks();
        while (emulation_state != PCEM_EMULATION_STATE_STOPPED)
        {
                new_time = get_ticks();
                elapsed = new_time - old_time;
                drawits += elapsed;
                old_time = new_time;

                if (drawits > 0 && emulation_state == PCEM_EMULATION_STATE_RUNNING)
                {
                        uint64_t start_time = timer_read();
                        uint64_t end_time;
                        drawits -= 10;
                        if (drawits > 50)
                                drawits = 0;
                        runpc();
                        frames++;
                        if (frames >= 200 && nvr_dosave)
                        {
                                frames = 0;
                                nvr_dosave = 0;
                                savenvr();
                        }
                        end_time = timer_read();
                        main_time += end_time - start_time;
                }
                else
                        msleep(1);
        }
}

extern void pcem_config_init();
extern void pcem_config_close();

extern int cpuspeed2;
extern int atfullspeed;

int pcem_start()
{
        int c;

        start_ms = get_ms();

        pclog("PCem start...\n");

        pcem_config_init();

        video_blit_memtoscreen_func = blit_memtoscreen;

        joystick_init();

        //initpc(0, NULL);

        loadconfig(NULL);

        cpuspeed2=(AT)?2:1;
        atfullspeed=0;

        device_init();        
        
        initvideo();
        mem_init();
        loadbios();
        mem_add_bios();
                        
        codegen_init();

        resetpchard();

        sound_init();

        blit_mutex = thread_create_mutex();

        emulation_state = PCEM_EMULATION_STATE_RUNNING;

        if (!loadbios())
        {
                if (romset != -1)
                        return PCEM_ERROR_ROMSET_NOT_AVAILABLE;
                for (c = 0; c < ROM_MAX; c++)
                {
                        if (romspresent[c])
                        {
                                romset = c;
                                model = model_getmodel(romset);
                                break;
                        }
                }
        }

        if (!video_card_available(video_old_to_new(gfxcard)))
        {
                if (romset != -1)
                        return PCEM_ERROR_VIDEO_BIOS_NOT_AVAILABLE;
                for (c = GFX_MAX - 1; c >= 0; c--)
                {
                        if (gfx_present[c])
                        {
                                gfxcard = c;
                                break;
                        }
                }
        }

        loadbios();
        resetpchard();
        midi_init();

        main_thread = thread_create(mainthread, NULL);

        onesec_thread = thread_create(onesecthread, NULL);

        return 0;
}

void pcem_pause()
{
        if (emulation_state == PCEM_EMULATION_STATE_RUNNING)
                emulation_state = PCEM_EMULATION_STATE_PAUSED;
}

void pcem_resume()
{
        if (emulation_state == PCEM_EMULATION_STATE_PAUSED)
                emulation_state = PCEM_EMULATION_STATE_RUNNING;
}

void pcem_reset()
{
        pcem_pause();
        msleep(100);
        resetpchard();
        pcem_resume();
}

void pcem_stop()
{
        emulation_state = PCEM_EMULATION_STATE_STOPPED;
        pclog("Stopping emulation...\n");

        startblit();
        //display_stop();

        savenvr();
        saveconfig(NULL);
        if (pcem_config_save_func)
        {
                pcem_config_save_func(0);
                pcem_config_save_func(1);
        }
        pcem_config_close();

        endblit();

        thread_destroy_mutex(blit_mutex);

        thread_kill(main_thread);
        thread_kill(onesec_thread);

        device_close_all();
        midi_close();

        pclog("Emulation stopped.\n");

        closepc();
}

extern int fps;

int pcem_get_emulation_speed()
{
        return fps;
}

int pcem_get_emulation_state()
{
        return emulation_state;
}

void pcem_drive_eject(int drive)
{
        if (drive == PCEM_SYS_DRIVE_A)
                disc_close(0);
        else if (drive == PCEM_SYS_DRIVE_B)
                disc_close(1);
        else if (drive == PCEM_SYS_DRIVE_CD)
        {
                if (cdrom_drive == 0)
                {
                        /* Switch from empty to empty. Do nothing. */
                        return;
                }
                atapi->exit();
                image_close();
                old_cdrom_drive = cdrom_drive;
                cdrom_drive = 0;
        }
        else if (drive == PCEM_SYS_DRIVE_ZIP)
                zip_eject();
        else if (drive == PCEM_SYS_DRIVE_CASSETTE)
                cassette_eject();
}

void pcem_drive_load_image(int drive, const char* file)
{
        if (drive == PCEM_SYS_DRIVE_A)
        {
                disc_close(0);
                char *fn = strdup(file);
                disc_load(0, fn);
                saveconfig(NULL);
                if (pcem_config_save_func) pcem_config_save_func(1);
                free(fn);
        }
        else if (drive == PCEM_SYS_DRIVE_B)
        {
                disc_close(1);
                char *fn = strdup(file);
                disc_load(1, fn);
                saveconfig(NULL);
                if (pcem_config_save_func) pcem_config_save_func(1);
                free(fn);
        }
        else if (drive == PCEM_SYS_DRIVE_CD)
        {
                old_cdrom_drive = cdrom_drive;
                char *fn = strdup(file);
                if ((strcmp(image_path, fn) == 0) && (cdrom_drive == CDROM_IMAGE))
                {
                        /* Switching from ISO to the same ISO. Do nothing. */
                        return;
                }
                atapi->exit();
                image_close();
                image_open(fn);
                saveconfig(NULL);
                if (pcem_config_save_func) pcem_config_save_func(1);
                cdrom_drive = CDROM_IMAGE;
        }
        else if (drive == PCEM_SYS_DRIVE_ZIP)
        {
                zip_eject();
                char *fn = strdup(file);
                zip_load(fn);
                saveconfig(NULL);
                if (pcem_config_save_func) pcem_config_save_func(1);
                free(fn);
        }
        else if (drive == PCEM_SYS_DRIVE_CASSETTE)
        {
                cassette_eject();
                char *fn = strdup(file);
                cassette_load(fn);
                saveconfig(NULL);
                if (pcem_config_save_func) pcem_config_save_func(1);
                free(fn);
        }
}

void pcem_send_action(int action)
{
        if (action == PCEM_ACTION_CTRL_ALT_DEL)
                resetpc_cad();
        else if (action == PCEM_ACTION_SAVE_CONFIG)
        {
                saveconfig(NULL);
                if (pcem_config_save_func)
                {
                        pcem_config_save_func(0);
                        pcem_config_save_func(1);
                }
        }
}
