//
// sdl_mixer_absent.cpp — what stands where SDL_mixer would be.
//
// SDL_mixer is a separate library from SDL2, and circle-libsdl2 does not
// provide it: the shim implements SDL's own audio API — opening a device and
// feeding it a callback — and nothing above that. SDL_mixer is the layer
// above: several sound effects mixed together, compressed music decoded, and
// MIDI synthesised. All of Heroes II's music is MIDI.
//
// fheroes2 calls SDL_mixer directly and has no build switch for going
// without it, so these functions have to exist for the program to link. What
// they must NOT do is pretend.
//
// So THE DEVICE NEVER OPENS. Mix_OpenAudio fails with a reason, and
// fheroes2's own Audio::Init treats that as fatal to the audio subsystem
// alone: it returns early, leaves the subsystem uninitialised, and every
// later call to play a sound or a track finds it that way and does nothing.
// The game runs, silent, and says on the serial console why. Nothing here is
// ever reached with a real chunk or a real track, because none can be
// created without a device.
//
// The alternative — reporting success and swallowing every sound — would
// leave the game believing it had music. That is the failure this file
// exists to avoid.
//
// This is a seam, not furniture. When circle-libsdl2 grows a mixer, this
// file and sdl2ext/SDL_mixer.h are what get deleted.
//
#include <SDL_mixer.h>

extern "C" {

// The reason, given once per call that asks for one, in SDL's error string —
// which is where fheroes2 reads it from and what it prints.
static const char AbsentMessage[] =
    "SDL_mixer is not available on this board: circle-libsdl2 implements "
    "SDL2's audio API but no mixer above it";

// Decoders. None are present, so none are reported. fheroes2 asks for FLAC,
// MP3, OGG and MIDI, compares what it got, and logs each one it did not get.
int Mix_Init(int)
{
    return 0;
}

void Mix_Quit(void)
{
}

// The one call that decides everything below it.
int Mix_OpenAudio(int, Uint16, int, int)
{
    SDL_SetError("%s", AbsentMessage);
    return -1;
}

// Zero devices open. fheroes2 only reaches this after a successful
// Mix_OpenAudio, so it is here for completeness rather than for a caller.
int Mix_QuerySpec(int *frequency, Uint16 *format, int *channels)
{
    if (frequency != nullptr)
        *frequency = 0;
    if (format != nullptr)
        *format = 0;
    if (channels != nullptr)
        *channels = 0;
    SDL_SetError("%s", AbsentMessage);
    return 0;
}

void Mix_CloseAudio(void)
{
}

// No device, so no channels. A caller asking how many exist is told none,
// and a caller asking for some is told it got none.
int Mix_AllocateChannels(int)
{
    return 0;
}

Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc)
{
    // The contract is the caller's stream is closed when freesrc is set,
    // whether or not the load succeeds. Honour it: the caller has already
    // let go of the stream by the time this returns.
    if (src != nullptr && freesrc != 0)
        SDL_RWclose(src);
    SDL_SetError("%s", AbsentMessage);
    return nullptr;
}

void Mix_FreeChunk(Mix_Chunk *)
{
}

Mix_Music *Mix_LoadMUS(const char *)
{
    SDL_SetError("%s", AbsentMessage);
    return nullptr;
}

Mix_Music *Mix_LoadMUS_RW(SDL_RWops *src, int freesrc)
{
    if (src != nullptr && freesrc != 0)
        SDL_RWclose(src);
    SDL_SetError("%s", AbsentMessage);
    return nullptr;
}

void Mix_FreeMusic(Mix_Music *)
{
}

// MIDI rendering settings. There is no synthesiser to configure, so both
// report failure — which is what stops the game believing a soundfont it
// found on the card is in use.
int Mix_SetSoundFonts(const char *)
{
    SDL_SetError("%s", AbsentMessage);
    return 0;
}

int Mix_SetTimidityCfg(const char *)
{
    SDL_SetError("%s", AbsentMessage);
    return 0;
}

// Playback. Every one of these needs a chunk or a track that cannot exist,
// so each reports the failure its own API uses: -1 for "no channel", 0 for
// "nothing is playing".
int Mix_PlayChannelTimed(int, Mix_Chunk *, int, int)
{
    SDL_SetError("%s", AbsentMessage);
    return -1;
}

int Mix_PlayMusic(Mix_Music *, int)
{
    SDL_SetError("%s", AbsentMessage);
    return -1;
}

int Mix_FadeInMusic(Mix_Music *, int, int)
{
    SDL_SetError("%s", AbsentMessage);
    return -1;
}

int Mix_FadeInMusicPos(Mix_Music *, int, int, double)
{
    SDL_SetError("%s", AbsentMessage);
    return -1;
}

// Volumes. SDL_mixer answers with the volume that was in force before the
// call; nothing here has one, so it is zero either way.
int Mix_Volume(int, int)
{
    return 0;
}

int Mix_VolumeChunk(Mix_Chunk *, int)
{
    return 0;
}

int Mix_VolumeMusic(int)
{
    return 0;
}

void Mix_Pause(int)
{
}

void Mix_Resume(int)
{
}

int Mix_HaltChannel(int)
{
    return 0;
}

int Mix_HaltMusic(void)
{
    return 0;
}

int Mix_Playing(int)
{
    return 0;
}

int Mix_PlayingMusic(void)
{
    return 0;
}

// Stereo placement of a sound effect on the adventure map. No channel, so
// nothing to place: SDL_mixer reports a failure to place as zero.
int Mix_SetPosition(int, Sint16, Uint8)
{
    return 0;
}

// The two completion callbacks. SDL_mixer calls them from its own mixing
// thread when a sound or a track ends; nothing here ever plays one, so
// nothing here ever calls them. They are accepted and dropped rather than
// stored, because a stored callback that is never called is harder to read
// than one that was plainly never kept.
void Mix_ChannelFinished(void (*)(int))
{
}

void Mix_HookMusicFinished(void (*)(void))
{
}

} // extern "C"
