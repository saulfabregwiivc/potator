#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/supervision.h"
#include "../../common/sound.h"

#include <SDL.h>

#define OR_DIE(cond) \
    if (cond) { \
        fprintf(stderr, "[Error] SDL: %s\n", SDL_GetError()); \
        exit(1); \
    }

SDL_bool paused = SDL_FALSE;

uint16_t screenBuffer[160 * 160];

SDL_Window *sdlScreen;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;

uint8_t *buffer;
uint32_t bufferSize = 0;

SDL_bool IsFullscreen(void);
void ToggleFullscreen(void);

int LoadROM(const char *filename)
{
    if (buffer != 0)
        free(buffer);

    FILE *romfile = fopen(filename, "rb");
    if (romfile == NULL) {
        printf("fopen(): Unable to open file!\n");
        return 1;
    }
    fseek(romfile, 0, SEEK_END);
    bufferSize = ftell(romfile);
    fseek(romfile, 0, SEEK_SET);

    buffer = (uint8_t *)malloc(bufferSize);

    fread(buffer, bufferSize, 1, romfile);

    if (fclose(romfile) == EOF) {
        printf("fclose(): Unable to close file!\n");
        return 1;
    }
    return 0;
}

void HandleInput(void)
{
    uint8_t controls_state = 0;
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);

    if (keystate[SDL_SCANCODE_RIGHT]) controls_state |= 0x01;
    if (keystate[SDL_SCANCODE_LEFT])  controls_state |= 0x02;
    if (keystate[SDL_SCANCODE_DOWN])  controls_state |= 0x04;
    if (keystate[SDL_SCANCODE_UP])    controls_state |= 0x08;
    if (keystate[SDL_SCANCODE_X])     controls_state |= 0x10; // B
    if (keystate[SDL_SCANCODE_C])     controls_state |= 0x20; // A
    if (keystate[SDL_SCANCODE_Z])     controls_state |= 0x40; // Select
    if (keystate[SDL_SCANCODE_SPACE]) controls_state |= 0x80; // Start

    controls_state_write(0, controls_state);
}

void AudioCallback(void *userdata, uint8_t *stream, int len)
{
    //SDL_memset(stream, 0, len);
    sound_stream_update(stream, len/4);
    // U8 to F32
    int i;
    float *s = (float*)(stream + len) - 1;
    for (i = len/4 - 1; i >= 0; i--) {
        *s-- = stream[i] / 255.0f;
    }
}

void Render(void)
{
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
}

void PollEvents(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            paused = SDL_TRUE;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    ToggleFullscreen();
                    break;
                case SDLK_1:
                    sv_saveState("rom   ", 0);
                    break;
                case SDLK_2:
                    sv_loadState("rom   ", 0);
                    break;
            }
        }
        else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            if (!IsFullscreen())
                SDL_SetWindowSize(sdlScreen, (event.window.data1 + 80) / 160 * 160, (event.window.data2 + 80) / 160 * 160);
        }
    }
}

int main(int argc, char *argv[])
{
    OR_DIE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0);

    sdlScreen = SDL_CreateWindow("Potator (SDL2)",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        160*3, 160*3,
        SDL_WINDOW_RESIZABLE);
    OR_DIE(sdlScreen == NULL);
    
    sdlRenderer = SDL_CreateRenderer(sdlScreen, -1, SDL_RENDERER_PRESENTVSYNC);
    OR_DIE(sdlRenderer == NULL);
    
    SDL_RenderSetLogicalSize(sdlRenderer, 160, 160);
    
    sdlTexture = SDL_CreateTexture(sdlRenderer,
        SDL_PIXELFORMAT_RGB555,
        SDL_TEXTUREACCESS_STREAMING,
        160, 160);
    OR_DIE(sdlTexture == NULL);

    SDL_AudioSpec audio_spec;
    SDL_memset(&audio_spec, 0, sizeof(SDL_AudioSpec));
    audio_spec.freq = BPS;
    audio_spec.channels = 2;
    audio_spec.samples = 512;
    audio_spec.format = AUDIO_F32; // Problem with U8
    audio_spec.callback = AudioCallback;
    audio_spec.userdata = NULL;
    //SDL_OpenAudio(&audio_spec, NULL);
    SDL_AudioDeviceID devid = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
    OR_DIE(devid == 0);

    if (argc <= 1) {
        LoadROM("rom.sv");
    }
    else {
        LoadROM(argv[1]);
    }

    supervision_init();
    supervision_load(buffer, bufferSize);

    //SDL_PauseAudio(0);
    SDL_PauseAudioDevice(devid, 0);

    while (!paused) {
        PollEvents();

        HandleInput();

        // Emulate
        supervision_exec(screenBuffer);

        // Draw
        SDL_UpdateTexture(sdlTexture, NULL, screenBuffer, 160 * sizeof(uint16_t));

        Render();
    }

    supervision_done();

    //SDL_CloseAudio();
    SDL_CloseAudioDevice(devid);

    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlScreen);

    SDL_Quit();
    return 0;
}

SDL_bool IsFullscreen(void)
{
    return 0 != (SDL_GetWindowFlags(sdlScreen) & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void ToggleFullscreen(void)
{
    static int mouseX;
    static int mouseY;
    static SDL_bool cursorInWindow = SDL_FALSE;

    if (!IsFullscreen()) {
        SDL_SetWindowFullscreen(sdlScreen, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_DISABLE);

        int x, y;
        SDL_GetMouseState(&x, &y);
        SDL_GetGlobalMouseState(&mouseX, &mouseY);
        if (x == mouseX && y == mouseY) {
            cursorInWindow = SDL_TRUE;
        }
    }
    else {
        SDL_SetWindowFullscreen(sdlScreen, 0);
        SDL_ShowCursor(SDL_ENABLE);

        // Don't move cursor
        if (cursorInWindow) {
            SDL_WarpMouseInWindow(sdlScreen, mouseX, mouseY);
        }
        else {
            SDL_WarpMouseGlobal(mouseX, mouseY);
        }
        cursorInWindow = SDL_FALSE;
    }
}
