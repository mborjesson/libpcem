# libpcem

Inspired by #doscember I also wanted to do something DOS-related this month. The result is this project, where the goal was to see if it would be possible to build [PCem](https://pcem-emulator.co.uk/) as a library and use it in Java.

It should have a simple API and the features that I needed to get DOS up and running with graphics, sound and mouse-/keyboard-input.

I was able to build it as a library and use it in Java through BridJ, however I quickly discovered that it instantly crashed when the dynamic recompiler was enabled. When testing it in C I found out that it worked when linked as a static library, so the dynamic recompiler is most likely not PIC-compliant which means that the emulation will be very slow when used a shared library (the cpu_use_dynarec-setting will be forced off).

## Please note

This project has been uploaded as-is and will not be actively developed right now. If there is a lot of interest for it or if the dynamic recompiler becomes PIC-compliant in the future I might work more on it.

The code was written and has only been tested on Linux and will likely not work on other systems, mostly because I have no easy access to Windows- and Mac-systems.

## How to build

Run these commands the first time to generate configuration files:
```
libtoolize
autoreconf
```

Then to build it:
```
./configure --enable-release-library
make
```

This will by default create a static and a shared library in src/.libs/.

## How to use

Check the pcem.h and pcem-config.h for all the exported functions, they all start with pcem_*. Many of the functions are for callbacks, like when there's something to render or when the keyboard- and mouse-state is polled. The configuration of the emulated machine is expected to be handled manually with the pcem_config_get, pcem_config_set and pcem_config_save functions. For convenience there's pcem_config_simple_init and pcem_config_simple_close in pcem-config.h which will make it behave a bit more automatically.

Here's a simple example that loads a configuration, sets up callbacks for rendering, audio and input polling. Then it starts the emulation, lets it run for 4 seconds and then stops:

```c++
#include <stdio.h>
#include <unistd.h>

extern "C" {
	#include "pcem.h"
	#include "pcem-config.h"
}

void on_video_size(int width, int height)
{
	printf("Video size changed: %dx%d\n", width, height);
}

void on_video_blit_draw(int x1, int x2, int y1, int y2, int offset_x, int offset_y, void *buffer, int buffer_width, int buffer_height, int buffer_channels)
{
	printf("Draw rect %d, %d -> %d, %d (%d, %d, %dx%d %d channels)\n", x1, y1, x2, y2, offset_x, offset_y, buffer_width, buffer_height, buffer_channels);
}

void on_input_keyboard_poll(void *states)
{
	printf("Keyboard poll\n");
}

void on_input_mouse_poll(int *x, int *y, int *z, int *buttons)
{
	printf("Mouse poll\n");
}

void on_audio_stream_create(int stream, int sample_rate, int sample_size_in_bits, int channels, int buffer_length)
{
	printf("Create audio stream %d: %d hz, %d bits, %d channels (buffer length: %d)\n", stream, sample_rate, sample_size_in_bits, channels, buffer_length);
}

void on_audio_stream_data(int stream, void *buffer, int buffer_length)
{
	printf("Audio stream %d data (buffer_length: %d)\n", stream, buffer_length);
}

int main(int argc, char** argv) {
	int result;

	printf("libpcem simple example\n");

	// load configuration read only
	if (pcem_config_simple_init("pcem.cfg", "configs/DOS.cfg", 1))
	{
		printf("Could not load configs.");
		return 1;
	}

	// set up callbacks
	pcem_callback_video_size(on_video_size);
	pcem_callback_video_blit_draw(on_video_blit_draw);
	pcem_callback_input_keyboard_poll(on_input_keyboard_poll);
	pcem_callback_input_mouse_poll(on_input_mouse_poll);
	pcem_callback_audio_stream_create(on_audio_stream_create);
	pcem_callback_audio_stream_data(on_audio_stream_data);

	// start emulation
	result = pcem_start();
	if (result != 0)
	{
		printf("Could not start PCem (%d)\n", result);
		return 2;
	}

	// wait a bit
	sleep(4);

	printf("Shutting down...\n");

	// shut down
	pcem_stop();
	pcem_config_simple_close();

	printf("All done.\n");

	return 0;
}}
```

