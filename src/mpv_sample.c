// Build with: gcc -o main main.c `pkg-config --libs --cflags mpv sdl2`
// gcc -g -o main mpv_sample.c `pkg-config --libs --cflags mpv sdl2` -lass -framework OpenGL

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>


#include <mpv/client.h>
#include <mpv/opengl_cb.h>

#include <ass/ass.h>
#include "SDL_ttf.h"

ASS_Library *ass_library;
ASS_Renderer *ass_renderer;
ASS_Track *ass_track;

SDL_Texture *danmaku_texture= NULL;
SDL_Renderer *renderer = NULL;

unsigned t1 = 0;

static Uint32 wakeup_on_mpv_redraw, wakeup_on_mpv_events;

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void *get_proc_address_mpv(void *fn_ctx, const char *name)
{
    return SDL_GL_GetProcAddress(name);
}

static void on_mpv_events(void *ctx)
{
    SDL_Event event = {.type = wakeup_on_mpv_events};
    SDL_PushEvent(&event);
}

static void on_mpv_redraw(void *ctx)
{
    SDL_Event event = {.type = wakeup_on_mpv_redraw};
    SDL_PushEvent(&event);
    printf("new frame\n");
}

void msg_callback(int level, const char *fmt, va_list va, void *data)
{
    if (level > 6)
        return;
    printf("libass: ");
    vprintf(fmt, va);
    printf("\n");
}

