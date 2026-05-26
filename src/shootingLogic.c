#include "../header/shootingLogic.h"
#include "raylib.h"
#include "raymath.h"

//일단 main.c에서 필요한 카메라 그리고 screenHeight, screenWidth를 가져와 준다
Camera camera;
const int screenWidth;
const int screenHeight;

//이거는 걍 탄창 보관소 array
//나중에 change
static Bullet bullets[50] = { 0 };
int total_bullets = 50;

// 게임이 시작이 되면 일단 총알은 없음
void InitShooting() {
    for (int i = 0; i < 50; i++) {
        bullets[i].active = false;
    }
}

//여기는 말그대로 총알을 스폰하는 함수
void spawnBullet(Camera3D cam) {
    //일단 놀고 있는 총알을 찾아줌 bullets.active = false
    for (int i = 0; i < 50; i++) {
        if (!bullets[i].active) {
            //일단 시작점은 캐랙터의 위치로 설정
            bullets[i].position = cam.position;
            
            // dir 방향 구하기 --> 타겟 - 위치 후 정규화
            Vector3 direction = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
            float speed = 50.0f; // 총알 속도
            
            //실제 움직이는것 --> 방향 * 속도
            bullets[i].velocity = Vector3Scale(direction, speed);
            //이제 나갈 준비 완료임
            bullets[i].active = true;
            total_bullets -= 1;
            
            return; 
        }
    }
}

//여기는 마우스를 받는곳
void ShootingLogic() {
    // 마우스 누르면 소환
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        spawnBullet(camera);
    }
}

//이제 실제로 총알을 구현
void UpdateAndDrawBullets() {
    //프레임마다 시간 가져오기 (delta time)
    float deltaTime = GetFrameTime(); 

    for (int i = 0; i < 50; i++) {
        if (bullets[i].active) {
            // bullet 위치 = bullet 위치 + (bullet 속도 * 걍시간)
            //이것은 게임루프안에 있으므로 시간이 지남에 따라서 총알이 움직이는것이다
            bullets[i].position = Vector3Add(bullets[i].position, Vector3Scale(bullets[i].velocity, deltaTime));

            // 일단은 구체로 표현
            DrawSphere(bullets[i].position, 0.1f, YELLOW);

            // 카메라에서 너무 멀어지면 총알 걍 버리기
            // 카메라와의 거리가 100이 넘어가면 사라진다
            if (Vector3Distance(camera.position, bullets[i].position) > 100.0f) {
                bullets[i].active = false;
            }
        }
    }
}
