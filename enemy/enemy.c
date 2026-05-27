#include "enemy.h"
#include "fireball.h"

#include "raylib.h"
#include "raymath.h"

#define ENEMY_RADIUS 0.3f

// main.c 에 있는 맵 데이터 사용
extern int myMap[16][16];

extern Vector3 mapPosition;

// -----------------------------------------------------------------------------
// 벽 충돌 검사
// -----------------------------------------------------------------------------
static bool CheckEnemyCollision(Vector3 testPos)
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
        if (y >= 0 && y < 16)
        {
            for (int x = cellX - 1; x <= cellX + 1; x++)
            {
                if (x >= 0 && x < 16)
                {
                    if (myMap[y][x] == 1)
                    {
                        Rectangle wallRect =
                        {
                            mapPosition.x - 0.5f + x,
                            mapPosition.z - 0.5f + y,
                            1.0f,
                            1.0f
                        };

                        if (
                            CheckCollisionCircleRec(
                                pos2D,
                                ENEMY_RADIUS,
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

// -----------------------------------------------------------------------------
// 적 초기화
// -----------------------------------------------------------------------------
void InitEnemy(
    Enemy* enemy,
    Vector3 startPos
)
{
    enemy->position = startPos;

    enemy->speed = 2.0f;

    enemy->detectRange = 10.0f;

    enemy->active = true;
}

// -----------------------------------------------------------------------------
// 적 업데이트
// -----------------------------------------------------------------------------
void UpdateEnemy(
    Enemy* enemy,
    Vector3 playerPos,
    float deltaTime
)
{
    if (!enemy->active)
        return;

    // 플레이어 방향 계산
    Vector3 direction =
        Vector3Subtract(
            playerPos,
            enemy->position
        );

    // 거리 계산
    float distance =
        Vector3Length(direction);

    // -------------------------------------------------------------------------
    // 플레이어 감지 시 추적
    // -------------------------------------------------------------------------
    if (distance < enemy->detectRange)
    {
        direction =
            Vector3Normalize(direction);

        // =====================================================
        // X축 이동
        // =====================================================
        Vector3 nextPosX =
            enemy->position;

        nextPosX.x +=
            direction.x
            *
            enemy->speed
            *
            deltaTime;

        if (!CheckEnemyCollision(nextPosX))
        {
            enemy->position.x =
                nextPosX.x;
        }

        // =====================================================
        // Z축 이동
        // =====================================================
        Vector3 nextPosZ =
            enemy->position;

        nextPosZ.z +=
            direction.z
            *
            enemy->speed
            *
            deltaTime;

        if (!CheckEnemyCollision(nextPosZ))
        {
            enemy->position.z =
                nextPosZ.z;
        }
    }

    // -------------------------------------------------------------------------
    // 불덩이 공격
    // 벽 너머 플레이어는 공격하지 않음
    // -------------------------------------------------------------------------
    static float shootTimer = 0.0f;

    shootTimer += deltaTime;

    if (
        distance < enemy->detectRange
        &&
        shootTimer >= 2.0f
        &&
        HasLineOfSight(
            enemy->position,
            playerPos
        )
    )
    {
        Vector3 fireballStart =
        {
            enemy->position.x,
            enemy->position.y + 1.0f,
            enemy->position.z
        };

        SpawnFireball(
            fireballStart,
            playerPos
        );

        shootTimer = 0.0f;
    }
}

// -----------------------------------------------------------------------------
// 적 렌더링
// -----------------------------------------------------------------------------
void DrawEnemy(Enemy* enemy)
{
    if (!enemy->active)
        return;

    Vector3 drawPos =
        enemy->position;

    drawPos.y = 0.75f;

    // 몸통
    DrawCube(
        drawPos,
        1.0f,
        1.5f,
        1.0f,
        BLUE
    );

    // 테두리
    DrawCubeWires(
        drawPos,
        1.0f,
        1.5f,
        1.0f,
        DARKBLUE
    );

    // 머리
    DrawSphere(
        (Vector3)
        {
            drawPos.x,
            drawPos.y + 1.1f,
            drawPos.z
        },
        0.25f,
        SKYBLUE
    );
}