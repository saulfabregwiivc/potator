#ifndef _MSC_VER
#include <stdbool.h>
#include <sched.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "libretro.h"
#include "libretro_core_options.h"

#include "sound.h"
#include "memorymap.h"
#include "supervision.h"
#include "controls.h"
#include "types.h"

#ifdef _3DS
extern void* linearMemAlign(size_t size, size_t alignment);
extern void linearFree(void* mem);
#endif

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

static bool libretro_supports_bitmasks = false;

#define VIDEO_WIDTH  160
#define VIDEO_HEIGHT 160
#define VIDEO_BUFFER_SIZE (VIDEO_WIDTH * VIDEO_HEIGHT * 2)
#define AUDIO_BUFFER_SIZE ((BPS / 60) << 1)

static int colour_scheme          = COLOUR_SCHEME_NONE;
static uint8_t *video_buffer      = NULL;
static uint8_t *video_buffer_prev = NULL;
static float *video_buffer_acc_r  = NULL;
static float *video_buffer_acc_g  = NULL;
static float *video_buffer_acc_b  = NULL;
static float lcd_persistence      = 0.5f;
static int16_t *audio_buffer      = NULL;
extern unsigned char controls_state;

struct sv_colour_scheme
{
   const char *name;
   int index;
};

struct sv_colour_scheme sv_colour_schemes[] = {
   { "default",                COLOUR_SCHEME_NONE },
   { "supervision",            COLOUR_SCHEME_SUPERVISION },
   { "gb_dmg",                 COLOUR_SCHEME_GB_DMG },
   { "gb_pocket",              COLOUR_SCHEME_GB_POCKET },
   { "gb_light",               COLOUR_SCHEME_GB_LIGHT },
   { "blossom_pink",           COLOUR_SCHEME_BLOSSOM_PINK },
   { "bubbles_blue",           COLOUR_SCHEME_BUBBLES_BLUE },
   { "buttercup_green",        COLOUR_SCHEME_BUTTERCUP_GREEN },
   { "digivice",               COLOUR_SCHEME_DIGIVICE },
   { "game_com",               COLOUR_SCHEME_GAME_COM },
   { "gameking",               COLOUR_SCHEME_GAMEKING },
   { "game_master",            COLOUR_SCHEME_GAME_MASTER },
   { "golden_wild",            COLOUR_SCHEME_GOLDEN_WILD },
   { "greenscale",             COLOUR_SCHEME_GREENSCALE },
   { "hokage_orange",          COLOUR_SCHEME_HOKAGE_ORANGE },
   { "labo_fawn",              COLOUR_SCHEME_LABO_FAWN },
   { "legendary_super_saiyan", COLOUR_SCHEME_LEGENDARY_SUPER_SAIYAN },
   { "microvision",            COLOUR_SCHEME_MICROVISION },
   { "million_live_gold",      COLOUR_SCHEME_MILLION_LIVE_GOLD },
   { "odyssey_gold",           COLOUR_SCHEME_ODYSSEY_GOLD },
   { "shiny_sky_blue",         COLOUR_SCHEME_SHINY_SKY_BLUE },
   { "slime_blue",             COLOUR_SCHEME_SLIME_BLUE },
   { "ti_83",                  COLOUR_SCHEME_TI_83 },
   { "travel_wood",            COLOUR_SCHEME_TRAVEL_WOOD },
   { "virtual_boy",            COLOUR_SCHEME_VIRTUAL_BOY },
   { NULL, 0 },
};

enum frame_blend_method
{
   FRAME_BLEND_NONE = 0,
   FRAME_BLEND_MIX,
   FRAME_BLEND_LCD_GHOST
};

static void (*blend_frames)(void) = NULL;

/***********************/
/* Interframe blending */
/***********************/

static void blend_frames_mix(void)
{
   uint16_t *curr = (uint16_t*)video_buffer;
   uint16_t *prev = (uint16_t*)video_buffer_prev;
   size_t i;

   for (i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++)
   {
      /* Get colours from current + previous frames */
      uint16_t rgb_curr = *curr;
      uint16_t rgb_prev = *prev;

      /* Store colours for next frame */
      *(prev++) = rgb_curr;

      /* Mix colours
       * > "Mixing Packed RGB Pixels Efficiently"
       *   http://blargg.8bitalley.com/info/rgb_mixing.html */
      *(curr++) = (rgb_curr + rgb_prev + ((rgb_curr ^ rgb_prev) & 0x821)) >> 1;
   }
}

