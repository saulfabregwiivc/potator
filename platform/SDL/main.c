#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "../../common/supervision.h"
#include "../../common/sound.h"

#define OR_DIE(cond) \
    if (cond) { \
        fprintf(stderr, "[Error] SDL: %s\n", SDL_GetError()); \
        exit(1); \
    }

#define SCREEN_W 160
#define SCREEN_H 160

SDL_bool done = SDL_FALSE;

uint8_t *buffer;
uint32_t bufferSize = 0;

uint16_t screenBuffer[SCREEN_W * SCREEN_H];
SDL_Surface *PrimarySurface;
int windowScale = 4;
int windowWidth = 0;
int windowHeight = 0;
int maxWindowWidth = 0;
int maxWindowHeight = 0;
SDL_bool isFullscreen = SDL_FALSE;
void ToggleFullscreen(void);
void NextPalette(void);
void UpdateWindowSize(void);
void IncreaseWindowSize(void);
void DecreaseWindowSize(void);
int currentGhosting = 0;
void IncreaseGhosting(void);
void DecreaseGhosting(void);

char romName[64];
void SetRomName(const char *path)
{
    const char *p = path + strlen(path);
    while (p != path) {
        if (*p == '\\' || *p == '/') {
            p++;
            break;
        }
        p--;
    }
    strncpy(romName, p, sizeof(romName));
    romName[sizeof(romName) - 1] = '\0';
}

int LoadROM(const char *filename)
{
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    SDL_RWops *romfile = SDL_RWFromFile(filename, "rb");
    if (romfile == NULL) {
        fprintf(stderr, "SDL_RWFromFile(): Unable to open file! (%s)\n", filename);
        return 1;
    }
    SDL_RWseek(romfile, 0, RW_SEEK_END);
    bufferSize = (uint32_t)SDL_RWtell(romfile);
    SDL_RWseek(romfile, 0, RW_SEEK_SET);
    buffer = (uint8_t *)malloc(bufferSize);
    SDL_RWread(romfile, buffer, bufferSize, 1);
    if (SDL_RWclose(romfile) != 0) {
        fprintf(stderr, "SDL_RWclose(): Unable to close file! (%s)\n", filename);
        return 1;
    }
    SetRomName(filename);
    return 0;
}

void PollEvents(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            done = SDL_TRUE;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
            case SDLK_RETURN:
                ToggleFullscreen();
                break;
            case SDLK_TAB:
                supervision_load(&buffer, bufferSize);
                break;
            case SDLK_1:
                supervision_save_state(romName, 0);
                break;
            case SDLK_2:
                supervision_load_state(romName, 0);
                break;
            case SDLK_p:
                NextPalette();
                break;
            case SDLK_MINUS:
                DecreaseWindowSize();
                break;
            case SDLK_EQUALS:
                IncreaseWindowSize();
                break;
            case SDLK_LEFTBRACKET:
                DecreaseGhosting();
                break;
            case SDLK_RIGHTBRACKET:
                IncreaseGhosting();
                break;
            case SDLK_m:
                SDL_PauseAudio(SDL_GetAudioStatus() == SDL_AUDIO_PLAYING);
                break;
            }
        }
        else if (event.type == SDL_VIDEORESIZE) {
            if (!isFullscreen) {
                int mins = event.resize.w < event.resize.h ? event.resize.w : event.resize.h;
                windowScale = mins / SCREEN_W;
                UpdateWindowSize();
            }
        }
    }
}

void HandleInput(void)
{
    uint8_t controls_state = 0;
    uint8_t *keystate = SDL_GetKeyState(NULL);

    if (keystate[SDLK_RIGHT]) controls_state |= 0x01;
    if (keystate[SDLK_LEFT])  controls_state |= 0x02;
    if (keystate[SDLK_DOWN])  controls_state |= 0x04;
    if (keystate[SDLK_UP])    controls_state |= 0x08;
    if (keystate[SDLK_x])     controls_state |= 0x10;
    if (keystate[SDLK_c])     controls_state |= 0x20;
    if (keystate[SDLK_z])     controls_state |= 0x40;
    if (keystate[SDLK_SPACE]) controls_state |= 0x80;

    controls_state_write(0, controls_state);
}

void Draw()
{
    SDL_LockSurface(PrimarySurface);
    uint32_t *pDest = (uint32_t *)PrimarySurface->pixels;
    uint16_t *pSrc = screenBuffer;

    if (isFullscreen) {
        // Center
        pDest += (windowWidth - SCREEN_W * windowScale) / 2 + (windowHeight - SCREEN_H * windowScale) / 2 * windowWidth;
    }
    for (int y = 0; y < SCREEN_H * windowScale; y++) {
        for (int x = 0; x < SCREEN_W * windowScale; x++) {
            //*pDest = ((*pSrc & 0x7C00) >> 10) | ((*pSrc & 0x03E0) << 1) | ((*pSrc & 0x001F) << 11); // -> SDL 16-bit
            //*pDest = ((*pSrc & 0x7C00) >> (10-3)) | ((*pSrc & 0x03E0) << (3+3)) | ((*pSrc & 0x001F) << (16+3)); // -> SDL 32-bit

            // RGB555 (R: 0-4 bits) -> BGRA8888 (SDL 32-bit)
            *pDest = SDL_MapRGB(PrimarySurface->format, (*pSrc & 0x001F) << 3, (*pSrc & 0x03E0) >> (5 - 3), (*pSrc & 0x7C00) >> (10 - 3));
            pDest++;
            if (x % windowScale == windowScale - 1) pSrc++;
        }
        if (y % windowScale != windowScale - 1) pSrc -= SCREEN_W;

        pDest += (windowWidth - windowScale * SCREEN_W);
    }

    SDL_UnlockSurface(PrimarySurface);
    SDL_Flip(PrimarySurface);
}

