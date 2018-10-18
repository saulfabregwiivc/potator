#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "../../common/supervision.h"
#include "../../common/sound.h"

#define OR_DIE(cond) \
    if (cond) { \
        fprintf(stderr, "[Error] SDL: %s\n", SDL_GetError()); \
        exit(1); \
    }

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define SCREEN_W 160
#define SCREEN_H 160

typedef enum {
    MENUSTATE_NONE,
    MENUSTATE_DROP_ROM,
    MENUSTATE_EMULATION,
    MENUSTATE_PAUSE,
    MENUSTATE_SET_KEY
} MenuState;

SDL_bool done = SDL_FALSE;
uint16_t screenBuffer[SCREEN_W * SCREEN_H];
SDL_Window *sdlScreen;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;

uint8_t *buffer;
uint32_t bufferSize = 0;

SDL_GameController *controller = NULL;

SDL_bool IsFullscreen(void);
void ToggleFullscreen(void);
uint64_t startCounter = 0;
SDL_bool isRefreshRate60 = SDL_FALSE;
void InitCounter(void);
SDL_bool NeedUpdate(void);
void DrawDropROM(void);

int nextMenuState = 0;
MenuState menuStates[4];
void PushMenuState(MenuState state);
void PopMenuState(void);
MenuState GetMenuState(void);

void Reset(void);

int currentPalette = 0;
void NextPalette(void);

int windowScale = 4;
void IncreaseWindowSize(void);
void DecreaseWindowSize(void);

void SaveState(void);
void LoadState(void);

int audioVolume = SDL_MIX_MAXVOLUME;
void SetVolume(int volume);
void MuteAudio(void);

int currentGhosting = 0;
void IncreaseGhosting(void);
void DecreaseGhosting(void);

char *keysNames[] = {
    "Right",
    "Left",
    "Down",
    "Up",
    "B",
    "A",
    "Select",
    "Start",

    "Toggle Fullscreen",
    "Reset",
    "Save State",
    "Load State",
    "Next Palette",
    "Decrease Window Size",
    "Increase Window Size",
    "Decrease Ghosting",
    "Increase Ghosting",
    "Mute Audio",
};
int keysMapping[] = {
    SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_UP,
    SDL_SCANCODE_X,     // B
    SDL_SCANCODE_C,     // A
    SDL_SCANCODE_Z,     // Select
    SDL_SCANCODE_SPACE, // Start

    SDL_SCANCODE_RETURN,
    SDL_SCANCODE_TAB,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_P,
    SDL_SCANCODE_MINUS,
    SDL_SCANCODE_EQUALS,
    SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_RIGHTBRACKET,
    SDL_SCANCODE_M,
};
void (*keysFuncs[])(void) = {
    ToggleFullscreen,
    Reset,
    SaveState,
    LoadState,
    NextPalette,
    DecreaseWindowSize,
    IncreaseWindowSize,
    DecreaseGhosting,
    IncreaseGhosting,
    MuteAudio,
};
int setButton = -1;
void SetKey(int button);

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
        fprintf(stderr, "SDL_RWFromFile(): Unable to open file!\n");
        return 1;
    }
    bufferSize = (uint32_t)SDL_RWsize(romfile);
    buffer = (uint8_t *)malloc(bufferSize);
    SDL_RWread(romfile, buffer, bufferSize, 1);
    if (SDL_RWclose(romfile) != 0) {
        fprintf(stderr, "SDL_RWclose(): Unable to close file!\n");
        return 1;
    }
    SetRomName(filename);
    return 0;
}

//int LoadROM(const char *filename)
//{
//    if (buffer != NULL) {
//        free(buffer);
//        buffer = NULL;
//    }
//
//    FILE *romfile = fopen(filename, "rb");
//    if (romfile == NULL) {
//        printf("fopen(): Unable to open file!\n");
//        return 1;
//    }
//    fseek(romfile, 0, SEEK_END);
//    bufferSize = ftell(romfile);
//    fseek(romfile, 0, SEEK_SET);
//
//    buffer = (uint8_t *)malloc(bufferSize);
//
//    fread(buffer, bufferSize, 1, romfile);
//
//    if (fclose(romfile) == EOF) {
//        printf("fclose(): Unable to close file!\n");
//        return 1;
//    }
//    SetRomName(filename);
//    return 0;
//}

