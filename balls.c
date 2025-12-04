#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>

#define SCREEN_WIDTH_PX 800U
#define SCREEN_HEIGHT_PX 800U

#define END(check, str1, str2) \
    if (check) { \
        assert(check); \
        fprintf(stderr, "%s\n%s", str1, str2); \
        exit(1); \
    } \

#define SDL_main main

typedef struct _Game {
    SDL_Renderer *renderer;
    SDL_Window *window;
} Game;

typedef uint8_t (*Update_callback)(Game *game, uint64_t frame, SDL_KeyCode key,
                                   bool keydown);

enum {UPDATE_MAIN};

enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_BLACK, COLOR_SIZE};

void
setColor(SDL_Renderer *renderer, uint8_t color)
{
    const SDL_Color colors[] = {
        [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
        [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
        [COLOR_BLUE] = {.r = 146, .g = 161, .b = 185, .a = 255},
        [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
        [COLOR_GREY] = {.r = 89, .g = 89, .b = 89, .a = 89},
        [COLOR_BLACK] = {.r = 0, .g = 0, .b = 0, .a = 0},
    };

    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g,
                           colors[color].b, colors[color].a);
}

void drawCircle(SDL_Renderer *ren, float radius, SDL_Point center, uint8_t color)
{
    float x = 0;
    float y = 0;
    for(float i = 0; i < 2 * M_PI; i += 0.001f)
    {
        x = cosf(i);
        y = sinf(i);
        SDL_Rect point = {
            (x * radius) + center.x, 
            (y * radius) + center.y,
            1, 
            1
        };
        setColor(ren, color);
        SDL_RenderFillRect(ren, &point);
    }
}

static uint8_t
updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown) {
    SDL_Point center = {.x = 400, .y = 400};
    drawCircle(game->renderer, 100, center, COLOR_RED);
    return UPDATE_MAIN;
}

void
Game_Update(Game *game, const uint8_t fps)
{
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;
    uint8_t update_id = 0;
    Update_callback update;
    float mspd = (1.0f / (float)fps) * 1000.0f;

    while (!quit) {
        uint32_t start = SDL_GetTicks();

        // Place update functions here
        switch (update_id) {
            case UPDATE_MAIN: update = updateMain; break;
            // case UPDATE_LOSE: update = updateLose; break;
        }


        SDL_Event event;
        SDL_KeyCode key = 0;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN: {
                    if (event.key.repeat == 0) {
                      key = event.key.keysym.sym;
                      keydown = true;
                    }

                    break;
                }

                case SDL_KEYUP: keydown = false; break;
                case SDL_QUIT: quit = true; break;
            }
        }

        update_id = update(game, frame, key, keydown);

        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        if (elapsed_time < mspd) {
            elapsed_time = mspd - elapsed_time;
            SDL_Delay(elapsed_time);
        } 

        SDL_RenderPresent(game->renderer);
        frame++;
    }
}

void
Game_Init(Game *game)
{
    memset(game, 0, sizeof(Game));

    END(SDL_Init(SDL_INIT_VIDEO) != 0, "Could not create texture",
        SDL_GetError());

    END(TTF_Init() != 0, "Could not initialize TTF", TTF_GetError());



    game->window = SDL_CreateWindow("balls", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH_PX, 
                     SCREEN_HEIGHT_PX, SDL_WINDOW_SHOWN);

    END(game->window == NULL, "Could not create window", SDL_GetError());

    game->renderer = SDL_CreateRenderer(game->window, 0,
                                        SDL_RENDERER_SOFTWARE);

    END(game->renderer == NULL, "Could not create renderer", SDL_GetError());
}

void
Game_Quit(Game *game)
{
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    TTF_Quit();
    SDL_Quit();
}

int main(void)
{
    Game game;
    Game_Init(&game);
    Game_Update(&game, 60);
    Game_Quit(&game);
    return 0;
}
