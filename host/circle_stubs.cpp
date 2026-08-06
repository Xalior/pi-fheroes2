//
// circle_stubs.cpp — the SDL2 surface layer fheroes2 needs and
// circle-libsdl2 does not implement.
//
// circle-libsdl2 renders from textures alone. Its SDL_Surface is a 32-bit
// staging buffer in XRGB8888 and nothing in the library blits, converts,
// locks or fills one, because the library's own path never needs to.
//
// fheroes2 stays inside that shape more closely than it looks. It keeps its
// own 8-bit paletted picture in its own memory, converts it into a 32-bit
// surface once per frame, and uploads that with SDL_UpdateTexture — so the
// only surface the library has to provide is the 32-bit one it already
// provides, and this file adds no conversion path at all. That is decided
// for it: the game asks the renderer whether any driver offers 8-bit
// textures, the library answers with ARGB8888 alone, and the game takes its
// 32-bit path.
//
// What is left is a short list of calls around the edges — reading back the
// window, naming a pixel colour, settings on a renderer that has one
// possible answer here. Each one either does the job properly or fails
// honestly, so nothing pretends to work. Where a function is a deliberate
// no-op it says why: on a bare-metal board with one fullscreen display and
// no window manager, there is nothing for it to do.
//
// These are seams, not permanent furniture. When the shim implements one of
// these for real, the way to adopt it is to DELETE the stub here: the
// archive is linked whole, so a leftover stub becomes a duplicate-symbol
// error at link time rather than a silent winner over the real thing.
//
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <SDL2/SDL.h>

extern "C" {

// ---------------------------------------------------------------------------
// Pixel formats and colours
// ---------------------------------------------------------------------------

// The shim's surfaces all carry one shared XRGB8888 format record, and these
// two are how a caller turns colour components into a pixel of it. fheroes2
// calls them per pixel while converting its paletted picture into a surface,
// so they are written as the shifts they are rather than as a lookup.
Uint32 SDL_MapRGB(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b)
{
    if (format == nullptr)
        return 0;

    // A paletted format has no channel shifts to map into. Nothing in this
    // port creates one, so this is the honest answer rather than a search
    // through a palette that does not exist.
    if (format->BitsPerPixel < 8 || format->palette != nullptr)
        return 0;

    return ((Uint32)r << format->Rshift) | ((Uint32)g << format->Gshift)
           | ((Uint32)b << format->Bshift) | format->Amask;
}

Uint32 SDL_MapRGBA(const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b,
                   Uint8 a)
{
    if (format == nullptr)
        return 0;
    if (format->BitsPerPixel < 8 || format->palette != nullptr)
        return 0;

    // No alpha channel in the format means the alpha argument has nowhere to
    // go. The colour is still correct; only the transparency is lost, and it
    // is lost because the surface cannot carry it.
    if (format->Amask == 0)
        return SDL_MapRGB(format, r, g, b);

    return ((Uint32)r << format->Rshift) | ((Uint32)g << format->Gshift)
           | ((Uint32)b << format->Bshift) | ((Uint32)a << format->Ashift);
}

// Building a format record from a format enumeration, for converting a
// loaded image into a known channel order. Only the screenshot loader asks
// for one, and only ever for a format this board has no surface in, so this
// reports honestly that it cannot make one rather than handing back a record
// that describes nothing.
SDL_PixelFormat *SDL_AllocFormat(Uint32 pixel_format)
{
    SDL_SetError("pixel format 0x%08x cannot be allocated: this port has one "
                 "surface format, XRGB8888", (unsigned)pixel_format);
    return nullptr;
}

void SDL_FreeFormat(SDL_PixelFormat *)
{
}

// Converting a surface between formats. There is one surface format here, so
// there is nothing to convert between.
SDL_Surface *SDL_ConvertSurface(SDL_Surface *, const SDL_PixelFormat *, Uint32)
{
    SDL_SetError("surface conversion is not implemented: this port has one "
                 "surface format, XRGB8888");
    return nullptr;
}

// The paletted surfaces this would colour are never created here, but the
// call is reached from the screenshot writer, so it exists and refuses
// rather than being left to the linker to complain about.
int SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *, int, int)
{
    if (palette == nullptr)
    {
        SDL_SetError("SDL_SetPaletteColors: no palette — paletted surfaces "
                     "are not implemented on this board");
        return -1;
    }
    return -1;
}

// Nothing here is RLE encoded or hardware backed, so a lock is a formality.
int SDL_LockSurface(SDL_Surface *) { return 0; }
void SDL_UnlockSurface(SDL_Surface *) {}

// ---------------------------------------------------------------------------
// Reading and writing BMP files
// ---------------------------------------------------------------------------
//
// fheroes2 uses these for its screenshot key and for loading an image from
// the card in the map editor. Both need a paletted surface, which this board
// does not have, so both fail with a reason and the game reports it. The
// game itself is unaffected: neither is on any path it runs through.

SDL_Surface *SDL_LoadBMP_RW(SDL_RWops *src, int freesrc)
{
    if (src != nullptr && freesrc != 0)
        SDL_RWclose(src);
    SDL_SetError("reading a BMP is not implemented on this board");
    return nullptr;
}

