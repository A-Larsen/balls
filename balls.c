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

#define METER_AS_PIXELS 3779U
#define BALL_COUNT 30

// (BALL_COUNT * (BALL_COUNT - 1)) / 2
// #define BALL_DISTINCT_UNORDERED_PAIRS 435

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
    SDL_Surface *backbuffer;
    SDL_Window *window;
    const SDL_Rect screen_rect;
    Ball balls[BALL_COUNT];
    uint32_t fps;
    float terminal_velocity;
    uint8_t ball_size_min;
    uint8_t ball_size_max;
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
      COLOR_PURPLE, COLOR_NEON_GREEN, COLOR_PINK, COLOR_YELLOW, COLOR_WHITE, 
      COLOR_BLACK, COLOR_SIZE};

const SDL_Color colors[] = {
    [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
    [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
    [COLOR_BLUE] = {.r = 39, .g = 211, .b = 245, .a = 255},
    [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
    [COLOR_GREY] = {.r = 89, .g = 89, .b = 89, .a = 89},
    [COLOR_YELLOW] = {.r = 230, .g = 240, .b = 0, .a = 255},
    [COLOR_PINK] = {.r = 245, .g = 39, .b = 108, .a = 255},
    [COLOR_NEON_GREEN] = {.r = 39, .g = 245, .b = 176, .a = 255},
    [COLOR_PURPLE] = {.r = 176, .g = 39, .b = 245, .a = 255},
    [COLOR_WHITE] = {.r = 255, .g = 255, .b = 255, .a = 255},
    [COLOR_BLACK] = {.r = 0, .g = 0, .b = 0, .a = 0},
};

void
setColor(SDL_Renderer *renderer,
         uint8_t color)
{

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
drawCircle(SDL_Renderer *renderer,
           SDL_Rect border,
           float radius,
           int px,
           int py,
           int w,
           uint8_t color)
{
    
    float x = 0;
    float y = 0;
    for(float i = 0; i < 2 * M_PI; i += 0.001f)
    {
        x = cosf(i);
        y = sinf(i);
        int rx = (x * radius);
        int ry = (y * radius);
        int tx = rx + px;
        int ty = ry + py;
        // wraping
        // This works except in case where a ball is in the corner. Than the
        // ball will apear in all four courners
        // SDL_Rect point = {
        //     .x = (tx > border.w) ? tx -= border.w : (tx < 0) ? tx += border.w : tx, 
        //     .y = (ty > border.h) ? ty -= border.h : (ty < 0) ? ty += border.h : ty, 
        //     w, 
        //     w
        // };
        SDL_Rect point = {
            .x = tx, 
            .y = ty, 
            w, 
            w
        };
        setColor(renderer, color);
        SDL_RenderFillRect(renderer, &point);
    }
}

void
drawBall(SDL_Renderer *renderer,
         Ball ball)
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

void
drawBall2(SDL_Surface *surface,
          Ball ball)
// (x - h)^2 + (y - k)^2 = r^2
// Drawing a circle with the standard circle formula.
// There may be a more performant way to do this...
{
    // setColor(renderer, ball.color);

    // diameter is radius * 2
    for (int x = 0; x < ball.radius * 2; x++) {
        for (int y = 0; y < ball.radius * 2; y++) {
          if ( ((x - ball.radius) * (x - ball.radius)) +
               ((y - ball.radius) * (y - ball.radius)) <=
               ball.radius * ball.radius) {
            int px = ball.px - ball.radius + x;
            int py = ball.py - ball.radius + y;

            // TODO: wrap circle around screen
            // SDL_RenderDrawPoint(renderer, px, py);
            SDL_Rect r = {.x = px, .y = py, .w = 1, .h = 1};
            uint32_t color = 
                ((colors[ball.color].r << 24) | (colors[ball.color].g << 16) |
                 (colors[ball.color].b << 8) | colors[ball.color].a);
            SDL_FillRect(surface, &r, color);

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

void
drawCursor(SDL_Renderer *renderer, SDL_Point p) {
    SDL_Rect r = {
        .x = p.x - 5,
        .y = p.y - 5,
        .w = 15,
        .h = 15
    };
    setColor(renderer, COLOR_WHITE);
    SDL_RenderFillRect(renderer, &r);
}

static uint8_t
updateMain(Game *game,
           float seconds,
           uint32_t milliseconds,
           SDL_KeyCode key,
           Mouse mouse,
           bool keydown)
{
    static float elapsedTime = 0;
    static int selected = -1;

    // Ball *colliding[BALL_DISTINCT_UNORDERED_PAIRS] = {0};
    Ball **colliding  = NULL;
    uint8_t collision_count = 0;


    // game->ball_distinct_unordered_pairs is the total number of possible
    // collisions, it is multiplied was two because there needed to be two values
    // for each collision, a ball at the target ball

    // TODO
    // use relative mouse mode for mouse movement

    // NOTE:
    // issues arise when mouse movement is too fast
    // don't check mouse click more than needed
    if(mouse.button == SDL_BUTTON_LEFT && (selected < 0)) {
        for (int i = 0; i < BALL_COUNT; ++i) {
            Ball *b = &game->balls[i];
            if (pointInBall(*b, mouse.p)) selected = i;
        }
    }

    if((!mouse.down) && (mouse.button == SDL_BUTTON_RIGHT) && (selected >= 0)) {
        Ball *b = &game->balls[selected];
        b->vx = 5.0f * (b->px - (float)mouse.p.x);
        b->vy = 5.0f * (b->py - (float)mouse.p.y);
    }

    if (!mouse.down) selected = -1;

    if(selected >= 0 && mouse.button != SDL_BUTTON_RIGHT) {
        elapsedTime = 0;
        Ball *b = &game->balls[selected];
        b->px = mouse.p.x;
        b->py = mouse.p.y;
    }  

    for (int i = 0; i < BALL_COUNT; ++i) {
        Ball *b1 = &game->balls[i];
        // drag
        b1->ax = -b1->vx * 0.8f;
        b1->ay = -b1->vy * 0.8f;

        b1->vx += b1->ax * elapsedTime;
        b1->vy += b1->ay * elapsedTime;
        b1->px += b1->vx * elapsedTime;
        b1->py += b1->vy * elapsedTime;

        // wrap around screen
        if (b1->px < 0) b1->px += (float)game->screen_rect.w;
        if (b1->px >= game->screen_rect.w) b1->px -= (float)game->screen_rect.w;
        if (b1->py < 0) b1->py += (float)game->screen_rect.h;
        if (b1->py >= game->screen_rect.h) b1->py -= (float)game->screen_rect.h;

        if (fabs(b1->vx * b1->vx + b1->vy * b1->vy) < 0.01f) {
            b1->vx = 0;
            b1->vy = 0;
        }

    }

    setColor(game->renderer, COLOR_BLACK);
    SDL_RenderClear(game->renderer);

    for (int i = 0; i < BALL_COUNT; ++i) {
        Ball *b1 = &game->balls[i];
        if (i == selected) drawBall(game->renderer, *b1);
        else drawCircle(game->renderer, game->screen_rect, b1->radius, b1->px,
                        b1->py, 2, b1->color);
        
    }

    // TODO
    // set up a list of values that will not conflict for i and j in gameInit

    // TODO
    // there is an error were the collision_count varaible will not reset to
    // zero
    for (int i = 0; i < BALL_COUNT; ++i) {
        Ball *b1 = &game->balls[i];

        for (int j = i + 1; j < BALL_COUNT; ++j) {
            Ball *b2 = &game->balls[j];

            if (!ballCollide(*b1, *b2)) continue;

            colliding = calloc(sizeof(Ball *), (collision_count + 2));
            END((!colliding), "calloc()", "could not allocate colliding()" );
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
    printf("collision count: %d\n", collision_count);
    for (int i = 0; i < collision_count; ++i) {
        Ball *b1 = colliding[i];
        Ball *b2 = colliding[i + 1];
        if (!(b1 && b2)) continue;
        float distance = getHyp(b1->px, b1->py, b2->px, b2->py);

        // normal
        float nx = (b2->px - b1->px) / distance;
        float ny = (b2->py - b1->py) / distance;

        // tangent
        float tx = -ny;
        float ty = nx;

        float dpTan1 = b1->vx * tx + b1->vy * ty;
        float dpTan2 = b2->vx * tx + b2->vy * ty;

        float dpNorm1 = b1->vx * nx + b1->vy * ny;
        float dpNorm2 = b2->vx * nx + b2->vy * ny;

        float m1 =
            (dpNorm1 * (b1->mass - b2->mass) + 2.0f * b2->mass * dpNorm2)
            / (b1->mass + b2->mass);

        float m2 =
            (dpNorm2 * (b2->mass - b1->mass) + 2.0f * b1->mass * dpNorm1)
            / (b1->mass + b2->mass);

        b1->vx = tx * dpTan1 + nx * m1;
        b1->vy = ty * dpTan1 + ny * m1;
        b2->vx = tx * dpTan2 + nx * m2;
        b2->vy = ty * dpTan2 + ny * m2;
        
    }

    if((mouse.down) && (mouse.button == SDL_BUTTON_RIGHT) && (selected >= 0)) {
        Ball *b = &game->balls[selected];
        setColor(game->renderer, COLOR_WHITE);
        SDL_RenderDrawLine(game->renderer, b->px, b->py, mouse.p.x, mouse.p.y);
    }
    if (selected < 0) drawCursor(game->renderer, mouse.p);
    // TODO
    // put a border around selected ball
    elapsedTime += 0.0008f;
    if (collision_count > 0)  {
        free(colliding);
        colliding = NULL;
    }

    return UPDATE_MAIN;
}

void
Game_Update(Game *game)
// The main game loop. Sets up which callback will be used in the function loop.
// Each update callback determines what update callback will be called next by
// returning the appropriate enum value
{
    uint8_t update_id = 0;
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;
    float mspf = (1.0f / (float)game->fps) * 1000.0f;
    SDL_Event event;
    SDL_KeyCode key = 0;
    Update_callback update;
    uint32_t ticks_start = SDL_GetTicks();
    Mouse mouse;

    // TODO
    // add backbuffer

    while (!quit) {

        // Place update functions here
        switch (update_id) {
            case UPDATE_MAIN: update = updateMain; break;
            case UPDATE_NOTHING: update = updateNothing; break;
        }

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
            // SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_NONE);
            // SDL_Texture *texture =
            //     SDL_CreateTextureFromSurface(game->renderer, game->backbuffer);
            // SDL_RenderCopyEx(game->renderer, texture, NULL, &game->screen_rect,
            //                 0, NULL, SDL_FLIP_NONE);
            SDL_RenderPresent(game->renderer);
            // SDL_DestroyTexture(texture);
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
        b->radius =
            ((rand() / (float)RAND_MAX) * (game->ball_size_max -
            game->ball_size_min)) + game->ball_size_min;

        b->px = (rand() / (float)RAND_MAX) * (float)game->screen_rect.w;

        b->py = (rand() / (float)RAND_MAX) * (float)game->screen_rect.h;

        b->color = (rand() / (float)RAND_MAX) * ((float)COLOR_SIZE - 2);

        b->mass = b->radius * 10;
        // drawBall2(game->backbuffer, *b);
    }

}

Game *
Game_Init()
// All the variable and data initialization needed for SDL and perhaps game
// variables
{
    static Game game = {
        .screen_rect = {.x = 0, .y = 0, .w = 800, .h = 800},
        .fps = 60,
        .ball_size_min = 15,
        .ball_size_max = 50,
        .backbuffer = NULL,
    };

    END(SDL_Init(SDL_INIT_VIDEO) != 0, "Could not create texture",
        SDL_GetError());

    END(TTF_Init() != 0, "Could not initialize TTF", TTF_GetError());



    game.window = SDL_CreateWindow("balls", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, game.screen_rect.w, 
                     game.screen_rect.h, SDL_WINDOW_SHOWN);

    END(game.window == NULL, "Could not create window", SDL_GetError());

    game.renderer =
        SDL_CreateRenderer(game.window, 0, SDL_RENDERER_ACCELERATED);

    END(game.renderer == NULL, "Could not create renderer", SDL_GetError());

    SDL_SetRelativeMouseMode(true);

    // setColor(game->backbuffer, COLOR_BLACK);
    // SDL_RenderClear(game->renderer);
    game.backbuffer = 
            SDL_CreateRGBSurface(0, game.screen_rect.w, game.screen_rect.h, 
                                 32, 0xFF000000, 0x00FF0000, 0x0000FF00,
                                 0x000000FF);
    // fill backbuffer with black
    SDL_FillRect(game.backbuffer, &game.screen_rect, 0x00000000);

    // print system information
    printf("big ending = %s\n", SDL_BYTEORDER == SDL_BIG_ENDIAN ? 
                                "true": "False");

    srand(time(NULL));
    createBalls(&game);

    return &game;
}

void
Game_Quit(Game *game)
{
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    SDL_FreeSurface(game->backbuffer);
    TTF_Quit();
    SDL_Quit();
}

int
main(void)
{
    Game *game = Game_Init();
    Game_Update(game);
    Game_Quit(game);
    return 0;
}
