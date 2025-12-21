Below is a **clear physics explanation**, the **math**, and **C programming examples** showing how to simulate a bouncing ball.

---

# 🌟 **Physics of a Bouncing Ball**

A bouncing ball is controlled by three main ideas:

---

# 1️⃣ **Gravity Accelerates the Ball Downward**

The ball’s vertical motion follows:

[
h(t) = h_0 - \tfrac{1}{2} g t^2
]

Velocity increases downward as:

[
v(t) = -gt
]

Where:

* (h) = height
* (g = 9.8\ \text{m/s}^2)
* (v) = velocity
* Downward is negative

---

# 2️⃣ **Impact With the Ground**

When the ball hits the ground, it changes direction.

A real ball **does NOT return to full height**; some energy is lost.

This is described using the **coefficient of restitution**, (e):

[
e = \frac{v_{\text{after}}}{v_{\text{before}}}
]

Typical values:

* Rubber ball: (e \approx 0.8 - 0.9)
* Basketball: (e \approx 0.75)
* Tennis ball: (e \approx 0.6)

Meaning:

* If velocity before impact is (-10\ \text{m/s})
* With (e = 0.8), after impact:
  [
  v_{\text{after}} = -e \cdot v_{\text{before}} = 8\ \text{m/s}
  ]

(The sign flips because the direction reverses.)

---

# 3️⃣ **Each Bounce Gets Shorter**

Because energy is lost each bounce, the maximum height shrinks:

[
h_{\text{after}} = e^2 \cdot h_{\text{before}}
]

---

# ⭐ **MATHEMATICAL MODEL**

Let:

* (h) = current height
* (v) = velocity
* (dt) = time step

Update each frame:

### Gravity:

[
v = v - g \cdot dt
]

### Position:

[
h = h + v \cdot dt
]

### Bounce:

If (h < 0):
[
h = 0
]
[
v = -e \cdot v
]

---

# 🖥️ **C Programming Example — Simple Bounce Simulation**

This uses a time-step loop to simulate the ball in text mode.

```c
#include <stdio.h>

int main() {
    double h = 10.0;     // initial height in meters
    double v = 0.0;      // initial velocity
    double g = 9.8;      // gravity
    double e = 0.8;      // coefficient of restitution
    double dt = 0.01;    // time step (seconds)

    for (int i = 0; i < 5000; i++) {
        // apply gravity
        v = v - g * dt;

        // update position
        h = h + v * dt;

        // bounce
        if (h < 0) {
            h = 0;
            v = -v * e;
        }

        printf("time = %.2f s, height = %.3f m, velocity = %.3f m/s\n",
               i * dt, h, v);
    }

    return 0;
}
```

This produces output like:

```
time = 0.00 s, height = 9.999 m, velocity = -0.098 m/s
time = 0.01 s, height = 9.997 m, velocity = -0.196 m/s
...
Bounce!
...
```

A very realistic-looking bounce.

---

# 🖥️ **C Example — Detecting Ground Collision Inside a Loop**

```c
if (h <= 0) {
    h = 0;
    v = -v * e;   // reverse and reduce velocity
}
```

This is the typical “bounce line” used in game loops.

---

# 🖥️ **C Example — Continuous Simulation with ASCII Display**

```c
#include <stdio.h>
#include <unistd.h>

int main() {
    double h = 20.0;
    double v = 0.0;
    double g = 9.8;
    double e = 0.85;
    double dt = 0.02;

    while (1) {
        v = v - g * dt;
        h = h + v * dt;

        if (h < 0) {
            h = 0;
            v = -v * e;
        }

        int screen_height = 25;
        int pos = (int)(screen_height - h);

        printf("\033[2J\033[1;1H");  // clear screen
        for (int i = 0; i < pos; i++) printf("\n");
        printf("   o\n");  // the ball
        fflush(stdout);

        usleep(20000);
    }
}
```

This animates the bouncing ball in your terminal.

---

# 🎯 Summary

| Concept                     | Equation                                   |
| --------------------------- | ------------------------------------------ |
| Gravity acceleration        | (v = v - g \cdot dt)                       |
| Position update             | (h = h + v \cdot dt)                       |
| Bounce                      | (v = -e v)                                 |
| New max height after bounce | (h_{\text{after}} = e^2 h_{\text{before}}) |

This combination gives a physically accurate bouncing ball.

---

If you want, I can also show:

* horizontal + vertical motion (parabolic bounce)
* collisions with walls
* 2D physics with vectors
* full game loop frameworks for SDL, Raylib, or OpenGL