void LoadBuffer(void)
{
    supervision_load(&buffer, bufferSize);
    MenuState prevState = GetMenuState();
    PopMenuState();
    PushMenuState(MENUSTATE_EMULATION);
    if (prevState == MENUSTATE_PAUSE) { // Focus wasn't gained
        PushMenuState(MENUSTATE_PAUSE);
    }
    supervision_set_color_scheme(currentPalette);
    supervision_set_ghosting(currentGhosting);
}

void AudioCallback(void *userdata, uint8_t *stream, int len)
{
    // The alternative of SDL_PauseAudio()
    //if (GetMenuState() != MENUSTATE_EMULATION) {
    //    SDL_memset(stream, 0, len);
    //    return;
    //}

    // U8 to F32
    sound_stream_update(stream, len / 4);
    float *s = (float*)(stream + len) - 1;
    for (int i = len / 4 - 1; i >= 0; i--) {
        // 127 - max
        *s-- = stream[i] / 127.0f * audioVolume / (float)SDL_MIX_MAXVOLUME;
    }

    // U8 or S8
    /*sound_stream_update(stream, len);
    for (int i = 0; i < len; i++) {
        stream[i] = (uint8_t)(stream[i] * audioVolume / (float)SDL_MIX_MAXVOLUME);
    }*/
}

void HandleInput(void)
{
    uint8_t controls_state = 0;
    const uint8_t *keystate = SDL_GetKeyboardState(NULL);

    if (keystate[keysMapping[0]]) controls_state |= 0x01;
    if (keystate[keysMapping[1]]) controls_state |= 0x02;
    if (keystate[keysMapping[2]]) controls_state |= 0x04;
    if (keystate[keysMapping[3]]) controls_state |= 0x08;
    if (keystate[keysMapping[4]]) controls_state |= 0x10; // B
    if (keystate[keysMapping[5]]) controls_state |= 0x20; // A
    if (keystate[keysMapping[6]]) controls_state |= 0x40; // Select
    if (keystate[keysMapping[7]]) controls_state |= 0x80; // Start

    if (SDL_GameControllerGetAttached(controller)) {
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) controls_state |= 0x01;
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  controls_state |= 0x02;
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  controls_state |= 0x04;
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))    controls_state |= 0x08;
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))          controls_state |= 0x10;
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X))          controls_state |= 0x20;
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK))       controls_state |= 0x40;
        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))      controls_state |= 0x80;
        // 31130/32768 == 0.95
        if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) >  31130) controls_state |= 0x01;
        if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) < -31130) controls_state |= 0x02;
        if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) >  31130) controls_state |= 0x04;
        if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) < -31130) controls_state |= 0x08;
    }

    controls_state_write(0, controls_state);
}

void PollEvents(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            done = SDL_TRUE;
        }
        else if (event.type == SDL_KEYDOWN) {
            if (GetMenuState() == MENUSTATE_SET_KEY) {
                if (event.key.keysym.scancode != SDL_SCANCODE_ESCAPE)
                    keysMapping[setButton] = event.key.keysym.scancode;
                PopMenuState();
                SDL_PauseAudio(0);
                continue;
            }
            for (int i = 0; i < COUNT_OF(keysFuncs); i++) {
                if (keysMapping[i + 8] == event.key.keysym.scancode) {
                    keysFuncs[i]();
                    break;
                }
            }
        }
        // SDL_CONTROLLERBUTTONDOWN doesn't work
        else if (event.type == SDL_JOYBUTTONDOWN) {
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) {
                SaveState();
            }
            else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
                LoadState();
            }
        }
        else if (event.type == SDL_WINDOWEVENT) {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                if (!IsFullscreen()) {
                    SDL_SetWindowSize(sdlScreen,
                        (event.window.data1 + SCREEN_W / 2) / SCREEN_W * SCREEN_W,
                        (event.window.data2 + SCREEN_H / 2) / SCREEN_H * SCREEN_H);
                }
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                if (GetMenuState() == MENUSTATE_SET_KEY) PopMenuState();
                PushMenuState(MENUSTATE_PAUSE);
                SDL_PauseAudio(1);
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                if (GetMenuState() == MENUSTATE_PAUSE) PopMenuState();
                SDL_PauseAudio(0);
                break;
            }
        }
        else if (event.type == SDL_DROPFILE) {
            if (LoadROM(event.drop.file) == 0) {
                LoadBuffer();
            }
            SDL_free(event.drop.file);
        }
        else if (event.type == SDL_JOYDEVICEADDED) {
            if (SDL_IsGameController(event.jdevice.which)) {
                controller = SDL_GameControllerOpen(event.jdevice.which);
                if (!controller) {
                    fprintf(stderr, "Could not open gamecontroller %i: %s\n", event.jdevice.which, SDL_GetError());
                }
            }
        }
    }
}