int SDL_SaveBMP_RW(SDL_Surface *, SDL_RWops *dst, int freedst)
{
    if (dst != nullptr && freedst != 0)
        SDL_RWclose(dst);
    SDL_SetError("writing a BMP is not implemented on this board");
    return -1;
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

// fheroes2 makes its one screen texture this way and then feeds it with
// SDL_UpdateTexture every frame, so this only has to size and format the
// texture — the pixels the surface holds at this moment are the blank ones
// it was created with.
SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *renderer,
                                          SDL_Surface *surface)
{
    if (renderer == nullptr || surface == nullptr)
    {
        SDL_SetError("SDL_CreateTextureFromSurface: no surface");
        return nullptr;
    }
    if (surface->format == nullptr || surface->format->BytesPerPixel != 4)
    {
        SDL_SetError("SDL_CreateTextureFromSurface: only 32-bit surfaces are "
                     "implemented");
        return nullptr;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             surface->w, surface->h);
    if (texture == nullptr)
        return nullptr;

    // Carry the surface's current contents across, so the texture starts as
    // a copy of the surface exactly as SDL2 promises. The two formats are
    // the same four bytes in the same order, which is why this is a copy of
    // rows and not a conversion.
    if (surface->pixels != nullptr)
    {
        if (SDL_UpdateTexture(texture, nullptr, surface->pixels,
                              surface->pitch) != 0)
        {
            SDL_DestroyTexture(texture);
            return nullptr;
        }
    }
    return texture;
}

// Render-to-texture: the shim's renderer draws to the screen and nowhere
// else. Restoring the default target succeeds because that is where it
// already is; asking for any other target fails, which is what stops a
// caller believing the picture went somewhere it did not.
int SDL_SetRenderTarget(SDL_Renderer *, SDL_Texture *texture)
{
    if (texture == nullptr)
        return 0;
    SDL_SetError("render targets are not implemented");
    return -1;
}

// ---------------------------------------------------------------------------
// Renderer and window settings with one possible answer here
// ---------------------------------------------------------------------------
//
// The display is one fullscreen panel that the host kernel declared before
// the game started, and the window is that panel. Its size, its position and
// its scaling are settled before any of these can be called.

// The window IS the canvas, and the canvas is exactly the size the game
// renders at, so the logical size a caller asks for is the one already in
// force.
int SDL_RenderSetLogicalSize(SDL_Renderer *, int, int) { return 0; }

// Frames are presented on the framebuffer's own page flip, which is a vsync
// by construction — there is no unsynchronised path to switch to.
int SDL_RenderSetVSync(SDL_Renderer *, int) { return 0; }

int SDL_SetWindowFullscreen(SDL_Window *, Uint32) { return 0; }
void SDL_SetWindowSize(SDL_Window *, int, int) {}
void SDL_SetWindowMinimumSize(SDL_Window *, int, int) {}

// No window manager, so no taskbar and no title bar to put an icon on.
void SDL_SetWindowIcon(SDL_Window *, SDL_Surface *) {}

// Grabbing confines the pointer to the window. The window is the whole
// screen and the shim already clamps the pointer to it, so the pointer is
// permanently grabbed and there is nothing to switch.
void SDL_SetWindowGrab(SDL_Window *, SDL_bool) {}

// The window is the display and the display starts at its own origin.
void SDL_GetWindowPosition(SDL_Window *, int *x, int *y)
{
    if (x != nullptr)
        *x = 0;
    if (y != nullptr)
        *y = 0;
}

// The shim keeps no title, because nothing here draws one. fheroes2 reads
// the title back only to carry it across a window rebuild, and sets it again
// straight afterwards, so an empty answer costs nothing that can be seen.
const char *SDL_GetWindowTitle(SDL_Window *)
{
    return "";
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

// Names for the keys the hotkey screens print. SDL builds these from a table
// this port does not carry; the printable keys name themselves and
// everything else is reported honestly as unnamed.
const char *SDL_GetKeyName(SDL_Keycode key)
{
    static char name[2];
    if (key >= ' ' && key < 0x7F)
    {
        name[0] = (char)key;
        name[1] = '\0';
        return name;
    }
    return "";
}

// Circle binds USB keyboards, mice and gamepads. It binds no touch device,
// and the shim reports none, so the game's touch paths are never entered.
int SDL_GetNumTouchDevices(void)
{
    return 0;
}

// ---------------------------------------------------------------------------
// Where the game's own files live
// ---------------------------------------------------------------------------

// On a desktop this is a per-user directory. Here it is the card directory
// the game was started from, which is also where its data files live: the
// configuration, the save games and the high scores all belong beside them.
// The caller frees what it gets, so this hands back a fresh copy every time.
char *SDL_GetPrefPath(const char *, const char *)
{
    static const char path[] = RAPI_GAME_DIR "/";
    char *copy = (char *)SDL_malloc(sizeof(path));
    if (copy != nullptr)
        memcpy(copy, path, sizeof(path));
    return copy;
}

} // extern "C"
