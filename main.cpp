#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}
//sizes are in pixels
#define SCREEN_WIDTH    1280
#define SCREEN_HEIGHT   720
#define LEVEL_SIZE 2000         //both width and height
#define PLAYER_SPEED 800.0
#define ENEMY_SPEED 260

#define CANNONBALL_SPEED 200.0
#define SHOT_LASTS_TICKS 4800

#define SHOT_TICKS_FORT 300
#define BULLET_COUNT_FORT SHOT_LASTS_TICKS/SHOT_TICKS_FORT //should be multiplier of shooting_sides
#define FORT_SHOOTING_SIDES 8

#define SHOT_TICKS_ENEMY_SHIP 600
#define BULLET_COUNT_ENEMY_SHIP SHOT_LASTS_TICKS/SHOT_TICKS_ENEMY_SHIP * 4 //*4 because ship shoots 4 sides at time
#define SHIP_SHOOTING_SIDES 4

#define SHOT_TICKS_SHOTGUN 400
#define BULLET_COUNT_SHOTGUN SHOT_LASTS_TICKS/SHOT_TICKS_SHOTGUN * 3 
#define SHOTGUN_SHOOTING_SIDES 4

#define ENEMY_SHIP_COUNT 2
#define FORT_COUNT 5
#define SHOTGUN_COUNT 1

#define PATTERN_TICKS_FORT 9000
#define PATTERN_TICKS_SHOTGUN 6000
#define PATTERN_TICKS_SHIP 5000

#define FORT_SIZE 155
#define ENEMY_SHIP_X 95
#define ENEMY_SHIP_Y 170
#define CANNONBALL_SIZE 15

#define MOVE_FORWARD_TICKS 6000

#define TEXT_SIZE 128
#define FORT_X 500 + SCREEN_WIDTH / 2
#define FORT_Y 500 + SCREEN_HEIGHT / 2
#define SHIP_X 250 + SCREEN_WIDTH / 2
#define SHIP_Y -800 + SCREEN_HEIGHT / 2
#define SHOTGUN_X SCREEN_WIDTH / 2
#define SHOTGUN_Y LEVEL_SIZE / 1.5
#define START_POS_X 0
#define START_POS_Y 0

#define PLAYER_SIZE_X 30
#define PLAYER_SIZE_Y 55
#define PLAYER_HEALTH 10
#define INVINCIBILITY_TICKS 1000

#define TRUE 1
#define FALSE 0

#define LEVEL_ONE 1
#define LEVEL_TWO 2
#define LEVEL_THREE 3
#define QUIT_TEST 4
#define TESTING_LEVELS 5
#define NORMAL_LEVEL 6
#define QUIT 7
#define NEW_GAME 8
struct object_t {
    double x, y;
};
struct player_t {
    SDL_Surface* surface = NULL;
    const SDL_Rect* hitbox;
    int hp = PLAYER_HEALTH, invincibility = FALSE;
    int lastInvincibilityTime = 0;
};
struct bullet_t {
    SDL_Surface* bullet = NULL;
    const SDL_Rect* hitbox;
    object_t bulletInfo;
    int alive = FALSE;
    int pattern = 1;
};
struct enemy_t {
    object_t pos;
    bullet_t bulletsFort[BULLET_COUNT_FORT];
    bullet_t bulletsShip[BULLET_COUNT_ENEMY_SHIP];
    bullet_t bulletsShotgun[BULLET_COUNT_SHOTGUN];
    SDL_Surface* surface = NULL;
    const SDL_Rect* hitbox;
    int shotNo = 0, speed = ENEMY_SPEED, patternShotsCounter = 0;
    unsigned int lastShootingTime = 0, lastTurningTime = 0, lastPatternChangeTime;
};

// draw a text txt on surface screen, starting from the point (x, y), charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
    SDL_Surface* charset) {
    int px, py, c;
    SDL_Rect s, d;
    s.w = 8;
    s.h = 8;
    d.w = 8;
    d.h = 8;
    while (*text) {
        c = *text & 255;
        px = (c % 16) * 8;
        py = (c / 16) * 8;
        s.x = px;
        s.y = py;
        d.x = x;
        d.y = y;
        SDL_BlitSurface(charset, &s, screen, &d);
        x += 8;
        text++;
    };
};

// draw a surface sprite on a surface screen in point (x, y), (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
    SDL_Rect dest;
    dest.x = x - sprite->w / 2;
    dest.y = y - sprite->h / 2;
    dest.w = sprite->w;
    dest.h = sprite->h;
    SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void drawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32*)p = color;
};

// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line, l is length
void drawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
    for (int i = 0; i < l; i++) {
        drawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    }
}

// draw a rectangle of size l by k
void drawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
    int i;
    drawLine(screen, x, y, k, 0, 1, outlineColor);
    drawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
    drawLine(screen, x, y, l, 1, 0, outlineColor);
    drawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
    for (i = y + 1; i < y + k - 1; i++)
        drawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
}

void moveObject(object_t& object, double vertical, double horizontal, double delta)
{
    object.x += horizontal * delta;
    object.y += vertical * delta;
}