void Loop(void)
{
    PollEvents();

    HandleInput();

    while (NeedUpdate()) {
        switch (GetMenuState()) {
        case MENUSTATE_EMULATION:
            supervision_exec(screenBuffer);
        break;
        case MENUSTATE_DROP_ROM:
            DrawDropROM();
            break;
        case MENUSTATE_PAUSE:
        case MENUSTATE_SET_KEY:
            break;
        default:
            break;
        }
    }

    // Draw
    SDL_UpdateTexture(sdlTexture, NULL, screenBuffer, SCREEN_W * sizeof(uint16_t));

    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
#ifdef __EMSCRIPTEN__
    if (done) {
        emscripten_cancel_main_loop();
    }
#endif
}

int main(int argc, char *argv[])
{
    OR_DIE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0);

    sdlScreen = SDL_CreateWindow("Potator (SDL2)",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W * windowScale, SCREEN_H * windowScale,
        SDL_WINDOW_RESIZABLE);
    OR_DIE(sdlScreen == NULL);

    sdlRenderer = SDL_CreateRenderer(sdlScreen, -1, SDL_RENDERER_PRESENTVSYNC);
    OR_DIE(sdlRenderer == NULL);

    SDL_RenderSetLogicalSize(sdlRenderer, SCREEN_W, SCREEN_H);

    sdlTexture = SDL_CreateTexture(sdlRenderer,
        SDL_PIXELFORMAT_BGR555,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W, SCREEN_H);
    OR_DIE(sdlTexture == NULL);

    SDL_AudioSpec audio_spec;
    SDL_zero(audio_spec);
    audio_spec.freq = SV_SAMPLE_RATE;
    audio_spec.channels = 2;
    audio_spec.samples = 512;
    audio_spec.format = AUDIO_F32; // Or AUDIO_S8. Problem with AUDIO_U8
    audio_spec.callback = AudioCallback;
    audio_spec.userdata = NULL;
    OR_DIE(SDL_OpenAudio(&audio_spec, NULL) < 0);
    //SDL_AudioDeviceID devid = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
    //OR_DIE(devid == 0);

    printf("# Controls\n");
    for (int i = 0; i < COUNT_OF(keysNames); i++) {
        printf("%20s: %s\n", keysNames[i], SDL_GetScancodeName(keysMapping[i]));
    }
    printf("\n");

    supervision_init();

    PushMenuState(MENUSTATE_DROP_ROM);
    if (LoadROM(argc <= 1 ? "rom.sv" : argv[1]) == 0) {
        LoadBuffer();
    }

    SDL_PauseAudio(0);
    //SDL_PauseAudioDevice(devid, 0);

    InitCounter();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(Loop, 0, 1);
#else
    while (!done) {
        Loop();
    }
#endif

    supervision_done();

    SDL_CloseAudio();
    //SDL_CloseAudioDevice(devid);

    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlScreen);

    SDL_Quit();
    return 0;
}

#define FULLSCREEN_FLAG SDL_WINDOW_FULLSCREEN_DESKTOP

SDL_bool IsFullscreen(void)
{
    return 0 != (SDL_GetWindowFlags(sdlScreen) & FULLSCREEN_FLAG);
}

void ToggleFullscreen(void)
{
    static int mouseX;
    static int mouseY;
    static SDL_bool cursorInWindow = SDL_FALSE;
#ifdef __EMSCRIPTEN__
    return;
#endif
    if (!IsFullscreen()) {
        SDL_SetWindowFullscreen(sdlScreen, FULLSCREEN_FLAG);
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

        // Don't move cursor. Bug?
        if (cursorInWindow) {
            SDL_WarpMouseInWindow(sdlScreen, mouseX, mouseY);
        }
        else {
            SDL_WarpMouseGlobal(mouseX, mouseY);
        }
        cursorInWindow = SDL_FALSE;
    }
}

void InitCounter(void)
{
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    isRefreshRate60 = (current.refresh_rate == 60);
    startCounter = SDL_GetPerformanceCounter();
}

