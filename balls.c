#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <SDL2/SDL_rect.h>

// NOTE:
// This is a less accurate depiction of gravity. I am using a different number
// as the mesurement in meters per second because you cannot have fractions of a
// pixel. In order to depict this more accurately I would need an approximate
// size of a pixel, and not every pixel is the same size. I have decided not to
// be THAT accurate.
#define METER_AS_PIXELS 3779U
// #define GRAVITY 0.5f

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
    bool out_of_bounds;
    uint16_t fps;
    float gravity;
    float terminal_velocity;
    SDL_Rect screen_rect;
} Game;

typedef struct _Ball {
    SDL_Point center; 
    SDL_Point velocities;
    uint16_t radius;
    bool vertical; // check if the ball is 
    bool horizontal;
    // these two varaibles reset with each fall
    float time_falling; // time it takes to fall from peak
    float height_falling; // max height for each fall
    float e; // restition value
} Ball;

typedef uint8_t (*Update_callback) (Game *game, 
                                    float seconds, 
                                    uint64_t frame,
                                    SDL_KeyCode key,
                                    bool keydown);

enum {UPDATE_MAIN, UPDATE_NOTHING};

enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_BLACK, COLOR_SIZE};

void
setColor(SDL_Renderer *renderer,
         uint8_t color)
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

void
drawCircle(SDL_Renderer *renderer,
           int radius,
           SDL_Point center,
           uint8_t color)
// (x - h)^2 + (y - k)^2 = r^2
// Drawing a circle with the standard circle formula.
// There may be a more performant way to do this...
{
    int size = radius;
    setColor(renderer, color);

    // diameter is radius * 2
    for (int x = 0; x < radius * 2; x++) {
        for (int y = 0; y < radius * 2; y++) {
            if (((x - size)*(x - size)) + ((y - size)*(y - size)) <= radius
                 * radius)
                SDL_RenderDrawPoint(renderer, center.x - radius + x, center.y - radius + y);
        }
    }
}

float clamp(float v, float min, float max) {
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
           uint64_t frame,
           SDL_KeyCode key,
           bool keydown)
{
    return UPDATE_NOTHING;
}

void
worldBoundry(Game *game, Ball *ball)
{
    int padding = 1;

    if (ball->center.x - ball->radius <= 0) {
        ball->center.x = ball->radius + padding;
    }

    if (ball->center.y - ball->radius <= 0) {
        ball->center.y = ball->radius + padding;
    }

    if (ball->center.x + ball->radius >= game->screen_rect.w) {
        ball->center.x = game->screen_rect.w - ball->radius - padding;
    }

    if (ball->center.y + ball->radius >= game->screen_rect.h) {
        ball->center.y = game->screen_rect.h - ball->radius - padding;
    }
}

static uint8_t
updateMain(Game *game,
           float seconds,
           uint64_t frame,
           SDL_KeyCode key,
           bool keydown)
{
    // as long as gravity and time are using the same units the fomrulas will
    // work. in this case we will do frames as out unit
    // * Rubber ball: (e \approx 0.8 - 0.9)
    // * Basketball: (e \approx 0.75)
    // * Tennis ball: (e \approx 0.6)
    static Ball ball = {
        .center = {.x = 400, .y = 400},
        .radius = 30,
        .vertical = true,
        .horizontal = true,
        .e = 0.9f
    };

    static float initial_y = 400;
    static float y = 400;
    static float restitution = 1;
    static float f = 0;
    static float velocity = 0;


    if (ball.vertical)  // falling downward
        velocity += (game->gravity * f);
     else 
        velocity -= restitution - (game->gravity * f);

    // clip this to world border
    ball.center.y = y + velocity;

    ball.height_falling = initial_y + (0.5f * game->gravity) * (f * f);

    ball.time_falling = sqrt(2 * (initial_y - ball.radius) / game->gravity);
    //
    // This if statement cannot be used because
     // if (f > time && ball.vertical) {
     if ((ball.center.y + ball.radius >= game->screen_rect.h) &&
         restitution >= 0) {
         f = 0;
         ball.vertical = false;
         restitution *= ball.e;
     }

    worldBoundry(game, &ball);

    // Should not go out of bounds. but check if it does
     game->out_of_bounds =
        !circleRectCollide(ball.center, ball.radius, game->screen_rect);

    drawCircle(game->renderer, ball.radius, ball.center, COLOR_RED);
    f++;

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
    float mspd = (1.0f / (float)game->fps) * 1000.0f;
    float seconds = 0;
    SDL_Event event;
    SDL_KeyCode key = 0;
    uint32_t end = 0;
    uint32_t elapsed_time = 0;
    uint8_t color = COLOR_BLACK;

    while (!quit) {
        uint32_t start = SDL_GetTicks();
        // clear screen
        if (game->out_of_bounds) color = COLOR_GREEN;
        setColor(game->renderer, color);
        SDL_RenderClear(game->renderer);
        // TODO:
        // find milliseconds so you can update gravity more acurately
        // (1 / frame)
        seconds = ((float)frame / (float)game->fps);
        // printf("%lu: %f\n", frame, seconds);
 

        // Place update functions here
        switch (update_id) {
            case UPDATE_MAIN: update = updateMain; break;
            case UPDATE_NOTHING: update = updateNothing; break;
            // case UPDATE_LOSE: update = updateLose; break;
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

                case SDL_KEYUP: keydown = false; break;
                case SDL_QUIT: quit = true; break;
            }
        }

        update_id = update(game, seconds, frame, key, keydown);

        end = SDL_GetTicks();
        elapsed_time = end - start;

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
// All the variable and data initialization needed for SDL and perhaps game
// variables
{
    memset(game, 0, sizeof(Game));
    game->screen_rect.x = 0;
    game->screen_rect.y = 0;
    game->screen_rect.w = 800;
    game->screen_rect.h = 800;

    END(SDL_Init(SDL_INIT_VIDEO) != 0, "Could not create texture",
        SDL_GetError());

    END(TTF_Init() != 0, "Could not initialize TTF", TTF_GetError());



    game->window = SDL_CreateWindow("balls", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, game->screen_rect.w, 
                     game->screen_rect.h, SDL_WINDOW_SHOWN);

    END(game->window == NULL, "Could not create window", SDL_GetError());

    game->renderer = SDL_CreateRenderer(game->window, 0,
                                        SDL_RENDERER_SOFTWARE);

    END(game->renderer == NULL, "Could not create renderer", SDL_GetError());
    game->fps = 400;
    // game->terminal_velocity = 
    game->gravity  = 0.004f;
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
    Game game;
    Game_Init(&game);
    Game_Update(&game);
    Game_Quit(&game);
    return 0;
}