void freeAndQuit(SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer, SDL_Surface* charset, SDL_Surface* player, SDL_Surface* screen, SDL_Surface* fort, SDL_Surface* map, SDL_Surface* enemyShip, SDL_Surface* shotgun)
{
    SDL_DestroyTexture(scrtex);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(charset);
    SDL_FreeSurface(player);
    SDL_FreeSurface(screen);
    SDL_FreeSurface(fort);
    SDL_FreeSurface(map);
    SDL_FreeSurface(enemyShip);
    SDL_FreeSurface(shotgun);
    SDL_Quit();
}

void setEnemyPositionNormalLevel(enemy_t enemyShip[ENEMY_SHIP_COUNT], enemy_t fort[FORT_COUNT], enemy_t shotgun[SHOTGUN_COUNT])
{
    enemyShip[0].pos = { SHIP_X,SHIP_Y };
    enemyShip[1].pos = { -SHIP_X,SHIP_Y };
    fort[0].pos = { FORT_X, FORT_Y };
    fort[1].pos = { -FORT_X, FORT_Y };
    fort[2].pos = { -FORT_X, -FORT_Y };
    fort[3].pos = { FORT_X, -FORT_Y };
    fort[4].pos = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
    shotgun[0].pos = { SHOTGUN_X, SHOTGUN_Y };
}

SDL_Surface* preparePictures(char filename[TEXT_SIZE], SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer, SDL_Surface* charset, SDL_Surface* enemy, SDL_Surface* player, SDL_Surface* screen, SDL_Surface* fort, SDL_Surface* map, SDL_Surface* shotgun)
{
    SDL_Surface* target = NULL;
    char path[TEXT_SIZE] = "./";
    target = SDL_LoadBMP(strcat(path, filename));

    if (target == NULL) {
        printf("SDL_LoadBMP(%s) error: %s\n", filename, SDL_GetError());
        freeAndQuit(scrtex, window, renderer, charset, player, screen, fort, map, enemy, shotgun);
        exit(0);
    }
    return target;
}

void chooseLevel(int level, enemy_t* fort, enemy_t* enemyShip, enemy_t* shotgun, object_t& cameraObj)
{
    switch (level) {
    case LEVEL_ONE:
        fort[0].pos = { SCREEN_WIDTH / 2,SCREEN_HEIGHT };
        cameraObj = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
        break;
    case LEVEL_TWO:
        enemyShip[0].pos = { SCREEN_WIDTH / 2,-LEVEL_SIZE / 4 + PLAYER_SIZE_Y };
        cameraObj = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
        break;
    case LEVEL_THREE:
        shotgun[0].pos = { SCREEN_WIDTH / 2,SCREEN_HEIGHT * 1.5 };
        cameraObj = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
        break;
    case NORMAL_LEVEL:
        setEnemyPositionNormalLevel(enemyShip, fort, shotgun);
        break;
    }
}

void newGame(object_t& camera, enemy_t enemyShip[ENEMY_SHIP_COUNT], enemy_t fort[FORT_COUNT], enemy_t shotgun[SHOTGUN_COUNT], int level, player_t& player)
{
    chooseLevel(level, fort, enemyShip, shotgun, camera);
    player.hp = PLAYER_HEALTH;
    player.lastInvincibilityTime = 0;
    for (int obj = 0; obj < ENEMY_SHIP_COUNT; obj++) {
        enemyShip[obj].shotNo = 0;
        enemyShip[obj].lastTurningTime = 0;
        enemyShip[obj].speed = -ENEMY_SPEED;
        for (int i = 0; i < BULLET_COUNT_ENEMY_SHIP; i++) {
            enemyShip[obj].bulletsShip[i].bulletInfo = { 0,0 };
            enemyShip[obj].bulletsShip[i].alive = FALSE;
        }
    }
    for (int obj = 0; obj < FORT_COUNT; obj++) {
        fort[obj].shotNo = 0;
        for (int i = 0; i < BULLET_COUNT_FORT; i++) {
            fort[obj].bulletsFort[i].bulletInfo = { 0,0 };
            fort[obj].bulletsFort[i].alive = FALSE;
        }
    }
    for (int obj = 0; obj < SHOTGUN_COUNT; obj++) {
        shotgun[obj].shotNo = 0;
        for (int i = 0; i < BULLET_COUNT_SHOTGUN; i++) {
            shotgun[obj].bulletsShotgun[i].bulletInfo = { 0,0 };
            shotgun[obj].bulletsShotgun[i].alive = FALSE;
        }
    }
}

