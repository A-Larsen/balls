Below is a clear explanation of **Circle vs Rectangle collision detection**, including **math formulas**, **edge-case handling**, and **C programming examples**.

---

# 🟢 **Circle vs Axis-Aligned Rectangle Collision**

We assume the rectangle is **axis-aligned (AABB)** and defined by:

* Rectangle min corner: ((rx, ry))
* Width (w), height (h)
* So max corner is ((rx+w,; ry+h))

Circle:

* Center: ((cx, cy))
* Radius (r)

---

# ✅ **Math Formula (Shortest Distance Method)**

The basic idea:

1. **Clamp** the circle center to the edges of the rectangle
2. Compute the closest point **inside the rectangle**
3. Compute distance from the circle center to that point
4. If distance ≤ radius → collision

### 1. Clamp function

[
\text{clamp}(v, min, max) = \max(min,;\min(v, max))
]

### 2. Closest point on rectangle to circle

[
\text{closestX} = \text{clamp}(cx,; rx,; rx+w)
]
[
\text{closestY} = \text{clamp}(cy,; ry,; ry+h)
]

### 3. Distance from circle to closest point

[
dx = cx - \text{closestX}
]
[
dy = cy - \text{closestY}
]

### 4. Collision rule

[
dx^2 + dy^2 \le r^2
]

No square root needed → fast.

---

# 🟦 **C Code Example: Circle vs Rectangle Collision**

```c
#include <stdio.h>
#include <stdbool.h>

float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

typedef struct {
    float x, y;   // top-left corner
    float w, h;
} Rect;

typedef struct {
    float x, y;   // center
    float r;
} Circle;

bool circleRectCollide(Circle c, Rect r) {
    // Find the closest point on the rectangle to the circle
    float closestX = clamp(c.x, r.x, r.x + r.w);
    float closestY = clamp(c.y, r.y, r.y + r.h);

    // Compute distance from circle center to that point
    float dx = c.x - closestX;
    float dy = c.y - closestY;

    // If within radius, collision
    return (dx * dx + dy * dy) <= (c.r * c.r);
}

int main() {
    Circle c = { 5.0f, 5.0f, 3.0f };
    Rect r = { 7.0f, 4.0f, 6.0f, 4.0f };

    if (circleRectCollide(c, r)) {
        printf("Collision!\n");
    } else {
        printf("No collision.\n");
    }
}
```

---

# 🧪 **Explanation of the above example**

Circle at (5,5), radius 3
Rectangle at (7,4), size 6×4

The closest point on the rect to the circle is (7,5).
Distance = 2 → less than radius 3 → collision.

---

# 🧠 Extra: Detecting Penetration Vector / Push-out Direction

If you're making a game, you often need to **push the circle out** of the rectangle.

Short version:

1. Compute closest point
2. Compute distance
3. Compute penetration vector

Example:

```c
#include <math.h>

void circleRectPenetration(Circle c, Rect r, float *outX, float *outY) {
    float closestX = clamp(c.x, r.x, r.x + r.w);
    float closestY = clamp(c.y, r.y, r.y + r.h);

    float dx = c.x - closestX;
    float dy = c.y - closestY;

    float distSq = dx*dx + dy*dy;
    float dist = sqrtf(distSq);

    float penetration = c.r - dist;

    if (penetration > 0) {
        *outX = (dx / dist) * penetration;
        *outY = (dy / dist) * penetration;
    } else {
        *outX = *outY = 0.0f;
    }
}
```

This gives the shortest vector needed to push the circle out of the rectangle.

---

# Want more?

I can also show:

✔ **Circle vs rotated rectangle (OBB)**
✔ **Collision response (velocity reflection)**
✔ **Graphical ASCII demo**
✔ **SDL2 example with moving circle and rectangle**

Just tell me!
