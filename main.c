#include "raylib.h"
#include "raymath.h"
#include <math.h> // sinf, cosf, fabsf 등을 위한 헤더

//----------------------------------------------------------------------------------
// 물리 엔진 상수 정의 (차분한 이동을 위해 속도 튜닝 완료)
//----------------------------------------------------------------------------------
#define GRAVITY         32.0f
#define MAX_SPEED       12.0f  
#define CROUCH_SPEED     6.0f
#define JUMP_FORCE      12.0f
#define MAX_ACCEL       90.0f  
#define FRICTION         0.86f
#define AIR_DRAG         0.98f
#define CONTROL          15.0f
#define CROUCH_HEIGHT    0.0f
#define STAND_HEIGHT     1.2f  
#define BOTTOM_HEIGHT    0.6f

// 맵 기믹을 위한 타일 상수 정의
#define TILE_EMPTY   0  
#define TILE_WALL    1  
#define TILE_ENEMY   2  
#define TILE_ITEM    3  
#define TILE_SHUTTER 4  
#define TILE_GOAL    9  

#define NORMALIZE_INPUT  0

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

// ----------------------------------------------------------------------------------
// 수제 미로 맵 크기 정의 및 공간 배율 세팅
// ----------------------------------------------------------------------------------
#define MAP_WIDTH  30  
#define MAP_HEIGHT 20  
#define WORLD_SCALE 2.5f 

extern int myNewMap[MAP_HEIGHT][MAP_WIDTH];

static Vector3 mapPosition = { -((MAP_WIDTH * WORLD_SCALE) / 2.0f), 0.0f, -((MAP_HEIGHT * WORLD_SCALE) / 2.0f) };

// 전역 변수들
static Body player = { 0 };
static Vector2 lookRotation = { 0 }; 
static float headTimer = 0.0f;
static float walkLerp = 0.0f;
static float headLerp = STAND_HEIGHT;
static Vector2 lean = { 0 };

// 💡 셔터 자동 개폐 및 1.5초 타이머 제어 변수들
static bool isShutterOpen = false;    
static float shutterOpenTimer = 0.0f; 
static float shutterHoldTimer = 0.0f; // 💡 완전히 열린 후 버틴 시간을 재는 타이머

// 함수 선언
static bool CheckMapCollision(Vector3 testPos, float radius);
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold);
static void UpdateCameraFPS(Camera *camera);