static void blend_frames_lcd_ghost(void)
{
   uint16_t *curr = (uint16_t*)video_buffer;
   float *prev_r  = video_buffer_acc_r;
   float *prev_g  = video_buffer_acc_g;
   float *prev_b  = video_buffer_acc_b;
   size_t i;

   for (i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++)
   {
      /* Get colours from current + previous frames */
      uint16_t rgb_curr = *curr;
      float r_prev      = *prev_r;
      float g_prev      = *prev_g;
      float b_prev      = *prev_b;

      /* Unpack current colours and convert to float */
      float r_curr = (float)(rgb_curr >> 11 & 0x1F);
      float g_curr = (float)(rgb_curr >>  6 & 0x1F);
      float b_curr = (float)(rgb_curr       & 0x1F);

      /* Mix colours for current frame */
      float r_mix = (r_curr * (1.0f - lcd_persistence)) + (r_prev * lcd_persistence);
      float g_mix = (g_curr * (1.0f - lcd_persistence)) + (g_prev * lcd_persistence);
      float b_mix = (b_curr * (1.0f - lcd_persistence)) + (b_prev * lcd_persistence);

      /* Output colour is the minimum of the input
       * and decayed values */
      r_mix = (r_mix < r_curr) ? r_mix : r_curr;
      g_mix = (g_mix < g_curr) ? g_mix : g_curr;
      b_mix = (b_mix < b_curr) ? b_mix : b_curr;

      /* Store colours for next frame */
      *(prev_r++) = r_mix;
      *(prev_g++) = g_mix;
      *(prev_b++) = b_mix;

      /* Convert, repack and assign colours for current frame */
      *(curr++) = ((uint16_t)(r_mix + 0.5f) & 0x1F) << 11 |
                  ((uint16_t)(g_mix + 0.5f) & 0x1F) << 6  |
                  ((uint16_t)(b_mix + 0.5f) & 0x1F);
   }
}

static void allocate_video_buffer_acc(void)
{
   size_t buf_size = VIDEO_WIDTH * VIDEO_HEIGHT * sizeof(float);
   size_t i;

   if (!video_buffer_acc_r)
      video_buffer_acc_r = (float*)malloc(buf_size);

   if (!video_buffer_acc_g)
      video_buffer_acc_g = (float*)malloc(buf_size);

   if (!video_buffer_acc_b)
      video_buffer_acc_b = (float*)malloc(buf_size);

   /* Cannot use memset() on arrays of floats... */
   for (i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++)
   {
      video_buffer_acc_r[i] = 0.0f;
      video_buffer_acc_g[i] = 0.0f;
      video_buffer_acc_b[i] = 0.0f;
   }
}

static void init_frame_blending(enum frame_blend_method blend_method)
{
   switch (blend_method)
   {
      case FRAME_BLEND_MIX:
         if (!video_buffer_prev)
            video_buffer_prev = (uint8_t*)malloc(VIDEO_BUFFER_SIZE * sizeof(uint8_t));
         memset(video_buffer_prev, 0, VIDEO_BUFFER_SIZE * sizeof(uint8_t));
         blend_frames = blend_frames_mix;
         break;
      case FRAME_BLEND_LCD_GHOST:
         allocate_video_buffer_acc();
         blend_frames = blend_frames_lcd_ghost;
         break;
      case FRAME_BLEND_NONE:
      default:
         blend_frames = NULL;
         break;
   }
}

/************************************
 * Auxiliary functions
 ************************************/

static int find_colour_scheme(const char* name)
{
   int scheme_index = COLOUR_SCHEME_NONE;
   size_t i;

   for (i = 0; sv_colour_schemes[i].name; i++)
   {
      if (!strcmp(sv_colour_schemes[i].name, name))
      {
         scheme_index = sv_colour_schemes[i].index;
         break;
      }
   }

   return scheme_index;
}

static void check_variables(bool startup)
{
   struct retro_variable var = {0};
   int colour_scheme_last;
   enum frame_blend_method blend_method;

   /* Internal Palette */
   colour_scheme_last = colour_scheme;
   colour_scheme      = COLOUR_SCHEME_NONE;
   var.key            = "potator_palette",
   var.value          = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      colour_scheme = find_colour_scheme(var.value);

   if (startup || (colour_scheme != colour_scheme_last))
      supervision_set_colour_scheme(colour_scheme);

   /* Interframe Blending */
   blend_method = FRAME_BLEND_NONE;
   var.key      = "potator_mix_frames";
   var.value    = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "mix"))
         blend_method = FRAME_BLEND_MIX;
      else if (!strcmp(var.value, "lcd_ghost"))
         blend_method = FRAME_BLEND_LCD_GHOST;
   }

   init_frame_blending(blend_method);

   /* LCD Persistence */
   lcd_persistence = 0.5f;
   var.key         = "potator_lcd_persistence";
   var.value       = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      lcd_persistence = (float)atof(var.value);
}

