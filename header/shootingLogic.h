//메크로
//이거는 헤더 파일이 여러번 포함되는 것을 방지하기 위해서 사용된다.
#pragma once
#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float radius;
    bool active;
} Bullet;

extern int total_bullets;
// main.c에서 호출할 함수들
void InitShooting();
void UpdateAndDrawBullets();
void ShootingLogic();