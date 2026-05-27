#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"

typedef struct Enemy
{
    Vector3 position;

    float speed;

    float detectRange;

    bool active;

} Enemy;

// 적 생성
void InitEnemy(Enemy* enemy, Vector3 startPos);

// 적 업데이트
void UpdateEnemy(
    Enemy* enemy,
    Vector3 playerPos,
    float deltaTime
);

// 적 그리기
void DrawEnemy(Enemy* enemy);

#endif