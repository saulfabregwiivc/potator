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

#define SV_W 160
#define SV_H 160
#define AUDIO_BUFFER_SIZE ((SV_SAMPLE_RATE / 60) << 1)

static enum SV_COLOR color_scheme  = SV_COLOR_SCHEME_DEFAULT;
static int ghosting_frames         = 0;
static uint16 *video_buffer        = NULL;
static uint8 *audio_samples_buffer = NULL;
static int16_t *audio_out_buffer   = NULL;
uint8 *rom_data                    = NULL;
size_t rom_size                    = 0;

struct sv_color_scheme
{
   const char *name;
   enum SV_COLOR scheme;
};

struct sv_color_scheme sv_color_schemes[] = {
   { "default",                SV_COLOR_SCHEME_DEFAULT },
   { "potator_amber",          SV_COLOR_SCHEME_AMBER },
   { "potator_green",          SV_COLOR_SCHEME_GREEN },
   { "potator_blue",           SV_COLOR_SCHEME_BLUE },
   { "potator_bgb",            SV_COLOR_SCHEME_BGB },
   { "potator_wataroo",        SV_COLOR_SCHEME_WATAROO },
   { "gb_dmg",                 SV_COLOR_SCHEME_GB_DMG },
   { "gb_pocket",              SV_COLOR_SCHEME_GB_POCKET },
   { "gb_light",               SV_COLOR_SCHEME_GB_LIGHT },
   { "blossom_pink",           SV_COLOR_SCHEME_BLOSSOM_PINK },
   { "bubbles_blue",           SV_COLOR_SCHEME_BUBBLES_BLUE },
   { "buttercup_green",        SV_COLOR_SCHEME_BUTTERCUP_GREEN },
   { "digivice",               SV_COLOR_SCHEME_DIGIVICE },
   { "game_com",               SV_COLOR_SCHEME_GAME_COM },
   { "gameking",               SV_COLOR_SCHEME_GAMEKING },
   { "game_master",            SV_COLOR_SCHEME_GAME_MASTER },
   { "golden_wild",            SV_COLOR_SCHEME_GOLDEN_WILD },
   { "greenscale",             SV_COLOR_SCHEME_GREENSCALE },
   { "hokage_orange",          SV_COLOR_SCHEME_HOKAGE_ORANGE },
   { "labo_fawn",              SV_COLOR_SCHEME_LABO_FAWN },
   { "legendary_super_saiyan", SV_COLOR_SCHEME_LEGENDARY_SUPER_SAIYAN },
   { "microvision",            SV_COLOR_SCHEME_MICROVISION },
   { "million_live_gold",      SV_COLOR_SCHEME_MILLION_LIVE_GOLD },
   { "odyssey_gold",           SV_COLOR_SCHEME_ODYSSEY_GOLD },
   { "shiny_sky_blue",         SV_COLOR_SCHEME_SHINY_SKY_BLUE },
   { "slime_blue",             SV_COLOR_SCHEME_SLIME_BLUE },
   { "ti_83",                  SV_COLOR_SCHEME_TI_83 },
   { "travel_wood",            SV_COLOR_SCHEME_TRAVEL_WOOD },
   { "virtual_boy",            SV_COLOR_SCHEME_VIRTUAL_BOY },
   { NULL,                     0 },
};

/************************************
 * Auxiliary functions
 ************************************/

static uint16 map_rgb565(uint8 r, uint8 g, uint8 b)
{
   return ((r >> 3) << 11) | ((g >> 3) << 6) | (b >> 3);
}

static enum SV_COLOR find_color_scheme(const char* name)
{
   enum SV_COLOR scheme = SV_COLOR_SCHEME_DEFAULT;
   size_t i;

   for (i = 0; sv_color_schemes[i].name; i++)
   {
      if (!strcmp(sv_color_schemes[i].name, name))
      {
         scheme = sv_color_schemes[i].scheme;
         break;
      }
   }

   return scheme;
}

static void check_variables(bool startup)
{
   struct retro_variable var = {0};
   enum SV_COLOR color_scheme_last;
   int ghosting_frames_last;

   /* Internal Palette */
   color_scheme_last = color_scheme;
   color_scheme      = SV_COLOR_SCHEME_DEFAULT;
   var.key           = "potator_palette",
   var.value         = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      color_scheme = find_color_scheme(var.value);

   if (startup || (color_scheme != color_scheme_last))
      supervision_set_color_scheme(color_scheme);

   /* LCD Ghosting */
   ghosting_frames_last = ghosting_frames;
   ghosting_frames      = 0;
   var.key              = "potator_lcd_ghosting",
   var.value            = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      ghosting_frames = atoi(var.value);

   if (startup || (ghosting_frames != ghosting_frames_last))
      supervision_set_ghosting(ghosting_frames);
}

static void update_input(void)
{
   uint8 controls_state = 0;
   unsigned joypad_bits = 0;
   size_t i;

   input_poll_cb();

   if (libretro_supports_bitmasks)
      joypad_bits = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
   else
      for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
         joypad_bits |= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;

   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP)     ? 0x08 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)  ? 0x01 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)   ? 0x02 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)   ? 0x04 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_A)      ? 0x20 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B)      ? 0x10 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 0x40 : 0;
   controls_state |= joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_START)  ? 0x80 : 0;

   supervision_set_input(controls_state);
}

