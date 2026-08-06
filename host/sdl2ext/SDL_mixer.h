/*
 * SDL_mixer.h — the slice of the SDL_mixer API that fheroes2 calls.
 *
 * SDL_mixer is a SEPARATE LIBRARY from SDL2. It mixes several sound channels
 * together, decodes compressed music files and synthesises MIDI, and it sits
 * on top of SDL's own audio API rather than inside it. circle-libsdl2
 * implements SDL2, not SDL_mixer, so on this board the library does not
 * exist at all.
 *
 * fheroes2 includes <SDL_mixer.h> unconditionally and has no build switch
 * for going without it, so the header has to be here for the game to
 * compile. This file is that header: the declarations fheroes2 uses, written
 * to match SDL_mixer's own signatures so upstream compiles exactly as
 * written, and nothing more. It is not SDL_mixer's header and it is not a
 * copy of one.
 *
 * What is behind it is sdl_mixer_absent.cpp in this directory's parent,
 * which answers every call with "no audio device". The game reads that at
 * startup, reports it, and runs in silence — see the README.
 *
 * When circle-libsdl2 grows a mixer, this pair of files is what gets
 * deleted.
 */
#ifndef _rapi_sdl_mixer_h
#define _rapi_sdl_mixer_h

#include <SDL_audio.h>
#include <SDL_error.h>
#include <SDL_rwops.h>
#include <SDL_stdinc.h>
#include <SDL_version.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The version this API surface is written against. fheroes2 tests it with
   SDL_MIXER_VERSION_ATLEAST to decide whether Mix_SetTimidityCfg exists,
   which it does below. */
#define SDL_MIXER_MAJOR_VERSION 2
#define SDL_MIXER_MINOR_VERSION 6
#define SDL_MIXER_PATCHLEVEL    3

#define SDL_MIXER_VERSION_ATLEAST(X, Y, Z)                              \
    ((SDL_MIXER_MAJOR_VERSION >= (X))                                   \
     && (SDL_MIXER_MAJOR_VERSION > (X) || SDL_MIXER_MINOR_VERSION >= (Y)) \
     && (SDL_MIXER_MAJOR_VERSION > (X) || SDL_MIXER_MINOR_VERSION > (Y)  \
         || SDL_MIXER_PATCHLEVEL >= (Z)))

/* Decoders a caller may ask Mix_Init for. */
typedef enum
{
    MIX_INIT_FLAC     = 0x00000001,
    MIX_INIT_MOD      = 0x00000002,
    MIX_INIT_MP3      = 0x00000008,
    MIX_INIT_OGG      = 0x00000010,
    MIX_INIT_MID      = 0x00000020,
    MIX_INIT_OPUS     = 0x00000040
} MIX_InitFlags;

/* Channels allocated by default, and the loudest a volume may be. */
#define MIX_CHANNELS    8
#define MIX_MAX_VOLUME  SDL_MIX_MAXVOLUME

/* A decoded sound effect. The fields are SDL_mixer's; fheroes2 only ever
   holds the pointer. */
typedef struct Mix_Chunk
{
    int    allocated;
    Uint8 *abuf;
    Uint32 alen;
    Uint8  volume;
} Mix_Chunk;

/* A music track. Opaque in SDL_mixer, and opaque here. */
typedef struct _Mix_Music Mix_Music;

/* SDL_mixer reports its errors through SDL's own error string. */
#define Mix_GetError SDL_GetError
#define Mix_SetError SDL_SetError

extern int  Mix_Init(int flags);
extern void Mix_Quit(void);

extern int  Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
extern int  Mix_QuerySpec(int *frequency, Uint16 *format, int *channels);
extern void Mix_CloseAudio(void);

extern int  Mix_AllocateChannels(int numchans);

extern Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc);
extern void       Mix_FreeChunk(Mix_Chunk *chunk);

extern Mix_Music *Mix_LoadMUS(const char *file);
extern Mix_Music *Mix_LoadMUS_RW(SDL_RWops *src, int freesrc);
extern void       Mix_FreeMusic(Mix_Music *music);

extern int  Mix_SetSoundFonts(const char *paths);
extern int  Mix_SetTimidityCfg(const char *path);

extern int  Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);
#define Mix_PlayChannel(channel, chunk, loops) \
    Mix_PlayChannelTimed(channel, chunk, loops, -1)

extern int  Mix_PlayMusic(Mix_Music *music, int loops);
extern int  Mix_FadeInMusic(Mix_Music *music, int loops, int ms);
extern int  Mix_FadeInMusicPos(Mix_Music *music, int loops, int ms, double position);

extern int  Mix_Volume(int channel, int volume);
extern int  Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
extern int  Mix_VolumeMusic(int volume);

extern void Mix_Pause(int channel);
extern void Mix_Resume(int channel);
extern int  Mix_HaltChannel(int channel);
extern int  Mix_HaltMusic(void);
extern int  Mix_Playing(int channel);
extern int  Mix_PlayingMusic(void);

extern int  Mix_SetPosition(int channel, Sint16 angle, Uint8 distance);

extern void Mix_ChannelFinished(void (*channel_finished)(int channel));
extern void Mix_HookMusicFinished(void (*music_finished)(void));

#ifdef __cplusplus
}
#endif

#endif
