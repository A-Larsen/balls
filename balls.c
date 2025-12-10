#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>

// NOTE:
// This is a less accurate depiction of gravity. I am using a different number
// as the mesurement in meters per second because you cannot have fractions of a
// pixel. In order to depict this more accurately I would need an approximate
// size of a pixel, and not every pixel is the same size. I have decided not to
// be THAT accurate.
#define METER_AS_PIXELS 3779U
#define BALL_COUNT 10
// #define GRAVITY 0.5f

#define END(check, str1, str2) \
    if (check) { \
        assert(check); \
        fprintf(stderr, "%s\n%s", str1, str2); \
        exit(1); \
    } \

#define SDL_main main

typedef struct _Ball {
    float px, py, vx, vy, ax, ay;
    float radius;
    uint8_t color;
    uint16_t mass;
} Ball;

typedef struct _Mouse {
    SDL_Point p;
    bool down;
    uint8_t button;
} Mouse;

typedef struct _Game {
    SDL_Renderer *renderer;
    SDL_Window *window;
    const SDL_Rect screen_rect;
    Ball balls[BALL_COUNT];

    bool out_of_bounds;
    uint32_t fps;
    float terminal_velocity;
    uint8_t ball_size_min;
    uint8_t ball_size_max;
    const uint16_t ball_distinct_unordered_pairs;
} Game;


typedef uint8_t (*Update_callback) (Game *game, 
                                    float seconds, 
                                    uint32_t milliseconds,
                                    SDL_KeyCode key,
                                    Mouse mouse,
                                    bool keydown);

enum {UPDATE_MAIN, UPDATE_NOTHING};

// Make sure last color is always black
enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_PURPLE, COLOR_NEON_GREEN, COLOR_PINK, COLOR_YELLOW, COLOR_BLACK,
      COLOR_SIZE};

