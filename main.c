#include "include\raylib.h"
#include "inttypes.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"
#include "math.h"

#define WINDOW_HEIGHT   500
#define WINDOW_WIDTH    700

#define GRAVITY 1
#define COORDS_CONVERT(x,y) ((x)+(y)*WINDOW_WIDTH)

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

typedef struct {
    uint32_t posx, posy;
    uint32_t sizex, sizey;
    Color color;
    bool frame;
} button_t;

Cell cells[WINDOW_HEIGHT * WINDOW_WIDTH];
static Cell_type drawing_mode = CT_SAND;

void draw_button (button_t *but){ DrawRectangle(but->posx, but->posy, but->sizex, but->sizey, but->color); if (but->frame) DrawRectangleLinesEx((Rectangle){but->posx, but->posy, but->sizex, but->sizey}, 3, BLACK); }
bool mouse_in_button (button_t *but, Vector2 mouse_pos){
    return ((mouse_pos.x >= but->posx && mouse_pos.x <= but->posx + but->sizex) &&
        (mouse_pos.x >= but->posy && mouse_pos.y <= but->posy + but->sizey)) ? true : false;
}
button_t create_button (uint32_t px, uint32_t py, uint32_t sx, uint32_t sy, Color col){
    button_t b;
    b.color = col;
    b.posx = px;
    b.posy = py;
    b.sizex = sx;
    b.sizey = sy;
    b.frame = false;
    return b;
}

Cell create_cell (Color color, Cell_type type){
    return (Cell){
        .mat_id = type,
        .life_time = 0,
        .velocity = 1.0f,
        .color = color,
        .updated = false
    };
}

Cell* get_cell (uint32_t posx, uint32_t posy){
    assert(posx >= 0);
    assert(posy < WINDOW_HEIGHT*WINDOW_WIDTH);
    return cells + COORDS_CONVERT(posx,posy);
}

void draw_cells(void){
    for (uint32_t py = 0; py < WINDOW_HEIGHT; py++)
        for (uint32_t px = 0; px < WINDOW_WIDTH; px++)
            DrawPixel(px, py, cells[px+py*WINDOW_WIDTH].color);
}

void update_sand (uint32_t posx, uint32_t posy){
    Cell *cursor = get_cell(posx, posy);
    assert(cursor->mat_id == CT_SAND);

    int8_t offset = GetRandomValue(0,1);
    static const int8_t offsets[2][3] = {
        {0, -1, 1},
        {0, 1, -1}
    };
    int8_t water_attach = GetRandomValue(0,254);

    /* 3 possible directions */
    for (int8_t i = 0; i < 3; i++){
        Cell *exp = get_cell(posx+offsets[offset][i], posy+1);
        if (exp->mat_id == CT_AIR){
            Cell tmp = *exp;
            *exp = *cursor;
            *cursor = tmp;
            return;
        } else if (water_attach && exp->mat_id == CT_WATER){
            Cell tmp = *exp;
            *exp = *cursor;
            *cursor = tmp;
            return;   
        }
    }
}

void update_water (uint32_t posx, uint32_t posy){
    Cell *cursor = get_cell(posx, posy);
    assert(cursor->mat_id == CT_WATER);

    int8_t offset = GetRandomValue(0,1);
    static const int8_t offsets[2][3] = {
        {0, -1, 1},
        {0, 1, -1}
    };

    for (int8_t oy = 1; oy >= 0; oy--)
        for (int8_t ox = 0; ox < 2; ox++){
            if (ox == 0 && oy == 0)
                continue;
            Cell *exp = get_cell(posx+offsets[offset][ox], posy+oy);
            if (exp->mat_id == CT_AIR){
                Cell tmp = *exp;
                *exp = *cursor;
                *cursor = tmp;
                return;
            }
        }
}

void update_cells (void){
    for (uint32_t py = WINDOW_HEIGHT-1; py > 0; py--)
        for (uint32_t px = 0; px < WINDOW_WIDTH; px++){
            Cell *cursor = get_cell(px,py);
            if (cursor->updated)
                continue;
            
            cursor->velocity += GRAVITY;
            
            switch (cursor->mat_id){
            case CT_SAND:
                update_sand(px, py);
                break;
            case CT_WATER:
                update_water(px, py);
            default:
                break;
            }
            
            cursor->updated = true;
    }
}

void create_image_cells (Image *img, uint32_t ox, uint32_t oy){
    for (uint32_t py = 0; py < img->height; py++)
        for (uint32_t px = 0; px < img->width; px++){
            *get_cell(px+ox, py+oy) = create_cell(GetImageColor(*img, px, py), drawing_mode);
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

    const uint32_t button_size = 20;
    button_t buttons[2] = {
        create_button(80, 5, button_size, button_size, (Color){222, 205, 111, 255}),
        create_button(80 + button_size + 5, 5, button_size, button_size, (Color){86, 150, 219, 255})
    };
    bool button_pressed = false;

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
            
            for (int8_t i = 0; i < 2; i++){
                draw_button(buttons+i);    
            }
            DrawCircleLines(GetMouseX(), GetMouseY(), 10, MAROON);
            DrawFPS(0,0);
        EndDrawing();

        Vector2 mouse_position = GetMousePosition();
        
        
        /* cell type switching */
        button_pressed = false;
        for (int8_t i = 0; i < 2; i++){
            buttons[i].frame = false;
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mouse_in_button(&buttons[i], mouse_position)){
                    drawing_mode = (Cell_type[]){CT_SAND, CT_WATER}[i];
                    button_pressed = true;
                    buttons[i].frame = true;
                }
        }
        if (button_pressed)
            continue;

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
            create_circle_cells(mouse_position, 10);
        }

        for (uint8_t i = 0; i < IMAGES_COUNT; i++){
            if (IsKeyPressed("1234567"[i]))
                create_image_cells(&images[i], (uint32_t)(mouse_position.x), (uint32_t)(mouse_position.y));
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