//----------------------------------------------------------------------------------
// 메인 프로그램
//----------------------------------------------------------------------------------
int main(void)
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    InitWindow(screenWidth, screenHeight, "raylib - 3D Maze Auto-Close Shutter Game");

    player.position = (Vector3){ mapPosition.x + (1.5f * WORLD_SCALE), 0.0f, mapPosition.z + (1.5f * WORLD_SCALE) }; 

    Camera camera = { 0 };
    camera.fovy = 85.0f; 
    camera.projection = CAMERA_PERSPECTIVE;
    
    DisableCursor(); 
    SetTargetFPS(60);

    while (!WindowShouldClose())    
    {
        // -------------------------------------------------------------------------
        // 1. 입력 및 물리 엔진 업데이트
        // -------------------------------------------------------------------------
        float delta = GetFrameTime();

        Vector2 mouseDelta = GetMouseDelta();
        float hybridSensitivity = 0.0007f; 
        
        lookRotation.x -= mouseDelta.x * hybridSensitivity;
        lookRotation.y -= mouseDelta.y * hybridSensitivity;
        lookRotation.y = Clamp(lookRotation.y, -89.0f * DEG2RAD, 89.0f * DEG2RAD);

        char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        bool crouching = IsKeyDown(KEY_LEFT_CONTROL);

        UpdateBody(&player, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE), crouching);

        // -------------------------------------------------------------------------
        // 2. 다이내믹 카메라 연출
        // -------------------------------------------------------------------------
        headLerp = Lerp(headLerp, (crouching ? CROUCH_HEIGHT : STAND_HEIGHT), 20.0f * delta);
        
        camera.position = (Vector3){
            player.position.x,
            player.position.y + (BOTTOM_HEIGHT + headLerp),
            player.position.z,
        };

        if (player.isGrounded && ((forward != 0) || (sideway != 0)))
        {
            headTimer += delta * 3.0f;
            walkLerp = Lerp(walkLerp, 1.0f, 10.0f * delta);
            camera.fovy = Lerp(camera.fovy, 80.0f, 5.0f * delta); 
        }
        else
        {
            walkLerp = Lerp(walkLerp, 0.0f, 10.0f * delta);
            camera.fovy = Lerp(camera.fovy, 85.0f, 5.0f * delta); 
        }

        lean.x = Lerp(lean.x, sideway * 0.02f, 10.0f * delta); 
        lean.y = Lerp(lean.y, forward * 0.015f, 10.0f * delta);

        UpdateCameraFPS(&camera); 

        // -------------------------------------------------------------------------
        // 🌟 [수정] 방화 셔터 상호작용 및 1.5초 자동 닫힘 타이머 로직
        // -------------------------------------------------------------------------
        if (IsKeyPressed(KEY_E)) 
        {
            isShutterOpen = !isShutterOpen;
            if (isShutterOpen) shutterHoldTimer = 0.0f; // 열기 시작할 때 대기 시간 초기화
        }

        if (isShutterOpen) 
        {
            // 문이 위로 열리는 애니메이션 (배율 공간에 맞춰 속도 1.5f 적용)
            if (shutterOpenTimer < 1.0f) 
            {
                shutterOpenTimer += delta * 1.5f;
                if (shutterOpenTimer > 1.0f) shutterOpenTimer = 1.0f;
            }
            // 💡 문이 완벽히 끝까지 열렸다면(shutterOpenTimer == 1.0f), 1.5초 동안 버팁니다.
            else 
            {
                shutterHoldTimer += delta;
                if (shutterHoldTimer >= 1.5f) 
                {
                    isShutterOpen = false; // 1.5초가 지나면 자동으로 닫힘 상태로 강제 전환!
                }
            }
        } 
        else 
        {
            // 문이 아래로 닫히는 애니메이션
            if (shutterOpenTimer > 0.0f) 
            {
                shutterOpenTimer -= delta * 1.5f;
                if (shutterOpenTimer < 0.0f) shutterOpenTimer = 0.0f;
            }
        }

        // -------------------------------------------------------------------------
        // 3. 화면 그리기
        // -------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE); 

            BeginMode3D(camera);
                
                // 바닥 격자판
                DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ MAP_WIDTH * WORLD_SCALE, MAP_HEIGHT * WORLD_SCALE }, DARKGRAY);

                // 3D 오브젝트 배치 및 렌더링
                for (int y = 0; y < MAP_HEIGHT; y++) 
                {
                    for (int x = 0; x < MAP_WIDTH; x++) 
                    {
                        Vector3 wallPos = { 
                            mapPosition.x + x * WORLD_SCALE + (WORLD_SCALE / 2.0f), 
                            mapPosition.y, 
                            mapPosition.z + y * WORLD_SCALE + (WORLD_SCALE / 2.0f) 
                        };

                        switch (myNewMap[y][x])
                        {
                            case TILE_WALL: 
                                wallPos.y = mapPosition.y + (3.5f * WORLD_SCALE / 2.0f); 
                                DrawCube(wallPos, WORLD_SCALE, 3.5f * WORLD_SCALE, WORLD_SCALE, GRAY);              
                                DrawCubeWires(wallPos, WORLD_SCALE, 3.5f * WORLD_SCALE, WORLD_SCALE, DARKGRAY);     
                                break;

                            case TILE_ENEMY: 
                                wallPos.y = mapPosition.y + 1.5f;
                                DrawCylinder(wallPos, 0.5f, 0.5f, 3.0f, 8, PURPLE);
                                DrawCylinderWires(wallPos, 0.5f, 0.5f, 3.0f, 8, DARKPURPLE);
                                break;

                            case TILE_ITEM: 
                                wallPos.y = mapPosition.y + 0.6f;
                                DrawCube(wallPos, 0.8f, 0.8f, 0.8f, YELLOW);              
                                DrawCubeWires(wallPos, 0.8f, 0.8f, 0.8f, GOLD);     
                                break;

                            case TILE_SHUTTER: 
                                wallPos.y = mapPosition.y + (3.5f * WORLD_SCALE / 2.0f) + (shutterOpenTimer * 4.5f * WORLD_SCALE); 
                                
                                float shutterWidthX = WORLD_SCALE * 0.96f;
                                float shutterDepthZ = 0.25f;

                                if (y > 0 && y < MAP_HEIGHT - 1) {
                                    if (myNewMap[y-1][x] == TILE_WALL || myNewMap[y+1][x] == TILE_WALL) {
                                        shutterWidthX = 0.25f; 
                                        shutterDepthZ = WORLD_SCALE * 0.96f; 
                                    }
                                }

                                // 방화 셔터 본체 그리기
                                DrawCube(wallPos, shutterWidthX, 3.5f * WORLD_SCALE, shutterDepthZ, BROWN);            
                                DrawCubeWires(wallPos, shutterWidthX, 3.5f * WORLD_SCALE, shutterDepthZ, DARKBROWN);

                                // 💡 [수정] 문고리(DrawSphere) 코드 무조건 없앰! 깔끔한 평면 장벽이 되었습니다.
                                break;

                            case TILE_GOAL: 
                                wallPos.y = mapPosition.y + 0.05f;
                                DrawCube(wallPos, WORLD_SCALE, 0.1f, WORLD_SCALE, LIME);              
                                break;
                        }
                    }
                }

            EndMode3D();

            // FHD 미니맵 레이더 렌더링
            int scale = 12; 
            int minimapX = GetScreenWidth() - (MAP_WIDTH * scale) - 30;
            int minimapY = 30;

            DrawRectangle(minimapX, minimapY, MAP_WIDTH * scale, MAP_HEIGHT * scale, Fade(BLACK, 0.5f));
            
            for (int y = 0; y < MAP_HEIGHT; y++) {
                for (int x = 0; x < MAP_WIDTH; x++) {
                    int tile = myNewMap[y][x];
                    if (tile == TILE_WALL) DrawRectangle(minimapX + x * scale, minimapY + y * scale, scale, scale, DARKGRAY);
                    else if (tile == TILE_SHUTTER) DrawRectangle(minimapX + x * scale, minimapY + y * scale, scale, scale, BROWN);
                    else if (tile == TILE_ITEM) DrawRectangle(minimapX + x * scale, minimapY + y * scale, scale, scale, YELLOW);
                    else if (tile == TILE_ENEMY) DrawRectangle(minimapX + x * scale, minimapY + y * scale, scale, scale, PURPLE); 
                    else if (tile == TILE_GOAL) DrawRectangle(minimapX + x * scale, minimapY + y * scale, scale, scale, LIME);   
                }
            }
            DrawRectangleLines(minimapX, minimapY, MAP_WIDTH * scale, MAP_HEIGHT * scale, GREEN); 

            int playerCellX = (int)((player.position.x - mapPosition.x) / WORLD_SCALE);
            int playerCellY = (int)((player.position.z - mapPosition.z) / WORLD_SCALE);
            if (playerCellX >= 0 && playerCellX < MAP_WIDTH && playerCellY >= 0 && playerCellY < MAP_HEIGHT) {
                DrawRectangle(minimapX + playerCellX * scale, minimapY + playerCellY * scale, scale, scale, GREEN);
            }

            // 가이드 UI
            DrawRectangle(5, 5, 480, 110, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines(5, 5, 480, 110, BLUE);
            DrawText("Controls (Full Mouse FPS Mode):", 15, 15, 12, BLACK);
            DrawText("- Move keys: W/S (Forward/Backward), A/D (Strafe Left/Right)", 15, 32, 12, BLACK);
            DrawText("- Mouse: Look EVERYWHERE (Up, Down, Left, Right)", 15, 49, 12, BLACK);
            DrawText("- Interaction: Press [E] key to Open Shutters!", 15, 66, 12, MAROON);
            DrawText("🌟 System: Shutters will AUTO-CLOSE after 1.5 seconds!", 15, 83, 12, MAROON);
            if (isShutterOpen && shutterOpenTimer >= 1.0f) {
                DrawText(TextFormat("Closing in: %.1f sec", 1.5f - shutterHoldTimer), 15, 100, 12, RED);
            }

            DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();                  
    return 0;
}

