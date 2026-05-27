#ifndef FIREBALL_H
#define FIREBALL_H

#include "raylib.h"

// -----------------------------------------------------------------------------
// 불덩이 구조체
// -----------------------------------------------------------------------------
typedef struct Fireball
{
    Vector3 position;

    Vector3 direction;

    float speed;

    bool active;

} Fireball;

// -----------------------------------------------------------------------------
// 함수 선언
// -----------------------------------------------------------------------------
void SpawnFireball(
    Vector3 startPos,
    Vector3 targetPos
);

void UpdateFireballs(
    float deltaTime
);

void DrawFireballs(void);

// 플레이어와 적 사이 시야 체크
bool HasLineOfSight(
    Vector3 start,
    Vector3 end
);

void InitFireballs(void);

#endif