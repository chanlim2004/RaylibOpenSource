/*******************************************************************************************
*
* raylib [core] example - 3d camera fps + Enemy AI
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

#include "enemy/enemy.h"
#include "enemy/fireball.h"

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

typedef struct
{
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;

} Body;

//----------------------------------------------------------------------------------
// 맵 설정
//----------------------------------------------------------------------------------
#define MAP_WIDTH 16
#define MAP_HEIGHT 16

int myMap[MAP_HEIGHT][MAP_WIDTH] =
{
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

Vector3 mapPosition = { -8.0f, 0.0f, -8.0f };

//----------------------------------------------------------------------------------
// 전역 변수
//----------------------------------------------------------------------------------
static Vector2 sensitivity = { 0.001f, 0.001f };

static Body player = { 0 };

static Enemy enemy;

static Vector2 lookRotation = { 0 };

static float headTimer = 0.0f;
static float walkLerp = 0.0f;
static float headLerp = STAND_HEIGHT;

static Vector2 lean = { 0 };

//----------------------------------------------------------------------------------
// 함수 선언
//----------------------------------------------------------------------------------
static bool CheckMapCollision(Vector3 testPos, float radius);

static void UpdateBody(
    Body *body,
    float rot,
    char side,
    char forward,
    bool jumpPressed,
    bool crouchHold
);

static void UpdateCameraFPS(Camera *camera);

//----------------------------------------------------------------------------------
// 메인 함수
//----------------------------------------------------------------------------------
int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib FPS + Enemy AI");

    player.position =
    (
        Vector3
        ){
            mapPosition.x + 1.0f,
            0.0f,
            mapPosition.z + 1.0f
        };

    // 적 생성
    InitEnemy(
        &enemy,
        (Vector3){ -5.0f, 0.0f, -5.0f }
    );

    // 불덩이 초기화
    InitFireballs();

    Camera camera = { 0 };

    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // 입력 처리
        //----------------------------------------------------------------------------------
        Vector2 mouseDelta = GetMouseDelta();

        lookRotation.x -= mouseDelta.x * sensitivity.x;
        lookRotation.y += mouseDelta.y * sensitivity.y;

        char sideway =
            (
                IsKeyDown(KEY_D)
                -
                IsKeyDown(KEY_A)
            );

        char forward =
            (
                IsKeyDown(KEY_W)
                -
                IsKeyDown(KEY_S)
            );

        bool crouching =
            IsKeyDown(KEY_LEFT_CONTROL);

        // 플레이어 업데이트
        UpdateBody(
            &player,
            lookRotation.x,
            sideway,
            forward,
            IsKeyPressed(KEY_SPACE),
            crouching
        );

        // 적 업데이트
        UpdateEnemy(
            &enemy,
            player.position,
            GetFrameTime()
        );

        // 불덩이 업데이트
        UpdateFireballs(
            GetFrameTime()
        );

        //----------------------------------------------------------------------------------
        // 카메라 연출
        //----------------------------------------------------------------------------------
        float delta = GetFrameTime();

        headLerp =
            Lerp(
                headLerp,
                (crouching ? CROUCH_HEIGHT : STAND_HEIGHT),
                20.0f * delta
            );

        camera.position =
        (
            Vector3
            ){
                player.position.x,
                player.position.y + (BOTTOM_HEIGHT + headLerp),
                player.position.z
            };

        if (
            player.isGrounded
            &&
            (
                (forward != 0)
                ||
                (sideway != 0)
            )
        )
        {
            headTimer += delta * 3.0f;

            walkLerp =
                Lerp(
                    walkLerp,
                    1.0f,
                    10.0f * delta
                );

            camera.fovy =
                Lerp(
                    camera.fovy,
                    55.0f,
                    5.0f * delta
                );
        }
        else
        {
            walkLerp =
                Lerp(
                    walkLerp,
                    0.0f,
                    10.0f * delta
                );

            camera.fovy =
                Lerp(
                    camera.fovy,
                    60.0f,
                    5.0f * delta
                );
        }

        lean.x =
            Lerp(
                lean.x,
                sideway * 0.02f,
                10.0f * delta
            );

        lean.y =
            Lerp(
                lean.y,
                forward * 0.015f,
                10.0f * delta
            );

        UpdateCameraFPS(&camera);

        //----------------------------------------------------------------------------------
        // 그리기 시작
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                // 바닥
                DrawPlane(
                    (Vector3){ 0.0f, 0.0f, 0.0f },
                    (Vector2){ MAP_WIDTH * 1.0f, MAP_HEIGHT * 1.0f },
                    DARKGRAY
                );

                // 적 그리기
                DrawEnemy(&enemy);

                // 불덩이 그리기
                DrawFireballs();

                // 벽 그리기
                for (int y = 0; y < MAP_HEIGHT; y++)
                {
                    for (int x = 0; x < MAP_WIDTH; x++)
                    {
                        if (myMap[y][x] == 1)
                        {
                            Vector3 wallPos =
                            {
                                mapPosition.x + x * 1.0f,
                                mapPosition.y + 1.5f,
                                mapPosition.z + y * 1.0f
                            };

                            DrawCube(
                                wallPos,
                                1.0f,
                                3.0f,
                                1.0f,
                                RED
                            );

                            DrawCubeWires(
                                wallPos,
                                1.0f,
                                3.0f,
                                1.0f,
                                MAROON
                            );
                        }
                    }
                }

            EndMode3D();

            //----------------------------------------------------------------------------------
            // 미니맵
            //----------------------------------------------------------------------------------
            int scale = 6;

            int minimapX =
                GetScreenWidth()
                -
                (MAP_WIDTH * scale)
                -
                20;

            int minimapY = 20;

            DrawRectangle(
                minimapX,
                minimapY,
                MAP_WIDTH * scale,
                MAP_HEIGHT * scale,
                Fade(BLACK, 0.5f)
            );

            for (int y = 0; y < MAP_HEIGHT; y++)
            {
                for (int x = 0; x < MAP_WIDTH; x++)
                {
                    if (myMap[y][x] == 1)
                    {
                        DrawRectangle(
                            minimapX + x * scale,
                            minimapY + y * scale,
                            scale,
                            scale,
                            RED
                        );
                    }
                }
            }

            DrawRectangleLines(
                minimapX,
                minimapY,
                MAP_WIDTH * scale,
                MAP_HEIGHT * scale,
                GREEN
            );

            // 플레이어 위치
            int playerCellX =
                (int)(
                    player.position.x
                    -
                    mapPosition.x
                    +
                    0.5f
                );

            int playerCellY =
                (int)(
                    player.position.z
                    -
                    mapPosition.z
                    +
                    0.5f
                );

            DrawRectangle(
                minimapX + playerCellX * scale,
                minimapY + playerCellY * scale,
                scale,
                scale,
                GREEN
            );

            // 적 위치
            int enemyCellX =
                (int)(
                    enemy.position.x
                    -
                    mapPosition.x
                    +
                    0.5f
                );

            int enemyCellY =
                (int)(
                    enemy.position.z
                    -
                    mapPosition.z
                    +
                    0.5f
                );

            DrawRectangle(
                minimapX + enemyCellX * scale,
                minimapY + enemyCellY * scale,
                scale,
                scale,
                BLUE
            );

            //----------------------------------------------------------------------------------
            // UI
            //----------------------------------------------------------------------------------
            DrawFPS(10, 10);

            DrawText(
                "WASD : Move",
                10,
                40,
                20,
                BLACK
            );

            DrawText(
                "SPACE : Jump",
                10,
                70,
                20,
                BLACK
            );

            DrawText(
                "Enemy AI Active",
                10,
                100,
                20,
                BLUE
            );

            DrawText(
                TextFormat(
                    "Enemy Pos: %.2f %.2f %.2f",
                    enemy.position.x,
                    enemy.position.y,
                    enemy.position.z
                ),
                10,
                130,
                20,
                BLACK
            );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

//----------------------------------------------------------------------------------
// 맵 충돌 검사
//----------------------------------------------------------------------------------
static bool CheckMapCollision(
    Vector3 testPos,
    float radius
)
{
    Vector2 pos2D =
    {
        testPos.x,
        testPos.z
    };

    int cellX =
        (int)(
            testPos.x
            -
            mapPosition.x
            +
            0.5f
        );

    int cellY =
        (int)(
            testPos.z
            -
            mapPosition.z
            +
            0.5f
        );

    for (int y = cellY - 1; y <= cellY + 1; y++)
    {
        if (y >= 0 && y < MAP_HEIGHT)
        {
            for (int x = cellX - 1; x <= cellX + 1; x++)
            {
                if (x >= 0 && x < MAP_WIDTH)
                {
                    if (myMap[y][x] == 1)
                    {
                        Rectangle wallRect =
                        {
                            mapPosition.x - 0.5f + x * 1.0f,
                            mapPosition.z - 0.5f + y * 1.0f,
                            1.0f,
                            1.0f
                        };

                        if (
                            CheckCollisionCircleRec(
                                pos2D,
                                radius,
                                wallRect
                            )
                        )
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

//----------------------------------------------------------------------------------
// 플레이어 물리 처리
//----------------------------------------------------------------------------------
static void UpdateBody(
    Body *body,
    float rot,
    char side,
    char forward,
    bool jumpPressed,
    bool crouchHold
)
{
    Vector2 input =
    {
        (float)side,
        (float)-forward
    };

    if ((side != 0) && (forward != 0))
    {
        input = Vector2Normalize(input);
    }

    float delta = GetFrameTime();

    if (!body->isGrounded)
    {
        body->velocity.y -= GRAVITY * delta;
    }

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = JUMP_FORCE;

        body->isGrounded = false;
    }

    Vector3 front =
    {
        sinf(rot),
        0.f,
        cosf(rot)
    };

    Vector3 right =
    {
        cosf(-rot),
        0.f,
        sinf(-rot)
    };

    Vector3 desiredDir =
    {
        input.x * right.x + input.y * front.x,
        0.0f,
        input.x * right.z + input.y * front.z
    };

    body->dir =
        Vector3Lerp(
            body->dir,
            desiredDir,
            CONTROL * delta
        );

    float decel =
        (
            body->isGrounded
            ?
            FRICTION
            :
            AIR_DRAG
        );

    Vector3 hvel =
    {
        body->velocity.x * decel,
        0.0f,
        body->velocity.z * decel
    };

    float speed =
        Vector3DotProduct(
            hvel,
            body->dir
        );

    float maxSpeed =
        (
            crouchHold
            ?
            CROUCH_SPEED
            :
            MAX_SPEED
        );

    float accel =
        Clamp(
            maxSpeed - speed,
            0.f,
            MAX_ACCEL * delta
        );

    hvel.x += body->dir.x * accel;
    hvel.z += body->dir.z * accel;

    body->velocity.x = hvel.x;
    body->velocity.z = hvel.z;

    float playerRadius = 0.2f;

    Vector3 nextPosX = body->position;

    nextPosX.x += body->velocity.x * delta;

    if (!CheckMapCollision(nextPosX, playerRadius))
    {
        body->position.x = nextPosX.x;
    }
    else
    {
        body->velocity.x = 0.0f;
    }

    Vector3 nextPosZ = body->position;

    nextPosZ.z += body->velocity.z * delta;

    if (!CheckMapCollision(nextPosZ, playerRadius))
    {
        body->position.z = nextPosZ.z;
    }
    else
    {
        body->velocity.z = 0.0f;
    }

    body->position.y += body->velocity.y * delta;

    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;

        body->velocity.y = 0.0f;

        body->isGrounded = true;
    }
}

//----------------------------------------------------------------------------------
// FPS 카메라
//----------------------------------------------------------------------------------
static void UpdateCameraFPS(Camera *camera)
{
    const Vector3 up =
    {
        0.0f,
        1.0f,
        0.0f
    };

    const Vector3 targetOffset =
    {
        0.0f,
        0.0f,
        -1.0f
    };

    Vector3 yaw =
        Vector3RotateByAxisAngle(
            targetOffset,
            up,
            lookRotation.x
        );

    Vector3 right =
        Vector3Normalize(
            Vector3CrossProduct(yaw, up)
        );

    float pitchAngle =
        Clamp(
            -lookRotation.y - lean.y,
            -PI/2 + 0.0001f,
            PI/2 - 0.0001f
        );

    Vector3 pitch =
        Vector3RotateByAxisAngle(
            yaw,
            right,
            pitchAngle
        );

    float headSin =
        sinf(headTimer * PI);

    float headCos =
        cosf(headTimer * PI);

    const float stepRotation = 0.01f;

    camera->up =
        Vector3RotateByAxisAngle(
            up,
            pitch,
            headSin * stepRotation + lean.x
        );

    const float bobSide = 0.1f;
    const float bobUp = 0.15f;

    Vector3 bobbing =
        Vector3Scale(
            right,
            headSin * bobSide
        );

    bobbing.y =
        fabsf(headCos * bobUp);

    camera->position =
        Vector3Add(
            camera->position,
            Vector3Scale(
                bobbing,
                walkLerp
            )
        );

    camera->target =
        Vector3Add(
            camera->position,
            pitch
        );
}