void prepareGame(SDL_Texture** scrtex, SDL_Window** window, SDL_Renderer** renderer, SDL_Surface** charset, enemy_t enemyShip[ENEMY_SHIP_COUNT], SDL_Surface** player, SDL_Surface** screen, enemy_t fort[FORT_COUNT], SDL_Surface** cannonball, int rc, unsigned int& t1, SDL_Surface** map, enemy_t shotgun[SHOTGUN_COUNT])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        exit(0);
    }

    //fullscreen mode
    //      rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, *&window, *&renderer);
    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, *&window, *&renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        exit(0);
    };

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 255);
    SDL_SetWindowTitle(*window, "This is where the fun begins");
    *screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    *scrtex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    //turn off mouse
    SDL_ShowCursor(SDL_DISABLE);

    *charset = preparePictures("graphics\\cs8x8.bmp", *scrtex, *window, *renderer, *charset, enemyShip->surface, *player, *screen, fort->surface, *map, shotgun->surface);
    SDL_SetColorKey(*charset, true, 0x000000);
    *player = preparePictures("graphics\\biggerShip.bmp", *scrtex, *window, *renderer, *charset, enemyShip->surface, *player, *screen, fort->surface, *map, shotgun->surface);
    *cannonball = preparePictures("graphics\\cannonball.bmp", *scrtex, *window, *renderer, *charset, enemyShip->surface, *player, *screen, fort->surface, *map, shotgun->surface);
    *map = preparePictures("graphics\\map.bmp", *scrtex, *window, *renderer, *charset, enemyShip->surface, *player, *screen, fort->surface, *map, shotgun->surface);

    for (int obj = 0; obj < ENEMY_SHIP_COUNT; obj++) {
        enemyShip[obj].surface = preparePictures("graphics\\enemyShip.bmp", *scrtex, *window, *renderer, *charset, enemyShip->surface, *player, *screen, fort->surface, *map, shotgun->surface);
        for (int i = 0; i < BULLET_COUNT_ENEMY_SHIP; i++) {
            enemyShip[obj].bulletsShip[i].bulletInfo = { 0,0 };
            enemyShip[obj].bulletsShip[i].bullet = *cannonball;
        }
    }
    for (int obj = 0; obj < FORT_COUNT; obj++) {
        fort[obj].surface = preparePictures("graphics\\fort4.bmp", *scrtex, *window, *renderer, *charset, enemyShip->surface, *player, *screen, fort->surface, *map, shotgun->surface);
        for (int i = 0; i < BULLET_COUNT_FORT; i++) {
            fort[obj].bulletsFort[i].bulletInfo = { 0,0 };
            fort[obj].bulletsFort[i].bullet = *cannonball;
        }
    }
    for (int obj = 0; obj < SHOTGUN_COUNT; obj++) {
        shotgun[obj].surface = preparePictures("graphics\\shotgun.bmp", *scrtex, *window, *renderer, *charset, enemyShip->surface, *player, *screen, fort->surface, *map, shotgun->surface);
        for (int i = 0; i < BULLET_COUNT_SHOTGUN; i++) {
            shotgun[obj].bulletsShotgun[i].bulletInfo = { 0,0 };
            shotgun[obj].bulletsShotgun[i].bullet = *cannonball;
        }
    }
    t1 = SDL_GetTicks();
}

int endGame(int color, SDL_Surface* screen, SDL_Surface* charset, object_t cameraObj, SDL_Texture* scrtex, SDL_Renderer* renderer)
{
    char text[TEXT_SIZE];
    SDL_Event event;
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
    int exit = FALSE;
    SDL_FillRect(screen, NULL, color);
    sprintf(text, "No one kills a pirate, try again!");
    DrawString(screen, screen->w / 2 - strlen(text) * 4, SCREEN_HEIGHT / 2, text, charset);
    sprintf(text, "Press N to play again, esc to exit");
    DrawString(screen, screen->w / 2 - strlen(text) * 4, SCREEN_HEIGHT / 1.5, text, charset);
    SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
    SDL_RenderCopy(renderer, scrtex, NULL, NULL);
    SDL_RenderPresent(renderer);
    while (TRUE)
    {
        SDL_PumpEvents();
        if (keyboardState[SDL_SCANCODE_N])
            return NEW_GAME;
        if (keyboardState[SDL_SCANCODE_ESCAPE])
            return QUIT;
    }
}

void drawEnemies(SDL_Surface* screen, enemy_t* enemy, object_t cameraObj, int enemyCount, int bulletCount, int sizeX, int sizeY)
{
    for (int i = 0; i < enemyCount; i++) {
        DrawSurface(screen, enemy[i].surface, enemy[i].pos.x - cameraObj.x, enemy[i].pos.y - cameraObj.y);
        for (int j = 0; j < bulletCount; j++) {
            if (enemy[i].bulletsFort[i].alive == TRUE)
                DrawSurface(screen, enemy[i].bulletsFort[j].bullet, enemy[i].pos.x + enemy[i].bulletsFort[j].bulletInfo.x - cameraObj.x, enemy[i].pos.y + enemy[i].bulletsFort[j].bulletInfo.y - cameraObj.y);
            if (enemy[i].bulletsShip[i].alive == TRUE)
                DrawSurface(screen, enemy[i].bulletsShip[j].bullet, enemy[i].pos.x + enemy[i].bulletsShip[j].bulletInfo.x - cameraObj.x, enemy[i].pos.y + enemy[i].bulletsShip[j].bulletInfo.y - cameraObj.y);
            if (enemy[i].bulletsShotgun[i].alive == TRUE)
                DrawSurface(screen, enemy[i].bulletsShotgun[j].bullet, enemy[i].pos.x + enemy[i].bulletsShotgun[j].bulletInfo.x - cameraObj.x, enemy[i].pos.y + enemy[i].bulletsShotgun[j].bulletInfo.y - cameraObj.y);
        }
    }
}

