#include "pch.h"
#include "Garbage.hpp"

#include <raylib.h>
#include <rlgl.h>

const int screenWidth = 1024;
const int screenHeight = 720;

float memoryInMB(size_t bytes)
{
    return static_cast<float>(bytes) / (1024.0f * 1024.0f);
}

float memoryInKB(size_t bytes)
{
    return static_cast<float>(bytes) / 1024.0f;
}

const char *memoryIn(size_t bytes)
{
    if (bytes >= 1.0e6)
    {
        return TextFormat("%.2f MB", memoryInMB(bytes));
    }
    else if (bytes >= 1.0e3)
    {
        return TextFormat("%.2f KB", memoryInKB(bytes));
    }
    return TextFormat("%zu bytes", bytes);
}

Texture2D bunnyTex;

float gravity = 0.5;
int maxX = 0;
int maxY = 0;
int minX = 0;
int minY = 0;

static const double rand_scale = 1.0 / (1 << 16) / (1 << 16);
double range()
{
    unsigned int lo = rand() & 0xfff;
    unsigned int mid = rand() & 0xfff;
    unsigned int hi = rand() & 0xff;
    double result = (lo | (mid << 12) | (hi << 24)) * rand_scale;
    return result;
}
double range(double min, double max)
{

    unsigned int lo = rand() & 0xfff;  // 12 bits
    unsigned int mid = rand() & 0xfff; // 12 bits
    unsigned int hi = rand() & 0xff;   // 8 bits

    double normalized = (lo | (mid << 12) | (hi << 24)) * rand_scale;

    return min + normalized * (max - min);
}

struct Bunny
{

    double speedX;
    double speedY;
    double x;
    double y;
    double life;
    Color color;
    bool die;

    Bunny()
    {
        x = GetMouseX();
        y = GetMouseY();
        speedX = range() * 8;
        speedY = range() * 5 - 2.5;
        color.r = range() * 255;
        color.g = range() * 255;
        color.b = range() * 255;
        color.a = 255;
        life = 500;
        die = false;
    }

    void render()
    {
        life -= 1;

        if (life <= 0)
        {
            die = true;
            return;
        }
        x += speedX;
        y += speedY;
        speedY += gravity;

        if (x > maxX)
        {
            speedX *= -1;
            x = maxX;
        }
        else if (x < minX)
        {
            speedX *= -1;
            x = minX;
        }

        if (y > maxY)
        {
            speedY *= -0.8;
            y = maxY;

            if (range() > 0.5)
                speedY -= 3 + range() * 4;
        }
        else if (y < minY)
        {
            speedY = 0;
            y = minY;
        }

        DrawTexture(bunnyTex, x, y, color);
    }

    ~Bunny()
    {
        std::cout << "Bunny deleted" << std::endl;
    }
};

void on_delete(Pointer *p)
{
    Bunny *b = static_cast<Bunny *>(p->value);
    delete b;

    //*std::cout << "on_delete" << std::endl;
}

int main()
{

    Scope *global = NEW_SCOPE(nullptr);
    Scope *local = NEW_SCOPE(global);

    Factory::as().setOnDelete(on_delete);

    ADD_ROOT(global);
    ADD_ROOT(local);

    global->define("a", 1);
    global->define("b", 2);
    global->define("c", 3);
    global->define("d", 4);


    global->define("a", 5);
    global->define("a", 5);
    global->define("a", 5);
    global->define("a", 5);

    local->lookup("a");

    List *list = NEW_LIST();
    ADD_ROOT(list);


    // Map *map = NEW_MAP();
    // ADD_ROOT(map);

    // map->insert(NEW_STRING("one"), NEW_INTEGER(1));
    // map->insert(NEW_STRING("two"), NEW_INTEGER(2));
    // map->insert(NEW_STRING("three"), NEW_INTEGER(3));
    // map->insert(NEW_STRING("four"), NEW_INTEGER(4));

    // if (map->contains(NEW_STRING("foura")))
    // {
    //     Object *value = map->get(NEW_STRING("foura"));
    //     Integer *i = static_cast<Integer *>(value);
    //     std::cout << "Value: " << i->value << std::endl;
    // }
    // else
    // {
    //     std::cout << "Not found" << std::endl;
    // }

    InitWindow(screenWidth, screenHeight, "BuEngine");
    SetTargetFPS(60);

      bunnyTex = LoadTexture("wabbit_alpha.png");

    maxX = screenWidth - bunnyTex.width;
    maxY = screenHeight - bunnyTex.height;
    minX= bunnyTex.width;
    minY = bunnyTex.height;


   // int i = 0;
    int count = 0;
    while (!WindowShouldClose())
    {

        local->define("mouse_x", GetMouseX());
        local->define("mouse_y", GetMouseY());
        local->define("down",(int)IsMouseButtonDown(MOUSE_LEFT_BUTTON));

        // for (int j = 0;j<500;j++)
        // {
        //      local->define("count", j);

        // }
        // local->define("index", i++);

        int mouse_x=  local->getInt("mouse_x");
        int mouse_y=  local->getInt("mouse_y");
        int down=     local->getInt("down");
        if (down)
        {
            count++;
            Pointer *buffer = NEW_POINTER(count);
            buffer->value=(void*) new Bunny();
            list->add(buffer);
        }

       // local->print();

        BeginDrawing();

        ClearBackground(BLACK);

        DrawCircle(mouse_x, mouse_y, 10, GREEN);

        int i = 0;
        int count = list->size();
        while (i < (int)count)
        {
            Object *p = list->get(i);
            Pointer *ob = static_cast<Pointer *>(p);
            if (!ob) 
            {
                i++;
                continue;
            }
            if (!ob->value)
            {
                i++;
                continue;
            }
            Bunny *bunny = static_cast<Bunny*>(ob->value);
            if (bunny->die)
            {
                list->erase(i);
                count = list->size();
                i++;
                continue;
            }
            bunny->render();
            i++;
        }

        DrawFPS(10, 10);
        DrawText(TextFormat("Memory: %s Objects : %zu", memoryIn(Arena::as().size()),Factory::as().size()), 10, 40, 20, WHITE);
        DrawText(TextFormat("Bunnys : %zu", list->size()), 10, 60, 20, WHITE);

        EndDrawing();

    }

    CloseWindow();

    REMOVE_ROOT(list);
    REMOVE_ROOT(local);
    REMOVE_ROOT(global);

    std::cout << "Exit!" << std::endl;
    Factory::as().clean();
    return 0;
}