static void update_input(void)
{
   unsigned joypad_bits = 0;
   size_t i;

   input_poll_cb();

   if (libretro_supports_bitmasks)
      joypad_bits = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
   else
      for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
         joypad_bits |= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;

   controls_state  = 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP)     ? 0x08 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)  ? 0x01 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)   ? 0x02 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)   ? 0x04 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_A)      ? 0x10 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B)      ? 0x20 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 0x40 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_START)  ? 0x80 : 0;
}

/************************************
 * libretro implementation
 ************************************/

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;
   libretro_set_core_options(environ_cb);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name = "Potator";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version = "1.1" GIT_VERSION;
   info->need_fullpath = false;
   info->valid_extensions = "bin|sv";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.fps            = 60;
   info->timing.sample_rate    = BPS;
   info->geometry.base_width   = VIDEO_WIDTH;
   info->geometry.base_height  = VIDEO_HEIGHT;
   info->geometry.max_width    = VIDEO_WIDTH;
   info->geometry.max_height   = VIDEO_HEIGHT;
   info->geometry.aspect_ratio = VIDEO_WIDTH / VIDEO_HEIGHT;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

size_t retro_serialize_size(void) 
{
   return (size_t)sv_saveStateBufSize();
}

bool retro_serialize(void *data, size_t size)
{ 
   return sv_saveStateBuf(data, (uint32)size);
}

bool retro_unserialize(const void *data, size_t size)
{
   return sv_loadStateBuf(data, (uint32)size);
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   uint8 *rom_data             = NULL;
   bool success                = false;

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },

      { 0 },
   };

   if (!info)
      return false;

   /* Set input descriptors */
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   /* Set colour depth */
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[Potator]: RGB565 is not supported.\n");
      return false;
   }

   /* Potator requires a *copy* of the ROM data...
    * > Note: the buffer will be free()'d inside
    *   supervision_load() */
   rom_data = (uint8*)malloc(info->size);

   if (!rom_data)
   {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[Potator]: Failed to allocate ROM buffer!\n");
      return false;
   }

   memcpy(rom_data, (const uint8*)info->data, info->size);

   /* Initialise emulator */
   supervision_init();

   /* Load ROM */
   success = supervision_load(rom_data, info->size);

   /* Apply initial core options */
   if (success)
      check_variables(true);

   return success;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void) 
{
   supervision_done();
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   return 0;
}

void retro_init(void)
{
   struct retro_log_callback log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

#ifdef _3DS
   video_buffer = (uint8_t*)linearMemAlign(VIDEO_BUFFER_SIZE * sizeof(uint8_t), 128);
#else
   video_buffer = (uint8_t*)malloc(VIDEO_BUFFER_SIZE * sizeof(uint8_t));
#endif

   /* Round up actual size to nearest multiple of 128
    * > Since (AUDIO_BUFFER_SIZE / 2) is an odd number,
    *   potator can write past the end of the buffer,
    *   so we need some extra padding... */
   audio_buffer = (int16_t*)malloc(((AUDIO_BUFFER_SIZE + 0x7F) & ~0x7F) * sizeof(int16_t));
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;
   controls_state             = 0;

   if (video_buffer)
   {
#ifdef _3DS
      linearFree(video_buffer);
#else
      free(video_buffer);
#endif
      video_buffer = NULL;
   }

   if (video_buffer_prev)
   {
      free(video_buffer_prev);
      video_buffer_prev = NULL;
   }

   if (video_buffer_acc_r)
   {
      free(video_buffer_acc_r);
      video_buffer_acc_r = NULL;
   }

   if (video_buffer_acc_g)
   {
      free(video_buffer_acc_g);
      video_buffer_acc_g = NULL;
   }

   if (video_buffer_acc_b)
   {
      free(video_buffer_acc_b);
      video_buffer_acc_b = NULL;
   }

   if (audio_buffer)
   {
      free(audio_buffer);
      audio_buffer = NULL;
   }
}

void retro_reset(void)
{
   supervision_reset();
   supervision_set_colour_scheme(colour_scheme);
}

void retro_run(void)
{
   bool options_updated  = false;

   /* Core options */
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
      check_variables(false);

   /* Update input */
   update_input();

   /* Run emulator */
   supervision_exec((uint16*)video_buffer, true);
   sound_decrement();

   /* Output video */
   if (blend_frames)
      blend_frames();
   video_cb(video_buffer, VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH << 1);

   /* Output audio */
   memset(audio_buffer, 0, AUDIO_BUFFER_SIZE * sizeof(int16_t));
   sound_stream_update((uint8*)audio_buffer, AUDIO_BUFFER_SIZE * sizeof(int16_t));
   audio_batch_cb(audio_buffer, AUDIO_BUFFER_SIZE >> 1);
}