void drawGame(SDL_Surface* charset, SDL_Surface* playersurface, SDL_Surface* screen, SDL_Surface* map, enemy_t* fort, enemy_t* enemyShip, enemy_t* shotgun, object_t cameraObj, int infoColor, float worldTime, float fps, int playerHP, int hpColor, player_t player, int deathColor)
{
    char text[TEXT_SIZE];
    //draw map; screen_width(height)/2 - camera.x(y) makes camera coordinates and SDL coordinates in sync
    DrawSurface(screen, map, SCREEN_WIDTH / 2 - cameraObj.x, SCREEN_HEIGHT / 2 - cameraObj.y);

    //screen_width and height / 2 to make player in center
    DrawSurface(screen, playersurface, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    drawEnemies(screen, fort, cameraObj, FORT_COUNT, BULLET_COUNT_FORT, FORT_SIZE, FORT_SIZE);
    drawEnemies(screen, enemyShip, cameraObj, ENEMY_SHIP_COUNT, BULLET_COUNT_ENEMY_SHIP, ENEMY_SHIP_X, ENEMY_SHIP_Y);
    drawEnemies(screen, shotgun, cameraObj, SHOTGUN_COUNT, BULLET_COUNT_SHOTGUN, FORT_SIZE, FORT_SIZE);

    drawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 20, infoColor, infoColor);
    sprintf(text, "Czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
    DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
    drawRectangle(screen, 4, 24, playerHP * (SCREEN_WIDTH - 8) / PLAYER_HEALTH, 10, hpColor, hpColor);
}

void changeShootingPattern(enemy_t* enemy, unsigned int timeNow, int& currentPattern, int howLong)
{
    if (timeNow - enemy->lastPatternChangeTime >= howLong)
    {
        if (currentPattern == 1)
            currentPattern = 2;
        else
            currentPattern = 1;
        enemy->lastPatternChangeTime = SDL_GetTicks();
    }
}

void fortShoot(object_t cameraObj, double delta, SDL_Surface* screen, enemy_t* enemy)
{
    unsigned int timeNow = SDL_GetTicks();
    static int currentPattern = 1;
    double speedX = 0, speedY = 0;

    if (timeNow - enemy->lastShootingTime >= SHOT_TICKS_FORT) {

        //when all shots are shot, old bullets are teleported to start
        if (enemy->bulletsFort[enemy->shotNo].alive == TRUE) {
            enemy->bulletsFort[enemy->shotNo].bulletInfo.x = 0;
            enemy->bulletsFort[enemy->shotNo].bulletInfo.y = 0;
        }
        enemy->bulletsFort[enemy->shotNo].alive = TRUE;

        changeShootingPattern(enemy, timeNow, currentPattern, PATTERN_TICKS_FORT);
        enemy->bulletsFort[enemy->shotNo].pattern = currentPattern;

        enemy->lastShootingTime = SDL_GetTicks();
        enemy->shotNo++;
        if (enemy->shotNo == BULLET_COUNT_FORT)
            enemy->shotNo = 0;
    }
    for (int i = 0; i < BULLET_COUNT_FORT; i++) {
        if (enemy->bulletsFort[i].alive == TRUE)
        {
            if (enemy->bulletsFort[i].pattern == 1) {        //8 sides spiral, clockwise
                speedX = cos(M_PI * i / 4) * CANNONBALL_SPEED;
                speedY = sin(M_PI * i / 4) * CANNONBALL_SPEED;
            }
            else    //16 sides spiral, anticlockwise
            {
                speedX = -cos(M_PI * i / 8) * CANNONBALL_SPEED;
                speedY = sin(M_PI * i / 8) * CANNONBALL_SPEED;
            }
            moveObject(enemy->bulletsFort[i].bulletInfo, speedY, speedX, delta);
        }
    }
}

void enemyShipShoot(object_t cameraObj, double delta, SDL_Surface* screen, enemy_t* enemy)
{
    unsigned int timeNow = SDL_GetTicks();
    static int currentPattern = 1;
    double speedX = 0, speedY = 0;
    if (timeNow - enemy->lastShootingTime >= SHOT_TICKS_ENEMY_SHIP) {
        for (int i = 0; i < SHIP_SHOOTING_SIDES; i++)
        {
            //when all shots are shot, old bullets are teleported to start
            if (enemy->bulletsShip[enemy->shotNo].alive == TRUE) {
                enemy->bulletsShip[enemy->shotNo].bulletInfo.x = 0;
                enemy->bulletsShip[enemy->shotNo].bulletInfo.y = 0;
            }
            enemy->bulletsShip[enemy->shotNo].alive = TRUE;

            changeShootingPattern(enemy, timeNow, currentPattern, PATTERN_TICKS_SHIP);
            enemy->bulletsShip[enemy->shotNo].pattern = currentPattern;

            enemy->lastShootingTime = SDL_GetTicks();
            enemy->shotNo++;
            if (enemy->shotNo == BULLET_COUNT_ENEMY_SHIP)
                enemy->shotNo = 0;
        }
    }
    for (int i = 0; i < BULLET_COUNT_ENEMY_SHIP; i++) {
        if (enemy->bulletsShip[i].alive == TRUE)
        {
            if (enemy->bulletsShip[i].pattern == 1) {        // X pattern
                switch (i % 2) {
                case 0:
                    speedX = CANNONBALL_SPEED * (cos(M_PI * i / 2));
                    speedY = CANNONBALL_SPEED * (cos(M_PI * i / 2));
                    break;
                case 1:
                    speedX = -CANNONBALL_SPEED * (sin(M_PI * i / 2 + M_PI));
                    speedY = CANNONBALL_SPEED * (sin(M_PI * i / 2 + M_PI));
                    break;
                }
            }
            else {   // + pattern
                speedX = CANNONBALL_SPEED * sin(M_PI * i / 2);
                speedY = CANNONBALL_SPEED * cos(M_PI * i / 2);
            }
            moveObject(enemy->bulletsShip[i].bulletInfo, speedY, speedX, delta);
        }
    }
}

void shotgunShoot(object_t cameraObj, double delta, SDL_Surface* screen, enemy_t* enemy)
{
    unsigned int timeNow = SDL_GetTicks();
    static int currentPattern = 1;
    double speedX = 0, speedY = 0;
    if (timeNow - enemy->lastShootingTime >= SHOT_TICKS_SHOTGUN) {
        for (int i = 0; i < SHOTGUN_SHOOTING_SIDES; i++)
        {
            //when all shots are shot, old bullets are teleported to start
            if (enemy->bulletsShotgun[enemy->shotNo].alive == TRUE) {
                enemy->bulletsShotgun[enemy->shotNo].bulletInfo.x = 0;
                enemy->bulletsShotgun[enemy->shotNo].bulletInfo.y = 0;
            }
            enemy->bulletsShotgun[enemy->shotNo].alive = TRUE;

            changeShootingPattern(enemy, timeNow, currentPattern, PATTERN_TICKS_SHOTGUN);
            enemy->bulletsShotgun[enemy->shotNo].pattern = currentPattern;

            enemy->lastShootingTime = SDL_GetTicks();
            enemy->shotNo++;
            if (enemy->shotNo == BULLET_COUNT_SHOTGUN)
                enemy->shotNo = 0;
        }
    }
    for (int i = 0; i < BULLET_COUNT_SHOTGUN; i++) {
        if (enemy->bulletsShotgun[i].alive == TRUE)
        {
            if (enemy->bulletsShotgun[i].pattern == 1) {     // 3 in front, slightly to sides pattern
                speedX = CANNONBALL_SPEED * cos((i + 2) * M_PI / 6);
                speedY = CANNONBALL_SPEED * -1;
            }
            else {   // 180 degree pattern
                if (i % 2 == 0) {
                    speedX = CANNONBALL_SPEED;
                    speedY = CANNONBALL_SPEED * fabs(cos(i * M_PI / 4)) * -1; //0 or -1
                }
                else {
                    speedX = CANNONBALL_SPEED * -1;
                    speedY = CANNONBALL_SPEED * fabs(cos((i - 1) * M_PI / 4)) * -1; //0 or -1
                }
            }
            moveObject(enemy->bulletsShotgun[i].bulletInfo, speedY, speedX, delta);
        }
    }
}

void enemyMove(enemy_t* enemy, double vertical, double horizontal, double delta, int& afterNewGame) {
    unsigned int timeNow = SDL_GetTicks();
    for (int i = 0; i < ENEMY_SHIP_COUNT; i++) {
        if (timeNow - enemy[i].lastTurningTime >= MOVE_FORWARD_TICKS || afterNewGame == TRUE)
        {
            enemy[i].speed *= (-1);
            enemy[i].lastTurningTime = SDL_GetTicks();
            afterNewGame = FALSE;
        }
        moveObject(enemy[i].pos, enemy[i].speed, 0, delta);
    }
}

int testLevelsMenu(SDL_Surface* screen, int color, SDL_Surface* charset, SDL_Texture* scrtex, SDL_Renderer* renderer)
{
    int selected = LEVEL_ONE, down = FALSE, up = FALSE;
    const int optionsCount = 5;
    char text[TEXT_SIZE];
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
    SDL_Event event;
    while (TRUE)
    {
        SDL_FillRect(screen, NULL, color);
        sprintf(text, "CONFIRM WITH SPACE");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 1 * SCREEN_HEIGHT / optionsCount, text, charset);

        sprintf(text, "LEVEL ONE --- FORT");
        if (selected == LEVEL_ONE)
            sprintf(text, ">>>LEVEL ONE --- FORT<<<");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 2.5 * SCREEN_HEIGHT / optionsCount, text, charset);

        sprintf(text, "LEVEL TWO --- SHIP");
        if (selected == LEVEL_TWO)
            sprintf(text, ">>>LEVEL TWO --- SHIP<<<");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 3 * SCREEN_HEIGHT / optionsCount, text, charset);

        sprintf(text, "LEVEL THREE --- SHOTGUN FORT");
        if (selected == LEVEL_THREE)
            sprintf(text, ">>>LEVEL THREE --- SHOTGUN FORT<<<");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 3.5 * SCREEN_HEIGHT / optionsCount, text, charset);

        sprintf(text, "EXIT");
        if (selected == QUIT_TEST)
            sprintf(text, ">>>EXIT<<<");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 4 * SCREEN_HEIGHT / optionsCount, text, charset);
        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        //move cursor. selected is incremented/decremented outside while loop to prevent accidental holding key for too long
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_UP)
                    up = TRUE;
                if (event.key.keysym.sym == SDLK_DOWN)
                    down = TRUE;
                if (event.key.keysym.sym == SDLK_SPACE)
                    return selected;
            }
        }
        if (down == TRUE) {
            selected++;
            down = FALSE;
        }
        if (up == TRUE) {
            selected--;
            up = FALSE;
        }
        //loop through menu
        if (selected > QUIT_TEST)
            selected = LEVEL_ONE;
        if (selected < LEVEL_ONE)
            selected = QUIT_TEST;
    }
}