// =================================================================================
// 충돌 검사 전용 함수
// =================================================================================
static bool CheckMapCollision(Vector3 testPos, float radius)
{
    Vector2 pos2D = { testPos.x, testPos.z };
    int cellX = (int)((testPos.x - mapPosition.x) / WORLD_SCALE);
    int cellY = (int)((testPos.z - mapPosition.z) / WORLD_SCALE);

    for (int y = cellY - 1; y <= cellY + 1; y++) {
        if (y >= 0 && y < MAP_HEIGHT) {
            for (int x = cellX - 1; x <= cellX + 1; x++) {
                if (x >= 0 && x < MAP_WIDTH) {
                    int tileType = myNewMap[y][x];

                    if (tileType == TILE_WALL || (tileType == TILE_SHUTTER && shutterOpenTimer < 0.8f)) { 
                        Rectangle wallRect = { 
                            mapPosition.x + x * WORLD_SCALE, 
                            mapPosition.z + y * WORLD_SCALE, 
                            WORLD_SCALE, WORLD_SCALE 
                        };
                        if (CheckCollisionCircleRec(pos2D, radius, wallRect)) {
                            return true; 
                        }
                    }
                }
            }
        }
    }
    return false; 
}

// =================================================================================
// 물리 모듈
// =================================================================================
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold)
{
    Vector2 input = (Vector2){ (float)side, (float)-forward };
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input);

    float delta = GetFrameTime();

    if (!body->isGrounded) body->velocity.y -= GRAVITY * delta;

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;
    }

    Vector3 front = (Vector3){ sinf(rot), 0.f, cosf(rot) };
    Vector3 right = (Vector3){ cosf(-rot), 0.f, sinf(-rot) };
    Vector3 desiredDir = (Vector3){ input.x * right.x + input.y * front.x, 0.0f, input.x * right.z + input.y * front.z, };
    
    body->dir = Vector3Lerp(body->dir, desiredDir, CONTROL * delta);

    float decel = (body->isGrounded ? FRICTION : AIR_DRAG);
    Vector3 hvel = (Vector3){ body->velocity.x * decel, 0.0f, body->velocity.z * decel };

    float hvelLength = Vector3Length(hvel);
    if (hvelLength < (MAX_SPEED * 0.01f)) hvel = (Vector3){ 0 };

    float speed = Vector3DotProduct(hvel, body->dir);
    float maxSpeed = (crouchHold ? CROUCH_SPEED : MAX_SPEED);
    float accel = Clamp(maxSpeed - speed, 0.f, MAX_ACCEL * delta);
    hvel.x += body->dir.x * accel;
    hvel.z += body->dir.z * accel;

    body->velocity.x = hvel.x;
    body->velocity.z = hvel.z;

    float playerRadius = 0.2f; 

    // X축 충돌 처리
    Vector3 nextPosX = body->position;
    nextPosX.x += body->velocity.x * delta;
    if (!CheckMapCollision(nextPosX, playerRadius)) {
        body->position.x = nextPosX.x; 
    } else {
        body->velocity.x = 0.0f;       
    }

    // Z축 충돌 처리
    Vector3 nextPosZ = body->position;
    nextPosZ.z += body->velocity.z * delta;
    if (!CheckMapCollision(nextPosZ, playerRadius)) {
        body->position.z = nextPosZ.z; 
    } else {
        body->velocity.z = 0.0f;       
    }

    // Y축 중력 처리
    body->position.y += body->velocity.y * delta;
    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true; 
    }
}

// =================================================================================
// 카메라 모듈
// =================================================================================
static void UpdateCameraFPS(Camera *camera)
{
    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };

    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    float pitchAngle = lookRotation.y - lean.y; 
    pitchAngle = Clamp(pitchAngle, -89.0f * DEG2RAD, 89.0f * DEG2RAD); 
    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    float headSin = sinf(headTimer * PI);
    float headCos = cosf(headTimer * PI);
    const float stepRotation = 0.01f;
    camera->up = Vector3RotateByAxisAngle(up, pitch, headSin * stepRotation + lean.x);

    const float bobSide = 0.05f;
    const float bobUp = 0.08f;
    Vector3 bobbing = Vector3Scale(right, headSin * bobSide);
    bobbing.y = fabsf(headCos * bobUp);

    camera->position = Vector3Add(camera->position, Vector3Scale(bobbing, walkLerp));
    camera->target = Vector3Add(camera->position, pitch);
}