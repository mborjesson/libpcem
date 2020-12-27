#ifndef __PCEM_H_
#define __PCEM_H_

#define PCEM_ERROR_ROMSET_NOT_AVAILABLE 10
#define PCEM_ERROR_VIDEO_BIOS_NOT_AVAILABLE 11

#define PCEM_EMULATION_STATE_STOPPED 1
#define PCEM_EMULATION_STATE_RUNNING 2
#define PCEM_EMULATION_STATE_PAUSED 3

#define PCEM_AUDIO_STREAM_DEFAULT 1
#define PCEM_AUDIO_STREAM_CD 2

#define PCEM_SYS_DRIVE_A 1
#define PCEM_SYS_DRIVE_B 2
#define PCEM_SYS_DRIVE_CD 3
#define PCEM_SYS_DRIVE_ZIP 4
#define PCEM_SYS_DRIVE_CASSETTE 5

#define PCEM_CONFIG_TYPE_INT 1
#define PCEM_CONFIG_TYPE_FLOAT 2
#define PCEM_CONFIG_TYPE_STRING 3

#define PCEM_ACTION_SAVE_CONFIG 1
#define PCEM_ACTION_CTRL_ALT_DEL 2

#define PCEM_EVENT_BLIT_BEGIN 1
#define PCEM_EVENT_BLIT_END 2
#define PCEM_EVENT_EMULATION_HALTED 3
#define PCEM_EVENT_AUDIO_INIT 4

typedef void (*pcem_video_size_t)(int width, int height);
typedef void (*pcem_video_blit_draw_t)(int x1, int x2, int y1, int y2, int offset_x, int offset_y, void *buffer, int buffer_width, int buffer_height, int buffer_channels);

typedef void (*pcem_input_keyboard_poll_t)(void* states);
typedef void (*pcem_input_mouse_poll_t)(int *x, int *y, int *z, int *buttons);

typedef void (*pcem_audio_stream_create_t)(int stream, int sample_rate, int sample_size_in_bits, int channels, int buffer_length);
typedef void (*pcem_audio_stream_data_t)(int stream, void *buffer, int buffer_length);

typedef int (*pcem_config_get_t)(int type, int is_global, const char *section, const char *name, void *value);
typedef int (*pcem_config_set_t)(int type, int is_global, const char *section, const char *name, void *value);
typedef void (*pcem_config_save_t)(int is_global);

typedef void (*pcem_on_event_t)(int is_global);

/**
 * \brief A callback for when certain events happens.
 */
extern void pcem_callback_on_event(pcem_on_event_t func);

/**
 * \brief A callback for when the video size has been updated.
 */
extern void pcem_callback_video_size(pcem_video_size_t func);
/**
 * \brief A callback for when the video buffer has been updated.
 */
extern void pcem_callback_video_blit_draw(pcem_video_blit_draw_t func);

/**
 * \brief A callback for when the emulator wants to poll the keyboard state.
 */
extern void pcem_callback_input_keyboard_poll(pcem_input_keyboard_poll_t func);
/**
 * \brief A callback for when the emulator wants to poll the mouse state.
 */
extern void pcem_callback_input_mouse_poll(pcem_input_mouse_poll_t func);

/**
 * \brief A callback for when an audio stream should be created.
 */
extern void pcem_callback_audio_stream_create(pcem_audio_stream_create_t func);
/**
 * \brief A callback for when there is new audio available.
 */
extern void pcem_callback_audio_stream_data(pcem_audio_stream_data_t func);

/**
 * \brief A callback for when a configuration option is read.
 */
extern void pcem_callback_config_get(pcem_config_get_t func);
/**
 * \brief A callback for when a configuration option is written.
 */
extern void pcem_callback_config_set(pcem_config_set_t func);
/**
 * \brief A callback for when a configuration should be saved.
 */
extern void pcem_callback_config_save(pcem_config_save_t func);

/**
 * \brief Set the path to where the emulator can find ROMs.
 */
extern void pcem_set_roms_paths(const char* path);
/**
 * \brief Set the path to where the emulator will write NVR-files.
 */
extern void pcem_set_nvr_path(const char* path);
/**
 * \brief Set the path to where the emulator will write its logs.
 */
extern void pcem_set_logs_path(const char* path);

/**
 * \brief Returns the current emulation speed, in percent.
 *
 * \return A value between 0 and 100.
 */
extern int pcem_get_emulation_speed();

/**
 * \brief Returns the current emulation state.
 *
 * \return Either PCEM_EMULATION_STATE_STOPPED, PCEM_EMULATION_STATE_STARTED or PCEM_EMULATION_STATE_PAUSED.
 */
extern int pcem_get_emulation_state();

/**
 * \brief Load an image to the specified drive.
 * 
 * \param drive Can be any of PCEM_DRIVE_*
 */
extern void pcem_drive_load_image(int drive, const char* file);
/**
 * \brief Eject the specified drive.
 * 
 * \param drive Can be any of PCEM_DRIVE_*
 */
extern void pcem_drive_eject(int drive);

/**
 * \brief Start the emulation.
 */
extern int pcem_start();
/**
 * \brief Hard reset the emulation.
 */
extern void pcem_reset();
/**
 * \brief Stop the emulation.
 */
extern void pcem_stop();
/**
 * \brief Pause the emulation.
 */
extern void pcem_pause();
/**
 * \brief Resume the emulation.
 */
extern void pcem_resume();

/**
 * \brief Send an action to the emulator.
 * 
 * \param action Can be any of PCEM_ACTION_*
 */
extern void pcem_send_action(int action);

#endif /* __PCEM_H_ */