int mainMenu(SDL_Surface* screen, int color, SDL_Surface* charset, SDL_Texture* scrtex, SDL_Renderer* renderer)
{
    int selected = TESTING_LEVELS, down = FALSE, up = FALSE;
    const int optionsCount = 5;
    char text[TEXT_SIZE];
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
    SDL_Event event;
    while (TRUE)
    {
        SDL_FillRect(screen, NULL, color);
        sprintf(text, "CONFIRM WITH SPACE");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 1 * SCREEN_HEIGHT / optionsCount, text, charset);

        sprintf(text, "TESTING LEVELS");
        if (selected == TESTING_LEVELS)
            sprintf(text, ">>>TESTING LEVELS<<<");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 2.5 * SCREEN_HEIGHT / optionsCount, text, charset);

        sprintf(text, "NORMAL LEVEL");
        if (selected == NORMAL_LEVEL)
            sprintf(text, ">>>NORMAL LEVEL<<<");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 3 * SCREEN_HEIGHT / optionsCount, text, charset);

        sprintf(text, "EXIT");
        if (selected == QUIT)
            sprintf(text, ">>>EXIT<<<");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 3.5 * SCREEN_HEIGHT / optionsCount, text, charset);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        //move cursor. selected is incremented/decremented outside while loop to prevent accidental holding key for too long
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_UP)
                    up = TRUE;
                if (event.key.keysym.sym == SDLK_DOWN)
                    down = TRUE;
                if (event.key.keysym.sym == SDLK_SPACE && selected != TESTING_LEVELS)
                    return selected;
                else if (event.key.keysym.sym == SDLK_SPACE && selected == TESTING_LEVELS)
                    return testLevelsMenu(screen, color, charset, scrtex, renderer);
            }
        }
        if (down == TRUE) {
            selected++;
            down = FALSE;
        }
        if (up == TRUE) {
            selected--;
            up = FALSE;
        }

        //loop through menu
        if (selected > QUIT)
            selected = TESTING_LEVELS;
        if (selected < TESTING_LEVELS)
            selected = QUIT;
    }
    return 1;
}