static void ass_init(int frame_w, int frame_h)
{
    ass_library = ass_library_init();
    if (!ass_library) {
        printf("ass_library_init failed!\n");
        exit(1);
    }

    ass_set_message_cb(ass_library, msg_callback, NULL);

    ass_renderer = ass_renderer_init(ass_library);
    if (!ass_renderer) {
        printf("ass_renderer_init failed!\n");
        exit(1);
    }

    ass_set_frame_size(ass_renderer, frame_w, frame_h);
    char *s = "/Library/Fonts/Arial Unicode.ttf";
    ass_set_fonts(ass_renderer, "/Library/Fonts/Arial Unicode.ttf", "sans-serif",
                  ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
}


static void _ProcessAssImage(SDL_Surface *surface, const ASS_Image *img) {
    int x, y;
    // libass headers claim img->color is RGBA, but the alpha is 0.
    unsigned char r = ((img->color) >> 24) & 0xFF;
    unsigned char g = ((img->color) >> 16) & 0xFF;
    unsigned char b = ((img->color) >>  8) & 0xFF;
    unsigned char *src = img->bitmap;
    unsigned char *dst = (unsigned char*)surface->pixels;
    unsigned char opacity = 255;  // 0 - 255; 255 is transparent

    for(y = 0; y < img->h; y++) {
        for(x = 0; x < img->w; x++) {
            dst[x * 4 + 0] = r;
            dst[x * 4 + 1] = g;
            dst[x * 4 + 2] = b;
            dst[x * 4 + 3] = src[x];
            /* dst[x * 4 + 3] = src[x] ? ( src[x] > opacity ? src[x] - opacity : 0) : src[x];  // set opacity !! */
        }
        src += img->stride;
        dst += surface->pitch;
    }
}

int danmaku_thread(void *arg) {

    // TEST
    SDL_Delay(2500);
    /* int WIDTH = 1280; */
    /* int HEIGHT = 720; */
    int WIDTH = 640;
    int HEIGHT = 360;
    unsigned char * pixels = (unsigned char *)malloc(4 *WIDTH * HEIGHT);
    printf("glReadPixels start\n");
    glReadPixels(0, 0,WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    printf("glReadPixels success\n");
    SDL_Surface* snap = SDL_CreateRGBSurfaceFrom(pixels,WIDTH,HEIGHT,8*4, WIDTH * 4, 0, 0, 0, 0);
    SDL_SaveBMP(snap, "snapshot.bmp");
    SDL_FreeSurface(snap);
    free(pixels);
    return 0;
    // TEST OVER



    mpv_handle *mpv = (mpv_handle *) arg;
Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    int i = 0;
    while(i < 20) {
        SDL_Delay(100);
        i ++;
        continue;
                SDL_RenderClear(renderer);
                ASS_Image *img = ass_render_frame(ass_renderer, ass_track, t1 , NULL);
                SDL_Surface *danmaku_surface = SDL_CreateRGBSurface(0, 1280, 720, 32,
                        rmask, gmask, bmask, amask);
                while (img) {
                    SDL_Surface *tmp = SDL_CreateRGBSurface(
                        0, img->w, img->h, 32,
                        rmask, gmask, bmask, amask);

                    _ProcessAssImage(tmp, img);
                    /* SDL_Rect rect_img; */
                    SDL_Rect *rect_img = malloc(sizeof(SDL_Rect));
                    rect_img->x = img->dst_x;
                    rect_img->y = img->dst_y;
                    rect_img->w = img->w;
                    rect_img->h = img->h;
                    SDL_BlitSurface(tmp, NULL, danmaku_surface, rect_img);
                     /* text_img = SDL_CreateTextureFromSurface(renderer, tmp); */
                    /* SDL_RenderCopy(renderer, text_img, NULL, &rect_img); */
                     /* danmaku_texture = SDL_CreateTextureFromSurface(renderer, tmp); */
                    /* SDL_RenderCopy(renderer, danmaku_texture, NULL, rect_img); */

                    img = img->next;
                }
                danmaku_texture = SDL_CreateTextureFromSurface(renderer, danmaku_surface);
                SDL_RenderCopy(renderer, danmaku_texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                i++;
                SDL_Delay(80);
    }
    const char * cmd[] = {"sub-add", "sakura1.ass", NULL};
    int status = mpv_command(mpv, cmd);
    if (status < 0) {
        printf("API ERROR: %s\n", mpv_error_string(status));
    } else {
        printf("success load subtitle");
    }
    return 1;

}

int main(int argc, char *argv[])
{
    /* if (argc < 2) */
        /* die("pass a single media file as argument"); */

    mpv_handle *mpv = mpv_create();
    if (!mpv)
        die("context init failed");

    // Some minor options can only be set before mpv_initialize().
    if (mpv_initialize(mpv) < 0)
        die("mpv init failed");

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        die("SDL init failed");

  // SDL_ttf init
  TTF_Init();

    SDL_Window *window =
        SDL_CreateWindow("hi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         1000, 500, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                                    SDL_WINDOW_RESIZABLE);
    if (!window)
        die("failed to create SDL window");
    // danmaku
const int frame_w = 1280;
const int frame_h = 720;
    /* print_font_providers(ass_library); */
    /* ass_init(frame_w, frame_h); */
    /* char *danmaku = "sakura1.ass"; */
    /* ass_track = ass_read_file(ass_library, danmaku, NULL); */
    /* renderer = SDL_CreateRenderer(window, -1, 0); */
    /* if (!renderer) { */
        /* fprintf(stderr, "SDL: could not create renderer - exiting\n"); */
        /* exit(1); */
    /* } */

    // The OpenGL API is somewhat separate from the normal mpv API. This only
    // returns NULL if no OpenGL support is compiled.
    mpv_opengl_cb_context *mpv_gl = mpv_get_sub_api(mpv, MPV_SUB_API_OPENGL_CB);
    if (!mpv_gl)
        die("failed to create mpv GL API handle");

    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    if (!glcontext)
        die("failed to create SDL GL context");




    // This makes mpv use the currently set GL context. It will use the callback
    // to resolve GL builtin functions, as well as extensions.
    if (mpv_opengl_cb_init_gl(mpv_gl, NULL, get_proc_address_mpv, NULL) < 0)
        die("failed to initialize mpv GL context");

    // Actually using the opengl_cb state has to be explicitly requested.
    // Otherwise, mpv will create a separate platform window.
    if (mpv_set_option_string(mpv, "vo", "opengl-cb") < 0)
        die("failed to set VO");

    // We use events for thread-safe notification of the SDL main loop.
    // Generally, the wakeup callbacks (set further below) should do as least
    // work as possible, and merely wake up another thread to do actual work.
    // On SDL, waking up the mainloop is the ideal course of action. SDL's
    // SDL_PushEvent() is thread-safe, so we use that.
    wakeup_on_mpv_redraw = SDL_RegisterEvents(1);
    wakeup_on_mpv_events = SDL_RegisterEvents(1);
    if (wakeup_on_mpv_redraw == (Uint32)-1 || wakeup_on_mpv_events == (Uint32)-1)
        die("could not register events");

    // When normal mpv events are available.
    mpv_set_wakeup_callback(mpv, on_mpv_events, NULL);

    // When a new frame should be drawn with mpv_opengl_cb_draw().
    // (Separate from the normal event handling mechanism for the sake of
    //  users which run OpenGL on a different thread.)
    mpv_opengl_cb_set_update_callback(mpv_gl, on_mpv_redraw, NULL);

    // Play this file. Note that this starts playback asynchronously.
    /* const char *cmd[] = {"loadfile", argv[1], NULL}; */
    const char *cmd[] = {"loadfile", "a.mp4", NULL};
    mpv_command(mpv, cmd);

    /* SDL_CreateThread(danmaku_thread, "danmaku_thread", mpv); */
    /* const char *cmd1[] = {"sub-add", argv[2], NULL}; */
    /* int status = mpv_command(mpv, cmd1); */
    /* if (status < 0) { */
        /* printf("API ERROR: %s\n", mpv_error_string(status)); */
    /* } else { */
        /* printf("success load subtitle"); */
    /* } */
    /* const char *cmd1[] = {"sub-add", "sakura1", NULL}; */
    /* mpv_command(mpv, cmd1); */

    unsigned int t0 = 0, ttmp = 0;
    t0 = SDL_GetTicks();

    TTF_Font *font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 40);
    if (font == NULL){
        fprintf(stderr, "TTF_OpenFont error\n");
            /* return; */
    }
               SDL_Color color = { 255, 255, 255, 255 };
               SDL_Color colorb = { 0, 0, 0, 255 };




    while (1) {
        ttmp = SDL_GetTicks();
        t1 +=  ttmp - t0;
        t0 = ttmp;
        SDL_Event event;
        if (SDL_WaitEvent(&event) != 1)
            die("event loop error");
        int redraw = 0;
        switch (event.type) {
        case SDL_QUIT:
            goto done;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
                redraw = 1;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_SPACE)
                mpv_command_string(mpv, "cycle pause");
            break;
        default:
            // Happens when a new video frame should be rendered, or if the
            // current frame has to be redrawn e.g. due to OSD changes.
            if (event.type == wakeup_on_mpv_redraw)
                redraw = 1;
            // Happens when at least 1 new event is in the mpv event queue.
            if (event.type == wakeup_on_mpv_events) {
                // Handle all remaining mpv events.
                while (1) {
                    mpv_event *mp_event = mpv_wait_event(mpv, 0);
                    if (mp_event->event_id == MPV_EVENT_NONE)
                        break;
                    printf("event: %s\n", mpv_event_name(mp_event->event_id));
                }
            }
        }
        if (redraw) {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            printf("w: %d\n", w);
            printf("h: %d\n", h);
            // Note:
            // - The 0 is the FBO to use; 0 is the default framebuffer (i.e.
            //   render to the window directly.
            // - The negative height tells mpv to flip the coordinate system.
            // - If you do not want the video to cover the whole screen, or want
            //   to apply any form of fancy transformation, you will have to
            //   render to a FBO.
            // - See opengl_cb.h on what OpenGL environment mpv expects, and
            //   other API details.


            mpv_opengl_cb_draw(mpv_gl, 0, w, -h);

Uint32 rmask, gmask, bmask, amask;
            #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            rmask = 0xff000000;
            gmask = 0x00ff0000;
            bmask = 0x0000ff00;
            amask = 0x000000ff;
#else
            rmask = 0x000000ff;
            gmask = 0x0000ff00;
            bmask = 0x00ff0000;
            amask = 0xff000000;
#endif


            int WIDTH = w;
            int HEIGHT = abs(-h);

            unsigned char * pixels = (unsigned char *)malloc(4 *WIDTH * HEIGHT);
            unsigned char * normal_pixels = (unsigned char *)malloc(4 * WIDTH * HEIGHT);
            unsigned char * flip_pixels = (unsigned char *)malloc(4 * WIDTH * HEIGHT);
            printf("glReadPixels start\n");

            glReadPixels(0, 0,WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            for (int y = 0; y < HEIGHT; y++){
                for (int x = 0; x < WIDTH * 4; x++) {
                    normal_pixels[x + ((HEIGHT - y - 1) * WIDTH * 4)] = pixels[x + (y * WIDTH * 4)];
                }
            }

            printf("glReadPixels success\n");


            /* SDL_Surface* snap = SDL_CreateRGBSurfaceFrom(pixels,WIDTH,HEIGHT,8*4, WIDTH * 4, rmask, gmask, bmask, amask); */
            SDL_Surface* snap = SDL_CreateRGBSurfaceFrom(normal_pixels,WIDTH,HEIGHT,8*4, WIDTH * 4, rmask, gmask, bmask, amask);
            /* SDL_Surface* snap = SDL_CreateRGBSurfaceFrom(flip_pixels,WIDTH,HEIGHT,8*4, WIDTH * 4, rmask, gmask, bmask, amask); */


             // TTF_RenderUTF8_Blended  START
            TTF_SetFontOutline(font, 1);
            SDL_Surface *text_sb = TTF_RenderUTF8_Blended(font, "中文with english", colorb);
            TTF_SetFontOutline(font, 0);
            SDL_Surface *text_s = TTF_RenderUTF8_Blended(font, "中文with english", color);
            SDL_Rect rectb = { 1, 1, 0, 0 };
            SDL_BlitSurface(text_s, NULL, text_sb, &rectb);
            SDL_Texture *text_t= SDL_CreateTextureFromSurface(renderer, text_sb);
            SDL_Rect rect;
            SDL_QueryTexture(text_t, NULL, NULL, &rect.w, &rect.h);
            rect.x = 150;
            rect.y = 150;
            SDL_DestroyTexture(text_t);
            SDL_BlitSurface(text_sb, NULL, snap, &rect);
            SDL_FreeSurface(text_sb);
            SDL_FreeSurface(text_s);
             // TTF_RenderUTF8_Blended  END



            SDL_SaveBMP(snap, "snapshot.bmp");

            for (int y = 0; y < HEIGHT; y++){
                for (int x = 0; x < WIDTH * 4; x++) {
                    flip_pixels[x + (y * WIDTH * 4)] = ((unsigned char*)snap->pixels)[x + ((HEIGHT - y - 1) * WIDTH * 4)];
                }
            }


            glEnable(GL_TEXTURE_2D);
            GLuint TextureID = 0;

            glGenTextures(1, &TextureID);
            glBindTexture(GL_TEXTURE_2D, TextureID);

            glDrawPixels(snap->w, snap->h, GL_RGBA, GL_UNSIGNED_BYTE, flip_pixels);

            glFlush();

            /* glDisable(GL_BLEND); */
            glDisable(GL_TEXTURE_2D);

            SDL_FreeSurface(snap);
            free(pixels);
            free(normal_pixels);
            free(flip_pixels);

            SDL_GL_SwapWindow(window);

        }
    }
done:

    // Destroy the GL renderer and all of the GL objects it allocated. If video
    // is still running, the video track will be deselected.
    mpv_opengl_cb_uninit_gl(mpv_gl);

    mpv_terminate_destroy(mpv);


               TTF_CloseFont(font);
    /* ass_free_track(ass_track); */
    /* ass_renderer_done(ass_renderer); */
    /* ass_library_done(ass_library); */
    return 0;
}
