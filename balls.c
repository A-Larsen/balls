/* 
 * Copyright (C) 2024  Austin Larsen
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define SCREEN_WIDTH_PX 1600
#define SCREEN_HEIGHT_PX 800
#define ACC_GRAVITY_MPS 9.81f
#define GROUND_HEIGHT_PX 750

typedef struct _Mouse {
    int x;
    int y;
    uint32_t button;
} Mouse;

enum /* color */ {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
                  COLOR_WHITE, COLOR_BLACK, COLOR_SIZE};

void getMouse(Mouse *mouse);
void update(SDL_Renderer *renderer, uint64_t frame, float seconds,
            SDL_KeyCode key, Mouse *mouse);
void
setColor(SDL_Renderer *renderer, uint8_t color);



void
drawBall(SDL_Renderer *renderer, int px, int py, uint8_t radius, uint8_t color)
// (x - h)^2 + (y - k)^2 = r^2
// Drawing a circle with the standard circle formula.
// There may be a more performant way to do this...
{
    setColor(renderer, color);

    // diameter is radius * 2
    for (int x = 0; x < radius * 2; x++) {
        for (int y = 0; y < radius * 2; y++) {
          if ( ((x - radius) * (x - radius)) +
               ((y - radius) * (y - radius)) <=
               radius * radius) {
                int tx = px - radius + x;
                int ty = py - radius + y;

            // TODO: wrap circle around screen
            SDL_RenderDrawPoint(renderer, tx, ty);

          }
        }
    }
}

int main(void)
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    printf("\033[2J"); // clear entire screen escape sequence

    { // SDL Initialization
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            fprintf(stderr, "could not init video\n%s", SDL_GetError());

        window = SDL_CreateWindow("Projectile",
                                             SDL_WINDOWPOS_UNDEFINED,
                                             SDL_WINDOWPOS_UNDEFINED,
                                             SCREEN_WIDTH_PX, SCREEN_HEIGHT_PX,
                                             SDL_WINDOW_SHOWN);

        renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);
    } // SDL Initialization


    { // game loop
        bool quit = false;
        const uint16_t fps = 60;
        const float mspd = (1.0f / (float)fps) * 1000.0f;
        uint64_t frame = 0;
        float seconds = 0;

        while (!quit) {
            uint32_t loop_start = SDL_GetTicks();

            SDL_Event event;
            SDL_KeyCode key = 0;
            Mouse mouse;
            bool keydown = false;

            getMouse(&mouse);

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

            setColor(renderer, COLOR_BLACK);
            SDL_RenderClear(renderer);
            update(renderer, frame, seconds, key, &mouse);
            SDL_RenderPresent(renderer);
            frame++;
            seconds = ((float)frame / (float)fps);

            uint32_t loop_end = SDL_GetTicks();
            uint32_t elapsed_time = loop_end - loop_start;

            uint32_t delay = ceilf(mspd - (float)elapsed_time);

            if (delay > 0) {
                SDL_Delay(delay);
            }
        }

    } // game loop


    { // quit
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    } // quit

}

void getMouse(Mouse * mouse)
{
    mouse->button = SDL_GetMouseState(&mouse->x, &mouse->y);
}

void drawPath(SDL_Renderer *renderer, SDL_Point *point, float velocity,
              float angle, float seconds)
    // velocity in meters per second
{
    // REMEMBER! negative is upward and positive is downward. That is why
    // the initial velocity for y (vi_y) is negative and acceleration due to
    // gravity (ACC_GRAVITY_MPS) is positive
    float vi_y = -velocity * sinf(angle);
    float vi_x = velocity * cosf(angle);
    float delta_x = (float)vi_x * (float)seconds;
    float delta_y = (float)vi_y * (float)seconds + (0.5f * ACC_GRAVITY_MPS
                        * (seconds * seconds));

    // SDL_Rect rect = {
    //     .x = point->x + delta_x,
    //     .y = point->y + delta_y,
    //     .w = 10,
    //     .h = 10
    // };
    int tx = point->x + delta_x;
    int ty = point->y + delta_y;

    if (delta_y <= -1) {
        printf("\033[H"); // clear and set to home position escape sequence
        printf("seconds: %f\n", seconds);
        printf("y displacement: %f\n", delta_y);
        printf("x displacement: %f\n", delta_x);
        printf("x: %d\n", tx);
        printf("y: %d\n", ty);
        // SDL_RenderFillRect(renderer, &rect);
        drawBall(renderer, tx, ty, 20, COLOR_BLUE);
    }

}

void update(SDL_Renderer *renderer, uint64_t frame, float seconds,
            SDL_KeyCode key, Mouse *mouse)
{
    static float launch_start = 0;
    static float launch_angle = 0;
    static float launch_velocity = 0;

    SDL_Point point = {
        .x = 10,
        .y = GROUND_HEIGHT_PX
    };

    // 0 to 90 degrees (1/2 pi)
    setColor(renderer, COLOR_BLUE);
    SDL_RenderDrawLine(renderer, point.x, point.y, mouse->x, mouse->y);


    int opposite = (mouse->y - point.y);
    int adjacent = (mouse->x - point.x);
    float angle = -atanf((float)opposite / (float)adjacent);

    setColor(renderer, COLOR_RED);
    SDL_RenderDrawLine(renderer, 0, GROUND_HEIGHT_PX, SCREEN_WIDTH_PX,
                       GROUND_HEIGHT_PX);

    if (mouse->button == 1) {
        launch_start = seconds;
        launch_angle = angle;
        // hypotenuse is velocity
        launch_velocity = sqrtf((opposite * opposite) + (adjacent * adjacent));
    }

    if (launch_start > 0) {
        drawPath(renderer, &point, launch_velocity, launch_angle, seconds -
                 launch_start);
    }
}

void
setColor(SDL_Renderer *renderer, uint8_t color)
{
    const SDL_Color colors[] = {
        [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
        [COLOR_WHITE] = {.r = 255, .g = 255, .b = 255, .a = 255},
        [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
        [COLOR_BLUE] = {.r = 146, .g = 161, .b = 185, .a = 255},
        [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
        [COLOR_GREY] = {.r = 89, .g = 89, .b = 89, .a = 89},
        [COLOR_BLACK] = {.r = 0, .g = 0, .b = 0, .a = 0},
    };

    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g,
                           colors[color].b, colors[color].a);
}