SDL_bool NeedUpdate(void)
{
    static double elapsedCounter = 0.0;
    static SDL_bool result = SDL_FALSE;

    if (isRefreshRate60) {
        result = !result;
    }
    else {
        // New frame
        if (!result) {
            uint64_t now = SDL_GetPerformanceCounter();
            elapsedCounter += (double)((now - startCounter) * 1000) / SDL_GetPerformanceFrequency();
            startCounter = now;
        }
        result = elapsedCounter >= 16.666;
        if (result) {
            elapsedCounter -= 16.666;
        }
    }
    return result;
}

void DrawDropROM(void)
{
    static uint8_t fade = 0;
    char dropRom[] = {
        "##  ##  ### ##   ##  ### # #"
        "# # # # # # # #  # # # # ###"
        "# # ##  # # ##   ##  # # # #"
        "##  # # ### #    # # ### # #"
    };
    uint8_t f = (fade < 32) ? fade : 63 - fade;
    uint16_t color = (f << 0) | (f << 5) | (f << 10);
    fade = (fade + 1) % 64;

    int width = 28, height = 4;
    int scale = 4, start = (SCREEN_W - width * scale) / 2 + SCREEN_W * (SCREEN_H - height * scale) / 2;
    for (int j = 0; j < height * scale; j++) {
        for (int i = 0; i < width * scale; i++) {
            if (dropRom[i / scale + width * (j / scale)] == '#')
                screenBuffer[start + i + SCREEN_W * j] = color;
        }
    }
}

void PushMenuState(MenuState state)
{
    if (nextMenuState == 0 || (nextMenuState > 0 && menuStates[nextMenuState - 1] != state)) {
        menuStates[nextMenuState] = state;
        nextMenuState++;
    }
}

void PopMenuState(void)
{
    if (nextMenuState > 0)
        nextMenuState--;
}

MenuState GetMenuState(void)
{
    if (nextMenuState > 0)
        return menuStates[nextMenuState - 1];
    return MENUSTATE_NONE;
}

// Emscripten
void UploadROM(void *newBuffer, int newBufferSize, const char *fileName)
{
    bufferSize = newBufferSize;
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    buffer = (uint8_t *)malloc(bufferSize);
    memcpy(buffer, newBuffer, bufferSize);

    SetRomName(fileName);
    LoadBuffer();
}

void Reset(void)
{
    if (GetMenuState() == MENUSTATE_EMULATION)
        LoadBuffer();
}

void NextPalette(void)
{
    currentPalette = (currentPalette + 1) % SV_COLOR_SCHEME_COUNT;
    supervision_set_color_scheme(currentPalette);
}

void IncreaseWindowSize(void)
{
    if (IsFullscreen())
        return;

    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);
    if (SCREEN_W * (windowScale + 1) <= dm.w && SCREEN_H * (windowScale + 1) <= dm.h) {
        windowScale++;
        SDL_SetWindowSize(sdlScreen, SCREEN_W * windowScale, SCREEN_H * windowScale);
    }
}

void DecreaseWindowSize(void)
{
    if (IsFullscreen())
        return;

    if (windowScale > 1) {
        windowScale--;
        SDL_SetWindowSize(sdlScreen, SCREEN_W * windowScale, SCREEN_H * windowScale);
    }
}

void SaveState(void)
{
    if (GetMenuState() == MENUSTATE_EMULATION)
        supervision_save_state(romName, 0);
}

void LoadState(void)
{
    if (GetMenuState() == MENUSTATE_EMULATION)
        supervision_load_state(romName, 0);
}

void SetVolume(int volume)
{
    if (volume < 0)
        audioVolume = 0;
    else if (volume > SDL_MIX_MAXVOLUME)
        audioVolume = SDL_MIX_MAXVOLUME; // 128
    else
        audioVolume = volume;
}

void MuteAudio(void)
{
    static int lastVolume = -1;
    if (lastVolume == -1) {
        lastVolume = audioVolume;
        audioVolume = 0;
    }
    else {
        audioVolume = lastVolume;
        lastVolume = -1;
    }
}

void IncreaseGhosting(void)
{
    if (currentGhosting < SV_GHOSTING_MAX) {
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

void SetKey(int button)
{
    setButton = button;
    PushMenuState(MENUSTATE_SET_KEY);
    SDL_PauseAudio(1);
}