void hitboxes(enemy_t* fort, enemy_t* enemyShip, enemy_t* shotgun, object_t cameraObj, bullet_t* allBullets)
{
    SDL_Rect fortHitbox[FORT_COUNT], shotgunHitbox[SHOTGUN_COUNT], enemyShipHitbox[ENEMY_SHIP_COUNT], fortCannonballs[BULLET_COUNT_FORT], shotgunCannonbals[BULLET_COUNT_SHOTGUN], enemyShipCannonballs[BULLET_COUNT_ENEMY_SHIP];
    int sharedIterator = 0;

    for (int i = 0; i < FORT_COUNT; i++)
    {
        fortHitbox[i] = { (int)(fort[i].pos.x - cameraObj.x) - FORT_SIZE / 2, (int)(fort[i].pos.y - cameraObj.y) - FORT_SIZE / 2, FORT_SIZE, FORT_SIZE };
        fort[i].hitbox = &fortHitbox[i];
        for (int j = 0; j < BULLET_COUNT_FORT; j++) {
            if (fort[i].bulletsFort[j].alive == TRUE)
            {
                fortCannonballs[j] = { (int)(fort[i].bulletsFort[j].bulletInfo.x - cameraObj.x + fort[i].pos.x) - CANNONBALL_SIZE / 2, (int)(fort[i].bulletsFort[j].bulletInfo.y - cameraObj.y + fort[i].pos.y) - CANNONBALL_SIZE / 2, CANNONBALL_SIZE, CANNONBALL_SIZE };
                fort[i].bulletsFort[j].hitbox = &fortCannonballs[j];
                if (j == 10) continue;
                allBullets[j].hitbox = &fortCannonballs[j];
                allBullets[j].alive = TRUE;
            }
        }
    }
    for (int i = 0; i < ENEMY_SHIP_COUNT; i++)
    {
        enemyShipHitbox[i] = { (int)(enemyShip[i].pos.x - cameraObj.x) - ENEMY_SHIP_X / 2, (int)(enemyShip[i].pos.y - cameraObj.y) - ENEMY_SHIP_Y / 2, ENEMY_SHIP_X, ENEMY_SHIP_Y };
        enemyShip[i].hitbox = &enemyShipHitbox[i];
        for (int j = 0; j < BULLET_COUNT_ENEMY_SHIP; j++) {
            if (enemyShip[i].bulletsShip[j].alive == TRUE)
            {
                enemyShipCannonballs[j] = { (int)(enemyShip[i].bulletsShip[j].bulletInfo.x - cameraObj.x + enemyShip[i].pos.x) - CANNONBALL_SIZE / 2, (int)(enemyShip[i].bulletsShip[j].bulletInfo.y - cameraObj.y + enemyShip[i].pos.y) - CANNONBALL_SIZE / 2, CANNONBALL_SIZE, CANNONBALL_SIZE };
                enemyShip[i].bulletsShip[j].hitbox = &enemyShipCannonballs[j];
                allBullets[j + BULLET_COUNT_FORT * FORT_COUNT].hitbox = &enemyShipCannonballs[j];
                allBullets[j + BULLET_COUNT_FORT * FORT_COUNT].alive = TRUE;
            }
        }
    }
    for (int i = 0; i < SHOTGUN_COUNT; i++)
    {
        shotgunHitbox[i] = { (int)(shotgun[i].pos.x - cameraObj.x) - FORT_SIZE / 2, (int)(shotgun[i].pos.y - cameraObj.y) - FORT_SIZE / 2, FORT_SIZE, FORT_SIZE };
        shotgun[i].hitbox = &shotgunHitbox[i];
        for (int j = 0; j < BULLET_COUNT_SHOTGUN; j++) {
            if (shotgun[i].bulletsShotgun[j].alive == TRUE)
            {
                shotgunCannonbals[j] = { (int)(shotgun[i].bulletsShotgun[j].bulletInfo.x - cameraObj.x + shotgun[i].pos.x) - CANNONBALL_SIZE / 2, (int)(shotgun[i].bulletsShotgun[j].bulletInfo.y - cameraObj.y + shotgun[i].pos.y) - CANNONBALL_SIZE / 2, CANNONBALL_SIZE, CANNONBALL_SIZE };
                shotgun[i].bulletsShotgun[j].hitbox = &shotgunCannonbals[j];
                allBullets[j + BULLET_COUNT_ENEMY_SHIP * ENEMY_SHIP_COUNT + BULLET_COUNT_FORT * FORT_COUNT].hitbox = &shotgunCannonbals[j];
                allBullets[j + BULLET_COUNT_ENEMY_SHIP * ENEMY_SHIP_COUNT + BULLET_COUNT_FORT * FORT_COUNT].alive = TRUE;
            }
        }
    }
}

