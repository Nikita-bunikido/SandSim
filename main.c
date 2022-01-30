#include "include\raylib.h"
#include "inttypes.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"
#include "math.h"

#define WINDOW_HEIGHT   500
#define WINDOW_WIDTH    700

#define GRAVITY 1

typedef enum {
    CT_SAND,
    CT_AIR,
    CT_WATER,
    CT_WALL
} Cell_type;

typedef struct {
    Cell_type mat_id;
    uint32_t life_time;
    float velocity;
    Color color;
    bool updated;
} Cell;

Cell cells[WINDOW_HEIGHT * WINDOW_WIDTH];

Cell create_cell (Color color, Cell_type type){
    return (Cell){
        .mat_id = type,
        .life_time = 0,
        .velocity = 1.0f,
        .color = color,
        .updated = false
    };
}

void draw_cells(void){
    for (uint32_t py = 0; py < WINDOW_HEIGHT; py++)
        for (uint32_t px = 0; px < WINDOW_WIDTH; px++)
            DrawPixel(px, py, cells[px+py*WINDOW_WIDTH].color);
}

void update_sand (uint32_t posx, uint32_t posy){
    Cell *cur = &cells[(int)(posx+posy*WINDOW_WIDTH)];

    assert(cur->mat_id == CT_SAND);

    int8_t offset = GetRandomValue(0, 1);
    int8_t offsets[2][3] = {
        {0, -1, 1},
        {0, 1, -1}
    };

    /* 3 possible directions */
    for (int8_t i = 0; i < 3; i++){
        size_t idx = (size_t)((posx+offsets[offset][i]) + (posy+1) * WINDOW_WIDTH);
        Cell *ch = &cells[idx];
        if (ch->mat_id == CT_AIR){
            Cell tmp = *ch;
            *ch = *cur;
            *cur = tmp;
            break;
        }
    }
}

void update_cells (void){
    for (uint32_t py = WINDOW_HEIGHT-1; py > 0; py--)
        for (uint32_t px = 0; px < WINDOW_WIDTH; px++){
            if (cells[px+py*WINDOW_WIDTH].updated)
                continue;
            
            cells[px+py*WINDOW_WIDTH].velocity += GRAVITY;
            Cell_type mat_id = cells[px+py*WINDOW_WIDTH].mat_id;

            switch (mat_id){
            case CT_SAND:
                update_sand(px, py);
                break;
            default:
                break;
            }
            
            cells[px+py*WINDOW_WIDTH].updated = true;
    }
}

void create_image_cells (Image *img, uint32_t ox, uint32_t oy){
    for (uint32_t py = 0; py < img->height; py++)
        for (uint32_t px = 0; px < img->width; px++){
            cells[(px+ox)+(py+oy)*WINDOW_WIDTH] = create_cell(GetImageColor(*img, px, py), CT_SAND);
        }
}

void create_circle_cells (Vector2 center, uint32_t radius){
    for (uint32_t py = center.y - radius; py < center.y + radius; py++)
        for (uint32_t px = center.x - radius; px < center.x + radius; px++){
            if (sqrt(pow(px-center.x, 2)+pow(py-center.y, 2))+GetRandomValue(0,1) < radius)
                cells[px+py*WINDOW_WIDTH] = create_cell(BLACK, CT_WALL);
        }
}


int main(){
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "real-time sand simulation");
    SetTargetFPS(60);

    #define IMAGES_COUNT    7
    Image images[IMAGES_COUNT];

    const char* path[IMAGES_COUNT] = {
        "source\\hydrant.png",
        "source\\cat1.png",
        "source\\cat2.png",
        "source\\suslik.png",
        "source\\floppa.png",
        "source\\tiger.png",
        "source\\c.png",
    };
    
    for (uint8_t i = 0; i < IMAGES_COUNT; i++)
        images[i] = LoadImage(path[i]);
    RenderTexture2D target = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    Shader post = LoadShader(NULL, "source\\bloom.fs");
    memset(cells, 0, sizeof(cells));
    for (uint64_t i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++){
        cells[i] = create_cell(RAYWHITE, CT_AIR);
    }

    for (uint32_t x = 0; x < WINDOW_WIDTH; x++){
        for (uint32_t y = WINDOW_HEIGHT-1; y >= WINDOW_HEIGHT-1-GetRandomValue(1,5); y--){
            cells[x+y*WINDOW_WIDTH] = create_cell(DARKBROWN, CT_WALL);
        }
    }

    while (!WindowShouldClose()){
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            draw_cells();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginShaderMode(post);
                DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();
            DrawCircleLines(GetMouseX(), GetMouseY(), 10, MAROON);
            DrawFPS(0,0);
        EndDrawing();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
            create_circle_cells(GetMousePosition(), 10);
        }

        for (uint8_t i = 0; i < IMAGES_COUNT; i++){
            if (IsKeyPressed("1234567"[i]))
                create_image_cells(&images[i], (uint32_t)GetMouseX(), (uint32_t)GetMouseY());
        }

        for (uint64_t i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
            cells[i].updated = false;
        for (int u = 0; u < 2; u++)
        update_cells();
    }

    UnloadRenderTexture(target);
    for (uint8_t i = 0; i < IMAGES_COUNT; i++)
        UnloadImage(images[i]);
    UnloadShader(post);
    CloseWindow();
    return 0;
}