// Audio
void AudioCallback(void *userdata, uint8_t *stream, int len)
{
    sound_stream_update(stream, len);
}

uint32_t startCounter;
void InitCounter(void)
{
    startCounter = SDL_GetTicks();
}

SDL_bool NeedUpdate(void)
{
    static double elapsedCounter = 0.0;
    static SDL_bool result = SDL_FALSE;

    // New frame
    if (!result) {
        uint32_t now = SDL_GetTicks();
        elapsedCounter += (double)(now - startCounter);
        startCounter = now;
    }
    result = elapsedCounter >= 16.666; // 1000.0/60.0
    if (result) {
        elapsedCounter -= 16.666;
    }
    return result;
}

// SDL Fix
#ifdef main
#undef main
#endif

int main(int argc, char *argv[])
{
    OR_DIE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0);

    SDL_WM_SetCaption("Potator (SDL1)", NULL);

    UpdateWindowSize();
    SDL_Rect **modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
    maxWindowWidth  = modes[0]->w;
    maxWindowHeight = modes[0]->h;

    SDL_FillRect(PrimarySurface, NULL, 0);

    SDL_AudioSpec audio_spec;
    memset(&audio_spec, 0, sizeof(audio_spec));
    audio_spec.freq = BPS;
    audio_spec.channels = 2;
    audio_spec.samples = 512;
    audio_spec.format = AUDIO_S8;
    audio_spec.callback = AudioCallback;
    audio_spec.userdata = NULL;
    OR_DIE(SDL_OpenAudio(&audio_spec, NULL) < 0);

    printf("# Controls\n\
               Right: Right\n\
                Left: Left\n\
                Down: Down\n\
                  Up: Up\n\
                   B: X\n\
                   A: C\n\
              Select: Z\n\
               Start: Space\n\
   Toggle Fullscreen: Return\n\
               Reset: Tab\n\
          Save State: 1\n\
          Load State: 2\n\
        Next Palette: P\n\
Decrease Window Size: -\n\
Increase Window Size: =\n\
   Decrease Ghosting: [\n\
   Increase Ghosting: ]\n\
          Mute Audio: M\n");

    supervision_init();

    if (LoadROM(argc <= 1 ? "rom.sv" : argv[1]) == 0) {
        supervision_load(&buffer, bufferSize);
    }
    else {
        done = SDL_TRUE;
    }

    SDL_PauseAudio(0);

    InitCounter();
    while (!done) {
        PollEvents();
        HandleInput();
        while (NeedUpdate()) {
            supervision_exec(screenBuffer);
        }
        Draw();
    }
    supervision_done();

    SDL_CloseAudio();

    SDL_Quit();
}

#define VIDEOMODE_FLAGS (SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE)

void ToggleFullscreen(void)
{
    static int lastWidth = 0;
    static int lastHeight = 0;
    static int lastScale = 0;

    uint32_t flags = VIDEOMODE_FLAGS;
    if (isFullscreen) {
        windowWidth  = lastWidth;
        windowHeight = lastHeight;
        windowScale  = lastScale;
        SDL_ShowCursor(SDL_ENABLE);
    }
    else {
        lastWidth  = windowWidth;
        lastHeight = windowHeight;
        lastScale  = windowScale;

        flags |= SDL_FULLSCREEN;
        windowScale  = maxWindowHeight / SCREEN_H;
        windowWidth  = maxWindowWidth;
        windowHeight = maxWindowHeight;
        SDL_ShowCursor(SDL_DISABLE);
    }
    PrimarySurface = SDL_SetVideoMode(windowWidth, windowHeight, 32, flags);
    OR_DIE(PrimarySurface == NULL);

    isFullscreen = !isFullscreen;
}

void NextPalette(void)
{
    static int currentPalette = 0;
    currentPalette = (currentPalette + 1) % COLOUR_SCHEME_COUNT;
    supervision_set_colour_scheme(currentPalette);
}

void UpdateWindowSize(void)
{
    windowWidth  = SCREEN_W * windowScale;
    windowHeight = SCREEN_H * windowScale;
    PrimarySurface = SDL_SetVideoMode(windowWidth, windowHeight, 32, VIDEOMODE_FLAGS);
    OR_DIE(PrimarySurface == NULL);
}

void IncreaseWindowSize(void)
{
    if (isFullscreen)
        return;

    if (SCREEN_W * (windowScale + 1) <= maxWindowWidth && SCREEN_H * (windowScale + 1) <= maxWindowHeight) {
        windowScale++;
        UpdateWindowSize();
    }
}

void DecreaseWindowSize(void)
{
    if (isFullscreen)
        return;

    if (windowScale > 1) {
        windowScale--;
        UpdateWindowSize();
    }
}

void IncreaseGhosting(void)
{
    if (currentGhosting < GHOSTING_MAX) {
        currentGhosting++;
        supervision_set_ghosting(currentGhosting);
    }
}

void DecreaseGhosting(void)
{
    if (currentGhosting > 0) {
        currentGhosting--;
        supervision_set_ghosting(currentGhosting);
    }
}
