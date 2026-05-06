/*******************************************************************************************
*
* raylib [core] example - 3d camera fps (100% Code Only, Wall Sliding Collision Fix)
*
* - 외부 이미지 파일(png) 완벽 제거 (100% 코드 전용)
* - 2차원 숫자 배열(myMap)을 이용한 커스텀 맵 시스템
* - [핵심 수정] X축과 Z축 충돌을 따로 계산하여 벽에 끼지 않고 매끄럽게 미끄러지게(Slide) 함!
* - 헤드 보빙(Bobbing) 및 뷰 기울임(Lean) 연출 완벽 복구
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

//----------------------------------------------------------------------------------
// 물리 엔진 상수 정의
//----------------------------------------------------------------------------------
#define GRAVITY         32.0f
#define MAX_SPEED       20.0f
#define CROUCH_SPEED     5.0f
#define JUMP_FORCE      12.0f
#define MAX_ACCEL      150.0f
#define FRICTION         0.86f
#define AIR_DRAG         0.98f
#define CONTROL         15.0f
#define CROUCH_HEIGHT    0.0f
#define STAND_HEIGHT     1.0f
#define BOTTOM_HEIGHT    0.5f

#define NORMALIZE_INPUT  0

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

// ----------------------------------------------------------------------------------
// 수제 미로 맵 (16 x 16 크기)
// ----------------------------------------------------------------------------------
#define MAP_WIDTH 16
#define MAP_HEIGHT 16

static int myMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,1,1,1,1,1,0,1},
    {1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,1,1,1,1,1,1,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,0,1,1,1,1,0,1},
    {1,0,0,0,1,0,1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,1,1,1,0,1,1,0,1},
    {1,0,1,0,1,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static Vector3 mapPosition = { -8.0f, 0.0f, -8.0f };  

// 전역 변수들
static Vector2 sensitivity = { 0.001f, 0.001f };
static Body player = { 0 };
static Vector2 lookRotation = { 0 };
static float headTimer = 0.0f;
static float walkLerp = 0.0f;
static float headLerp = STAND_HEIGHT;
static Vector2 lean = { 0 };

// 함수 선언
static bool CheckMapCollision(Vector3 testPos, float radius);
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold);
static void UpdateCameraFPS(Camera *camera);

//----------------------------------------------------------------------------------
// 메인 프로그램
//----------------------------------------------------------------------------------
int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "raylib - Smooth Wall Sliding & Head Bobbing");

    player.position = (Vector3){ mapPosition.x + 1.0f, 0.0f, mapPosition.z + 1.0f }; 

    Camera camera = { 0 };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    DisableCursor(); 
    SetTargetFPS(60);

    while (!WindowShouldClose())    
    {
        // -------------------------------------------------------------------------
        // 1. 입력 및 물리 엔진 업데이트
        // -------------------------------------------------------------------------
        Vector2 mouseDelta = GetMouseDelta();
        lookRotation.x -= mouseDelta.x * sensitivity.x;
        lookRotation.y += mouseDelta.y * sensitivity.y;

        char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        bool crouching = IsKeyDown(KEY_LEFT_CONTROL);

        // UpdateBody 안에서 X/Z축 이동과 스무스한 충돌 처리를 모두 완료합니다.
        UpdateBody(&player, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE), crouching);

        // -------------------------------------------------------------------------
        // 2. 다이내믹 카메라 연출 (Head Bobbing & Lean 부활!)
        // -------------------------------------------------------------------------
        float delta = GetFrameTime();
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
            camera.fovy = Lerp(camera.fovy, 55.0f, 5.0f * delta); // 시야 좁아짐 연출
        }
        else
        {
            walkLerp = Lerp(walkLerp, 0.0f, 10.0f * delta);
            camera.fovy = Lerp(camera.fovy, 60.0f, 5.0f * delta);
        }

        lean.x = Lerp(lean.x, sideway * 0.02f, 10.0f * delta); // 좌우 몸 기울임 연출
        lean.y = Lerp(lean.y, forward * 0.015f, 10.0f * delta);

        UpdateCameraFPS(&camera); 

        // -------------------------------------------------------------------------
        // 3. 화면 그리기
        // -------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE); 

            BeginMode3D(camera);
                
                DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ MAP_WIDTH * 1.0f, MAP_HEIGHT * 1.0f }, DARKGRAY);

                for (int y = 0; y < MAP_HEIGHT; y++) 
                {
                    for (int x = 0; x < MAP_WIDTH; x++) 
                    {
                        if (myMap[y][x] == 1) 
                        {
                            Vector3 wallPos = { mapPosition.x + x * 1.0f, mapPosition.y + 1.5f, mapPosition.z + y * 1.0f };
                            DrawCube(wallPos, 1.0f, 3.0f, 1.0f, RED);               
                            DrawCubeWires(wallPos, 1.0f, 3.0f, 1.0f, MAROON);     
                        }
                    }
                }

            EndMode3D();

            // 2D 미니맵 레이더
            int scale = 6; 
            int minimapX = GetScreenWidth() - (MAP_WIDTH * scale) - 20;
            int minimapY = 20;

            DrawRectangle(minimapX, minimapY, MAP_WIDTH * scale, MAP_HEIGHT * scale, Fade(BLACK, 0.5f));
            
            for (int y = 0; y < MAP_HEIGHT; y++) {
                for (int x = 0; x < MAP_WIDTH; x++) {
                    if (myMap[y][x] == 1) {
                        DrawRectangle(minimapX + x * scale, minimapY + y * scale, scale, scale, RED);
                    }
                }
            }
            DrawRectangleLines(minimapX, minimapY, MAP_WIDTH * scale, MAP_HEIGHT * scale, GREEN); 

            // 미니맵 위 플레이어 위치
            int playerCellX = (int)(player.position.x - mapPosition.x + 0.5f);
            int playerCellY = (int)(player.position.z - mapPosition.z + 0.5f);
            DrawRectangle(minimapX + playerCellX * scale, minimapY + playerCellY * scale, scale, scale, GREEN);

            DrawRectangle(5, 5, 330, 75, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines(5, 5, 330, 75, BLUE);
            DrawText("Camera controls:", 15, 15, 10, BLACK);
            DrawText("- Move keys: W, A, S, D, Space, Left-Ctrl", 15, 30, 10, BLACK);
            DrawText("- Look around: arrow keys or mouse", 15, 45, 10, BLACK);
            DrawText(TextFormat("- Velocity Len: (%06.3f)", Vector2Length((Vector2){ player.velocity.x, player.velocity.z })), 15, 60, 10, BLACK);

            DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();                  
    return 0;
}

// =================================================================================
// [핵심 추가] 충돌 검사 전용 함수 
// 지정한 위치(testPos)가 벽과 겹치는지 Yes/No만 반환합니다.
// =================================================================================
static bool CheckMapCollision(Vector3 testPos, float radius)
{
    Vector2 pos2D = { testPos.x, testPos.z };
    
    int cellX = (int)(testPos.x - mapPosition.x + 0.5f);
    int cellY = (int)(testPos.z - mapPosition.z + 0.5f);

    for (int y = cellY - 1; y <= cellY + 1; y++) {
        if (y >= 0 && y < MAP_HEIGHT) {
            for (int x = cellX - 1; x <= cellX + 1; x++) {
                if (x >= 0 && x < MAP_WIDTH) {
                    if (myMap[y][x] == 1) { // 벽이 있다면
                        Rectangle wallRect = { 
                            mapPosition.x - 0.5f + x * 1.0f, 
                            mapPosition.z - 0.5f + y * 1.0f, 
                            1.0f, 1.0f 
                        };
                        if (CheckCollisionCircleRec(pos2D, radius, wallRect)) {
                            return true; // 부딪힘!
                        }
                    }
                }
            }
        }
    }
    return false; // 안 부딪힘!
}

// =================================================================================
// 물리 모듈: 스무스한 벽 타기(Sliding) 적용
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

    // -------------------------------------------------------------------------
    // [해결의 핵심] 축 분리 충돌 처리 (Axis-Separation Collision Resolution)
    // -------------------------------------------------------------------------
    float playerRadius = 0.2f;

    // 1. X축 이동 시도
    Vector3 nextPosX = body->position;
    nextPosX.x += body->velocity.x * delta;
    
    if (!CheckMapCollision(nextPosX, playerRadius)) {
        body->position.x = nextPosX.x; // 벽이 없으면 X축 전진
    } else {
        body->velocity.x = 0.0f;       // 막혔다면 X축 속도만 죽임
    }

    // 2. Z축 이동 시도
    Vector3 nextPosZ = body->position;
    nextPosZ.z += body->velocity.z * delta;

    if (!CheckMapCollision(nextPosZ, playerRadius)) {
        body->position.z = nextPosZ.z; // 벽이 없으면 Z축 전진
    } else {
        body->velocity.z = 0.0f;       // 막혔다면 Z축 속도만 죽임
    }

    // 3. Y축(점프/중력) 처리
    body->position.y += body->velocity.y * delta;
    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true; 
    }
}

// =================================================================================
// 카메라 모듈 (역동적 연출 Bobbing 부활)
// =================================================================================
static void UpdateCameraFPS(Camera *camera)
{
    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };

    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    float maxAngleUp = Vector3Angle(up, yaw);
    maxAngleUp -= 0.001f; 
    if ( -(lookRotation.y) > maxAngleUp) { lookRotation.y = -maxAngleUp; }

    float maxAngleDown = Vector3Angle(Vector3Negate(up), yaw);
    maxAngleDown *= -1.0f; 
    maxAngleDown += 0.001f; 
    if ( -(lookRotation.y) < maxAngleDown) { lookRotation.y = -maxAngleDown; }

    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    float pitchAngle = -lookRotation.y - lean.y;
    pitchAngle = Clamp(pitchAngle, -PI/2 + 0.0001f, PI/2 - 0.0001f); 
    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    // 카메라 기울임 및 출렁임 연출 (역동감 복구!)
    float headSin = sinf(headTimer * PI);
    float headCos = cosf(headTimer * PI);
    const float stepRotation = 0.01f;
    camera->up = Vector3RotateByAxisAngle(up, pitch, headSin * stepRotation + lean.x);

    const float bobSide = 0.1f;
    const float bobUp = 0.15f;
    Vector3 bobbing = Vector3Scale(right, headSin * bobSide);
    bobbing.y = fabsf(headCos * bobUp);

    camera->position = Vector3Add(camera->position, Vector3Scale(bobbing, walkLerp));
    camera->target = Vector3Add(camera->position, pitch);
}
