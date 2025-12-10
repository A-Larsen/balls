```c
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <math.h>

// -----------------------------------------
// Utility functions
// -----------------------------------------
float clamp(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

bool circleRectCollide(float cx, float cy, float r,
                       float rx, float ry, float rw, float rh)
{
    float closestX = clamp(cx, rx, rx + rw);
    float closestY = clamp(cy, ry, ry + rh);

    float dx = cx - closestX;
    float dy = cy - closestY;

    return (dx*dx + dy*dy) <= (r*r);
}

// -----------------------------------------
// Main SDL demo
// -----------------------------------------
int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "Circle vs Rectangle Collision - SDL2", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Circle properties
    float cx = 100.0f;
    float cy = 100.0f;
    float cr = 25.0f;
    float speed = 200.0f; // pixels per second

    // Rectangle properties
    float rx = 400.0f;
    float ry = 250.0f;
    float rw = 150.0f;
    float rh = 100.0f;

    Uint32 last = SDL_GetTicks();
    bool running = true;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        last = now;

        // ------------------
        // Input
        // ------------------
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_ESCAPE]) running = false;

        // Move circle
        if (keys[SDL_SCANCODE_W]) cy -= speed * dt;
        if (keys[SDL_SCANCODE_S]) cy += speed * dt;
        if (keys[SDL_SCANCODE_A]) cx -= speed * dt;
        if (keys[SDL_SCANCODE_D]) cx += speed * dt;

        // ------------------
        // Collision check
        // ------------------
        bool hit = circleRectCollide(cx, cy, cr, rx, ry, rw, rh);

        // ------------------
        // Render
        // ------------------
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        // Rectangle (turn red when hit)
        if (hit)
            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        else
            SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);

        SDL_Rect rect = { (int)rx, (int)ry, (int)rw, (int)rh };
        SDL_RenderFillRect(renderer, &rect);

        // Draw circle (simple filled circle)
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        for (int w = 0; w < cr * 2; w++) {
            for (int h = 0; h < cr * 2; h++) {
                int dx = cr - w;
                int dy = cr - h;
                if ((dx*dx + dy*dy) <= (cr * cr)) {
                    SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
                }
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```