void collisionDetector(player_t& player, bullet_t* allBullets)
{
    for (int i = 0; i < BULLET_COUNT_FORT * FORT_COUNT + BULLET_COUNT_ENEMY_SHIP * ENEMY_SHIP_COUNT + SHOTGUN_COUNT * BULLET_COUNT_SHOTGUN; i++) {
        if (allBullets[i].alive == TRUE) {
            if (SDL_HasIntersection(player.hitbox, allBullets[i].hitbox) == SDL_TRUE && player.invincibility == FALSE) {
                printf("%d\n", i);
                player.hp--;
                player.invincibility = TRUE;
                player.lastInvincibilityTime = SDL_GetTicks();
            }
        }
    }
}

void cancelInvincibility(player_t& player)
{
    if (SDL_GetTicks() - player.lastInvincibilityTime > INVINCIBILITY_TICKS)
    {
        player.invincibility = FALSE;
    }
}

void timer(unsigned int& t2, double& delta, unsigned int& t1, int& frames, double& fps, double& fpsTimer, double& worldTime)
{
    //delta is current time (t2-t1) in seconds 
    t2 = SDL_GetTicks();
    delta = (t2 - t1) * 0.001;
    t1 = t2;
    worldTime += delta;

    fpsTimer += delta;
    if (fpsTimer > 0.5) {
        fps = frames * 2;
        frames = 0;
        fpsTimer -= 0.5;
    };
}

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
    unsigned int t1, t2;
    int frames = 0, rc = 0, level = LEVEL_ONE, afterNewGame = FALSE, choice;
    double delta, worldTime = 0, fpsTimer = 0, fps = 0, speedVertical = 0, speedHorizontal = 0;
    SDL_Event event;
    SDL_Surface* screen = NULL, * charset = NULL, * cannonball = NULL, * map = NULL;
    SDL_Texture* scrtex = NULL;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    bullet_t allEnemyBullets[BULLET_COUNT_FORT * FORT_COUNT + BULLET_COUNT_ENEMY_SHIP * ENEMY_SHIP_COUNT + SHOTGUN_COUNT * BULLET_COUNT_SHOTGUN];
    object_t cameraObj = { START_POS_X,START_POS_Y }; //center player on map  
    enemy_t enemyShip[ENEMY_SHIP_COUNT], fort[FORT_COUNT], shotgun[SHOTGUN_COUNT];
    player_t player;
    SDL_Rect playerHitbox = { SCREEN_WIDTH / 2 - PLAYER_SIZE_X, SCREEN_HEIGHT / 2 - PLAYER_SIZE_Y, PLAYER_SIZE_X * 2, 2 * PLAYER_SIZE_Y };
    player.hitbox = &playerHitbox;
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

    prepareGame(&scrtex, &window, &renderer, &charset, enemyShip, &player.surface, &screen, fort, &cannonball, rc, t1, &map, shotgun);
    int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    int gray = SDL_MapRGB(screen->format, 72, 72, 72);
    int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    int seablue = SDL_MapRGB(screen->format, 0, 112, 232);
    int lightgreen = SDL_MapRGB(screen->format, 118, 215, 196);

    level = mainMenu(screen, gray, charset, scrtex, renderer);

    chooseLevel(level, fort, enemyShip, shotgun, cameraObj);
    while (level != QUIT) {
        timer(t2, delta, t1, frames, fps, fpsTimer, worldTime);
        drawGame(charset, player.surface, screen, map, fort, enemyShip, shotgun, cameraObj, lightgreen, worldTime, fps, player.hp, red, player, gray);
        if (player.hp <= 0)
        {
            choice = endGame(gray, screen, charset, cameraObj, scrtex, renderer);
            if (choice == NEW_GAME) {
                newGame(cameraObj, enemyShip, fort, shotgun, level, player);
                afterNewGame = TRUE;
            }
            if (choice == QUIT) {
                level = mainMenu(screen, gray, charset, scrtex, renderer);
                newGame(cameraObj, enemyShip, fort, shotgun, level, player);
            }
        }

        if (level == NORMAL_LEVEL)
        {
            for (int i = 0; i < FORT_COUNT; i++)
                fortShoot(cameraObj, delta, screen, &fort[i]);
            for (int i = 0; i < ENEMY_SHIP_COUNT; i++)
                enemyShipShoot(cameraObj, delta, screen, &enemyShip[i]);
            for (int i = 0; i < SHOTGUN_COUNT; i++)
                shotgunShoot(cameraObj, delta, screen, &shotgun[i]);
        }
        else if (level == LEVEL_ONE)
            fortShoot(cameraObj, delta, screen, &fort[0]);
        else if (level == LEVEL_TWO)
            enemyShipShoot(cameraObj, delta, screen, &enemyShip[0]);
        else if (level == LEVEL_THREE)
            shotgunShoot(cameraObj, delta, screen, &shotgun[0]);

        enemyMove(enemyShip, enemyShip[0].speed, 0, delta, afterNewGame);
        hitboxes(fort, enemyShip, shotgun, cameraObj, allEnemyBullets);

        cancelInvincibility(player);
        collisionDetector(player, allEnemyBullets);
        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_PumpEvents();
        if (keyboardState[SDL_SCANCODE_UP])         speedVertical = -PLAYER_SPEED;
        if (keyboardState[SDL_SCANCODE_DOWN])   speedVertical = PLAYER_SPEED;
        if (keyboardState[SDL_SCANCODE_RIGHT])  speedHorizontal = PLAYER_SPEED;
        if (keyboardState[SDL_SCANCODE_LEFT])   speedHorizontal = -PLAYER_SPEED;
        if (keyboardState[SDL_SCANCODE_ESCAPE]) level = QUIT;
        if (keyboardState[SDL_SCANCODE_N]) {
            newGame(cameraObj, enemyShip, fort, shotgun, level, player);
            afterNewGame = TRUE;
        }
        moveObject(cameraObj, speedVertical, speedHorizontal, delta);

        //check if outside of map
        if (cameraObj.x > LEVEL_SIZE / 2 - PLAYER_SIZE_X || cameraObj.x < -LEVEL_SIZE / 2 + PLAYER_SIZE_X || cameraObj.y > LEVEL_SIZE / 2 - PLAYER_SIZE_Y || cameraObj.y < -LEVEL_SIZE / 2 + PLAYER_SIZE_Y)
        {
            moveObject(cameraObj, -speedVertical, -speedHorizontal, delta);
        }
        speedHorizontal = 0;
        speedVertical = 0;
        frames++;
    };
    freeAndQuit(scrtex, window, renderer, charset, player.surface, screen, fort->surface, map, enemyShip->surface, shotgun->surface);
    return 0;
};