static void update_audio(void)
{
   uint8 *samples_ptr = audio_samples_buffer;
   int16_t *out_ptr   = audio_out_buffer;
   size_t i;

   supervision_update_sound(audio_samples_buffer, AUDIO_BUFFER_SIZE);

   /* Convert from U8 (0 - 45) to S16 */
   for (i = 0; i < AUDIO_BUFFER_SIZE; i++)
      *(out_ptr++) = *(samples_ptr++) << (8 + 1);

   audio_batch_cb(audio_out_buffer, AUDIO_BUFFER_SIZE >> 1);
}

static void init_retro_memory_map(void)
{
   bool cheevos_supported                  = true;
   const uint64_t ram_flags                = RETRO_MEMDESC_SYSTEM_RAM;
   const uint64_t rom_flags                = RETRO_MEMDESC_CONST;
   struct retro_memory_map mmaps           = {0};
   struct retro_memory_descriptor descs[4] =
   {
      { ram_flags, memorymap_getLowerRamPointer(), 0, 0x0000, 0, 0, 0x2000,   "RAMLO"  },
      { ram_flags, memorymap_getRegisters(),       0, 0x2000, 0, 0, 0x2000,   "RAMREG" },
      { ram_flags, memorymap_getUpperRamPointer(), 0, 0x4000, 0, 0, 0x2000,   "RAMHI"  },
      { rom_flags, rom_data,                       0, 0x8000, 0, 0, 0x8000,   "ROM"    },
   };

   mmaps.descriptors     = descs;
   mmaps.num_descriptors = sizeof(descs) / sizeof(*descs);

   environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &mmaps);
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &cheevos_supported);
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
   info->library_version = "1.0.5 " GIT_VERSION;
   info->need_fullpath = false;
   info->valid_extensions = "bin|sv";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.fps            = 60;
   info->timing.sample_rate    = SV_SAMPLE_RATE;
   info->geometry.base_width   = SV_W;
   info->geometry.base_height  = SV_H;
   info->geometry.max_width    = SV_W;
   info->geometry.max_height   = SV_H;
   info->geometry.aspect_ratio = SV_W / SV_H;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

size_t retro_serialize_size(void) 
{
   return (size_t)supervision_save_state_buf_size();
}

bool retro_serialize(void *data, size_t size)
{ 
   return supervision_save_state_buf((uint8*)data, (uint32)size);
}

bool retro_unserialize(const void *data, size_t size)
{
   return supervision_load_state_buf((uint8*)data, (uint32)size);
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

   /* Potator requires a *copy* of the ROM data */
   rom_size = info->size;
   rom_data = (uint8*)malloc(rom_size);

   if (!rom_data)
   {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[Potator]: Failed to allocate ROM buffer!\n");
      return false;
   }

   memcpy(rom_data, (const uint8*)info->data, rom_size);

   /* Initialise emulator */
   supervision_init();

   /* Load ROM */
   success = supervision_load(rom_data, rom_size);

   if (success)
   {
      /* Set palette mapping function */
      supervision_set_map_func(map_rgb565);

      /* Apply initial core options */
      check_variables(true);

      /* Initialise frontend memory map */
      init_retro_memory_map();
   }

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

   if (rom_data)
   {
      free(rom_data);
      rom_data = NULL;
   }

   rom_size = 0;
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
   video_buffer = (uint16*)linearMemAlign(SV_W * SV_H * sizeof(uint16), 128);
#else
   video_buffer = (uint16*)malloc(SV_W * SV_H * sizeof(uint16));
#endif
   memset(video_buffer, 0, SV_W * SV_H * sizeof(uint16));

   /* Round up sample buffer size to nearest multiple
    * of 128, to avoid potential overflows */
   audio_samples_buffer = (uint8*)malloc(((AUDIO_BUFFER_SIZE + 0x7F) & ~0x7F) * sizeof(uint8));
   audio_out_buffer     = (int16_t*)malloc(AUDIO_BUFFER_SIZE * sizeof(int16_t));
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;

   if (video_buffer)
   {
#ifdef _3DS
      linearFree(video_buffer);
#else
      free(video_buffer);
#endif
      video_buffer = NULL;
   }

   if (audio_samples_buffer)
   {
      free(audio_samples_buffer);
      audio_samples_buffer = NULL;
   }

   if (audio_out_buffer)
   {
      free(audio_out_buffer);
      audio_out_buffer = NULL;
   }
}

void retro_reset(void)
{
   supervision_reset();
   supervision_set_map_func(map_rgb565);
   supervision_set_color_scheme(color_scheme);
   supervision_set_ghosting(ghosting_frames);
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
   supervision_exec(video_buffer);

   /* Output video */
   video_cb(video_buffer, SV_W, SV_H, SV_W << 1);

   /* Output audio */
   update_audio();
}