void
setColor(SDL_Renderer *renderer,
         uint8_t color)
{
    const SDL_Color colors[] = {
        [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
        [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
        [COLOR_BLUE] = {.r = 39, .g = 211, .b = 245, .a = 255},
        [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
        [COLOR_GREY] = {.r = 89, .g = 89, .b = 89, .a = 89},
        [COLOR_YELLOW] = {.r = 230, .g = 230, .b = 0, .a = 255},
        [COLOR_PINK] = {.r = 245, .g = 39, .b = 108, .a = 255},
        [COLOR_NEON_GREEN] = {.r = 39, .g = 245, .b = 176, .a = 255},
        [COLOR_PURPLE] = {.r = 176, .g = 39, .b = 245, .a = 255},
        [COLOR_BLACK] = {.r = 0, .g = 0, .b = 0, .a = 0},
    };

    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g,
                           colors[color].b, colors[color].a);
}

float getHyp(float x1, float y1, float x2, float y2) {
    return sqrtf((float)(x1 - x2) * (float)(x1 - x2) +
           (float)(y1 - y2) * (float)(y1 - y2));
}

bool ballCollide(Ball b1,
                 Ball b2)
{
    return fabs((b1.px - b2.px) * (b1.px - b2.px) +
           (b1.py - b2.py) * (b1.py - b2.py)) <=
           (float)(b1.radius + b2.radius) * (float)(b1.radius + b2.radius);
}

bool pointInBall(Ball b,
                 SDL_Point p)
{
    return fabs((b.px - p.x) * (b.px - p.x) + (b.py - p.y) * (b.py - p.y))
           <= (b.radius * b.radius);
}

void
drawBall(SDL_Renderer *renderer, Ball ball)
// (x - h)^2 + (y - k)^2 = r^2
// Drawing a circle with the standard circle formula.
// There may be a more performant way to do this...
{
    setColor(renderer, ball.color);

    // diameter is radius * 2
    for (int x = 0; x < ball.radius * 2; x++) {
        for (int y = 0; y < ball.radius * 2; y++) {
          if ( ((x - ball.radius) * (x - ball.radius)) +
               ((y - ball.radius) * (y - ball.radius)) <=
               ball.radius * ball.radius) {
            int px = ball.px - ball.radius + x;
            int py = ball.py - ball.radius + y;

            // TODO: wrap circle around screen
            SDL_RenderDrawPoint(renderer, px, py);

          }
        }
    }
}

float clamp(float v,
            float min,
            float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

bool circleRectCollide(SDL_Point circle,
                       float r,
                       SDL_Rect rect)
{
    float closestX = clamp(circle.x, rect.x, rect.x + rect.w);
    float closestY = clamp(circle.y, rect.y, rect.y + rect.h);

    // this gets the distance form the center of the circle to a side of a
    // rectangle. It could be negative or positive. Because we are squaring them
    // it doesn't matter.
    float dx = circle.x - closestX;
    float dy = circle.y - closestY;

    // use the dot product to determine collision
    return (dx*dx + dy*dy) <= (r*r);
}


static uint8_t
updateNothing(Game *game,
           float seconds,
           uint32_t milliseconds,
           SDL_KeyCode key,
           Mouse mouse,
           bool keydown)
{
    return UPDATE_NOTHING;
}

void
worldBoundry(Game *game, Ball *ball)
{
    int padding = 1;

    if (ball->px - ball->radius <= 0) {
        ball->px = ball->radius + padding;
    }

    if (ball->py - ball->radius <= 0) {
        ball->py = ball->radius + padding;
    }

    if (ball->px + ball->radius >= game->screen_rect.w) {
        ball->px = game->screen_rect.w - ball->radius - padding;
    }

    if (ball->py + ball->radius >= game->screen_rect.h) {
        ball->py = game->screen_rect.h - ball->radius - padding;
    }
}

static uint8_t
updateMain(Game *game,
           float seconds,
           uint32_t milliseconds,
           SDL_KeyCode key,
           Mouse mouse,
           bool keydown)
{
    static float time = 0;
    static int selected = -1;
    // draw balls and wrap them around the screen

    // game->ball_distinct_unordered_pairs is the total number of possible
    // collisions, it is multiplied was two because there needed to be two values
    // for each collision, a ball at the target ball
    Ball *colliding[game->ball_distinct_unordered_pairs];
    uint8_t collision_count = 0;

    // NOTE:
    // issues arise when mouse movement is too fast
    // don't check mouse click more than needed
    if(mouse.button == SDL_BUTTON_LEFT && (selected < 0)) {
        for (int i = 0; i < BALL_COUNT; ++i) {
            Ball *b = &game->balls[i];
            if (pointInBall(*b, mouse.p)) {
                selected = i; // toggle selected
            }
        }
    }
    if (!mouse.down) selected = -1;

    if(selected >= 0) {
        Ball *b = &game->balls[selected];
        b->px = mouse.p.x;
        b->py = mouse.p.y;
        SDL_ShowCursor(false);
    } else SDL_ShowCursor(true);


    // TODO
    // set up a list of values that will not conflict for i and j in gameInit
    for (int i = 0; i < BALL_COUNT; ++i) {
        Ball *b1 = &game->balls[i];

        for (int j = i + 1; j < BALL_COUNT; ++j) {
            Ball *b2 = &game->balls[j];

            if (!ballCollide(*b1, *b2)) continue;

            colliding[collision_count++] = b1;
            colliding[collision_count++] = b2;
            float distance = getHyp(b1->px, b1->py, b2->px, b2->py);

            float overlap = 0.5f * (distance - (float)b1->radius -
                            (float)b2->radius);
            b1->px -= 
                overlap * (float)(b1->px - b2->px) / distance;
            b1->py -= 
                overlap * (float)(b1->py - b2->py) / distance;
            b2->px += 
                overlap * ((float)b1->px - b2->px) / distance;
            b2->py += 
                overlap * (float)(b1->py - b2->py) / distance;
        }

    }

    for (int i = 0; i < collision_count; ++i) {

    }

    // TODO only draw balls that are colliding
    // everying else should be update with backbuffer
    for (int i = 0; i < BALL_COUNT; ++i) {
        Ball *b = &game->balls[i];
        drawBall(game->renderer, *b);
    }

    time++;

    return UPDATE_MAIN;
}

void
Game_Update(Game *game)
// The main game loop. Sets up which callback will be used in the function loop.
// Each update callback determines what update callback will be called next by
// returning the appropriate enum value
{
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;
    uint8_t update_id = 0;
    Update_callback update;
    float mspf = (1.0f / (float)game->fps) * 1000.0f;
    SDL_Event event;
    SDL_KeyCode key = 0;
    uint8_t color = COLOR_BLACK;
    uint32_t ticks_start = SDL_GetTicks();

    // TODO
    // add backbuffer

    while (!quit) {
        // clear screen
        if (game->out_of_bounds) color = COLOR_GREEN;
        setColor(game->renderer, color);
        SDL_RenderClear(game->renderer);

        // Place update functions here
        switch (update_id) {
            case UPDATE_MAIN: update = updateMain; break;
            case UPDATE_NOTHING: update = updateNothing; break;
        }


        Mouse mouse;
        while (SDL_PollEvent(&event)) {

            switch (event.type) {
                case SDL_KEYDOWN: {
                    if (event.key.repeat == 0) {
                      key = event.key.keysym.sym;
                      keydown = true;
                    }

                    break;
                }
                case SDL_MOUSEBUTTONDOWN: {
                    mouse.button = event.button.button;
                    mouse.down = true;
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    mouse.button = event.button.button;
                    mouse.down = false;
                    break;
                }
                case SDL_MOUSEMOTION: {
                    mouse.p.x = event.motion.x;
                    mouse.p.y = event.motion.y;
                    break;
                }

                case SDL_KEYUP: keydown = false; break;
                case SDL_QUIT: quit = true; break;
            }
        }

        update_id = update(game, SDL_GetTicks(), frame, key, mouse,keydown);

        if ((SDL_GetTicks() - ticks_start) >= mspf) {
            ticks_start = SDL_GetTicks();
            SDL_RenderPresent(game->renderer);
        }

        frame++;
    }
}

void createBalls(Game *game) {
    for (int i = 0; i < BALL_COUNT; ++i) {
        Ball *b = &game->balls[i];
        b->vx = 0;
        b->vy = 0;
        b->ax = 0;
        b->ay = 0;
        b->radius = ((rand() / (float)RAND_MAX) * 
                                (game->ball_size_max - game->ball_size_min)) +
                                game->ball_size_min;
        b->px = (rand() / (float)RAND_MAX) *
                (float)game->screen_rect.w;

        b->py = (rand() / (float)RAND_MAX) *
                      (float)game->screen_rect.h;

        b->color = (rand() / (float)RAND_MAX) *
                               ((float)COLOR_SIZE - 1);
        b->mass = b->radius * 10;
    }

}

void
Game_Init(Game *game)
// All the variable and data initialization needed for SDL and perhaps game
// variables
{
    // TODO:
    // Have Game struct defined in here and then returned

    END(SDL_Init(SDL_INIT_VIDEO) != 0, "Could not create texture",
        SDL_GetError());

    END(TTF_Init() != 0, "Could not initialize TTF", TTF_GetError());



    game->window = SDL_CreateWindow("balls", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, game->screen_rect.w, 
                     game->screen_rect.h, SDL_WINDOW_SHOWN);

    END(game->window == NULL, "Could not create window", SDL_GetError());

    game->renderer = SDL_CreateRenderer(game->window, 0,
                                        SDL_RENDERER_ACCELERATED);

    END(game->renderer == NULL, "Could not create renderer", SDL_GetError());
    // game->fps = -1;
    game->fps = 60;
    srand(time(NULL));

    // create 10 balls with a random position and radius
    game->ball_size_min = 30;
    game->ball_size_max = 50;
    createBalls(game);
}

void
Game_Quit(Game *game)
{
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    TTF_Quit();
    SDL_Quit();
}

int
main(void)
{
    // Constansts defined here
    Game game = {
        .ball_distinct_unordered_pairs =
            ((float)BALL_COUNT * ((float)BALL_COUNT - 1.0f)) / 2.0f,
        .screen_rect = {.x = 0, .y = 0, .w = 800, .h = 800}
    };
    Game_Init(&game);
    Game_Update(&game);
    Game_Quit(&game);
    return 0;
}
