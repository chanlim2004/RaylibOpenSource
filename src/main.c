#include "raylib.h"
#include "raymath.h"
#include "../header/userInterface.h"
#include "../header/main.h"
#include "../header/shootingLogic.h"

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
#define MAP_WIDTH 16
#define MAP_HEIGHT 16

//2차원 배열이다
static int myMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};
//map이 정확의 어디에 위치를 할건지에 대한 내용
static Vector3 mapPosition = { -8.0f, 0.0f, -8.0f };  

//이거는 그냥 screen을 소환하는거임
const int screenWidth = 800;
const int screenHeight = 450;
//camera소환
Camera camera = { 0 };

//전역 변수들
//얼마나 마우스를 빨리 움직일 것인지
static Vector2 sensitivity = { 0.001f, 0.001f };
//이거는 실제 플레이어
static Body player = { 0 };
//rotation 인간이 얼마나 요리보고 조리보고 할 수 있는지
static Vector2 lookRotation = { 0 };
//이거는 정확의 뭔지 잘 모름 이따가 돌아오겠음
static float headTimer = 0.0f;
static float walkLerp = 0.0f;
static float headLerp = STAND_HEIGHT;
static Vector2 lean = { 0 };

// 함수 선언
static bool CheckMapCollision(Vector3 testPos, float radius);
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold);
static void UpdateCameraFPS(Camera *camera);

int main(void)
{
    //총알 초기화
    InitShooting();
    InitWindow(screenWidth, screenHeight, "raylib - Smooth Wall Sliding & Head Bobbing");

    //플레이어의 실제 포지션 어디에 있을것인지를 보는거다
    player.position = (Vector3){ mapPosition.x + 1.0f, 0.0f, mapPosition.z + 1.0f }; 

    //camera fovy --> 카메라를 축소하고 늘리는것을 담당한다
    camera.fovy = 60.0f;
    //멀리 보이는 물체는 작게 보이고 가까이에 있는 물체는 크게 보이게끔 한다.
    //그것이 Camera Perspective
    camera.projection = CAMERA_PERSPECTIVE;
    
    //커서는 안보이게 하는것
    DisableCursor(); 
    //FPS는 60프레임으로 마추는 것이다
    SetTargetFPS(60);

    while (!WindowShouldClose())    
    {
	
	// 마우스가 지난 프레임보다 얼마나 더 움직였니?
        Vector2 mouseDelta = GetMouseDelta();
	// 이것은 카메라 로테이션을 다룬다 디테일 한것은 노트에 있다.(1)
	// 3D그래픽스는 표준 수학은 반시계 방향이 왼쪽이 +다
	// 그래서 오른쪽으로 돌리려면 (+)값 오른쪽 마우스 방향을 빼줘야 오른쪽으로 돌아간다
        lookRotation.x -= mouseDelta.x * sensitivity.x;
        lookRotation.y += mouseDelta.y * sensitivity.y;

        //마우스 클릭을 받는곳
        //총알을 쏘기 위한 로직
        ShootingLogic();
	//이거는 옆앞 WDSA를 설정하는것이다
        char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        bool crouching = IsKeyDown(KEY_LEFT_CONTROL);

        // UpdateBody 안에서 X/Z축 이동과 스무스한 충돌 처리를 모두 완료합니다.
        UpdateBody(&player, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE), crouching);

	
	// 지난 프레임부터 지금까지 얼마나 시간이 흘렀나를 보는것입니다.
        float delta = GetFrameTime();
	//Lerp는 --> 선형보간의 약자다
	//일종에 두 점을 부드럽게 이어주는 방식이다
	//Lerp(0.0f, crouching 즉 ctrl를 눌렀으면 --> 0.0f즉 CROUCH_HEIGHT를 주는것이고 만약에 아니면 그냥 STAND_HEIGHT=1을 준다, 이것은 lerp time이다) 
        headLerp = Lerp(headLerp, (crouching ? CROUCH_HEIGHT : STAND_HEIGHT), 20.0f * delta);
        
        camera.position = (Vector3){
            player.position.x,
	    //이것은 BOTTOM_HEIGHT = 0.5f 그리고 headlerp = crtl를 누르면 0 그리고 아니면 1.0f를 준다
            player.position.y + (BOTTOM_HEIGHT + headLerp),
            player.position.z,
        };
	//이코드는 카메라 시야 좁히는 코드
	//만약 player가 땅에 있고 움직이고 있다면
        if (player.isGrounded && ((forward != 0) || (sideway != 0)))
        {
	    //사람이 거를떄마다 타이머다 이것은 오로지 캐릭터가 걸을때만 작용되는 것입니다
            headTimer += delta * 3.0f;
	    //화면이 출렁거리는것을 표현 0.0부터 1.0 까지 부드럽게 표현하는 선형 보간법
            walkLerp = Lerp(walkLerp, 1.0f, 10.0f * delta);
	    //카메라 fovy즉 카메라를 돌리는거다 60.0f에서 55.0f으로 축소를 하는것이다
            camera.fovy = Lerp(camera.fovy, 55.0f, 5.0f * delta); // 시야 좁아짐 연출
        }
        else
        {
	    //다시 원상복구 하는것이다
            walkLerp = Lerp(walkLerp, 0.0f, 10.0f * delta);
            camera.fovy = Lerp(camera.fovy, 60.0f, 5.0f * delta);
        }
	//여기는 실제 연출 즉 움직이는것을 연출
	//lean은 일종에 백터인데 Vector2(0)
	//좌우로 0.02f만큼 기우는걸 연출하기 위한 과정
	//이제는 너도 lerp이 뭔지 아니깐 걍 읽으삼
        lean.x = Lerp(lean.x, sideway * 0.02f, 10.0f * delta); // 좌우 몸 기울임 연출
        lean.y = Lerp(lean.y, forward * 0.015f, 10.0f * delta);

	//이제 카메라 업데이트에다가 (주소camera) 넘겨주는 과정
        UpdateCameraFPS(&camera); 

        BeginDrawing();
	    //전부다 하얀색으로 칠하는거임 배경을
            ClearBackground(RAYWHITE); 
	    //카메라 시작
            BeginMode3D(camera);
                //바로 바닥을 까라주는 코드가
		//Map Width와 Map Height만큼
                DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ MAP_WIDTH * 1.0f, MAP_HEIGHT * 1.0f }, DARKGRAY);
		//이제 실제로 배열을 하는 과정이다 우리가 봤던 0,0,1,0,0,1이런 코드를
                for (int y = 0; y < MAP_HEIGHT; y++) 
                {
                    for (int x = 0; x < MAP_WIDTH; x++) 
                    {
			//만약에 내 배열에
                        if (myMap[y][x] == 1) 
                        {//이제 MAP의 position그리고 그것들의 크기를 설정을 해줄것이다
                            Vector3 wallPos = { mapPosition.x + x * 1.0f, mapPosition.y + 1.5f, mapPosition.z + y * 1.0f };
			    //이거는 큐브 및 와이어들을 그려주는 것이다
                            DrawCube(wallPos, 1.0f, 3.0f, 1.0f, RED);               
                            DrawCubeWires(wallPos, 1.0f, 3.0f, 1.0f, MAROON);     
                        }
                    }
                }
                //이것은 총알을 업데이트하고 그려주는 함수이다
                UpdateAndDrawBullets();
            //그릴건 다 끝났으니 해재한다
            EndMode3D();

            // 2D 미니맵 레이더
            int scale = 6; 
	    //이것은 간단하게 사이즈를 저장해주는 코드이다
	    //screenWidth - (작은 MAP크기 = 16 *scale)-20
            int minimapX = GetScreenWidth() - (MAP_WIDTH * scale) - 20;
            int minimapY = 20;
            //이것은 userInterface.c에 있는 함수이다 그냥 인터페이스를 그려주는 함수이다
            Interface();
	    //DrawRectangle를 그려주는 시간이다
	    //(위치, 크기, 색깔) 이순으로 간다
            DrawRectangle(minimapX, minimapY, MAP_WIDTH * scale, MAP_HEIGHT * scale, Fade(BLACK, 0.5f));
            
	    //아까와 비슷한 양식으로 MAP에다가 직사각형을 그려주는 방식이다 MAP에다가
            for (int y = 0; y < MAP_HEIGHT; y++) {
                for (int x = 0; x < MAP_WIDTH; x++) {
                    if (myMap[y][x] == 1) {
                        DrawRectangle(minimapX + x * scale, minimapY + y * scale, scale, scale, RED);
                    }
                }
            }
	    //이것은 맵에 태두리라고 볼 수 있다
            DrawRectangleLines(minimapX, minimapY, MAP_WIDTH * scale, MAP_HEIGHT * scale, GREEN); 

            // 미니맵 위 플레이어 위치이다 yes
	    //player이 정확의 어디에 위치를 하고 있는지 계산
	    //player.x - 전체 map position player 위치만 남는다
            int playerCellX = (int)(player.position.x - mapPosition.x + 0.5f);
	    //이것도 똑같겠지
            int playerCellY = (int)(player.position.z - mapPosition.z + 0.5f);
	    //실제로 플레이어를 그리는 과정이다
            DrawRectangle(minimapX + playerCellX * scale, minimapY + playerCellY * scale, scale, scale, GREEN);
	    
	    //이거는 내 프로그램의 FPS를 뛰어주는 창이다
            DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();                  
    return 0;
}

static bool CheckMapCollision(Vector3 testPos, float radius)
{
    //좌표 평면만 보겠다 2D로 보겠다 이말이다
    //이것은 어찌보면 플레이어의 포지션이다
    Vector2 pos2D = { testPos.x, testPos.z };
    
    //이거는 각각의 셀을 보는것이다 예를 들어보겠다
    //어떤 셀에 플레이어와 맵 사이에 길이를 구하는것이다
    //맵은 (0,0)에서 시작을 하고 실질적으로는 끝은 -8에 있다
    //마지막 0.5f더하기는 일종에 리버스 가우스화 하는것입니다
    //2.3이 나오면 3으로 바꿔준다
    //2.1이 나와도 3으로 그냥 반올림 한다
    int cellX = (int)(testPos.x - mapPosition.x + 0.5f);
    int cellY = (int)(testPos.z - mapPosition.z + 0.5f);
    
    //CellY를 주변 즉 내 위치 기준 +1 그리고 -1 분석을 한다 int단위로
    for (int y = cellY - 1; y <= cellY + 1; y++) {
	//만약에 MAP을 버서나지 않았다면??
        if (y >= 0 && y < MAP_HEIGHT) {
	    //CELLX도 비슷한 양식으로 간다
            for (int x = cellX - 1; x <= cellX + 1; x++) {
                if (x >= 0 && x < MAP_WIDTH) {
		    //만약에 벽이 있다면
                    if (myMap[y][x] == 1) {
			//투명한 박스를 만드는 함수이다
			//Reactangle(포지션, 크기);
                        Rectangle wallRect = { 
			    //여기서 0.5 마이너스를 하는 이유는 rectangle을 정중앙 즉 플레이어 기준에 맞추는 것이다
			    //원래 rectangle에 오리지널 포지션은 왼쪽 위입니다.
			    //마지막 1.0f를 곱하는 이유는 그냥 정수를 실수 안전하게 바꿀려는 이유다
                            mapPosition.x - 0.5f + x * 1.0f, 
                            mapPosition.z - 0.5f + y * 1.0f, 
                            1.0f, 1.0f 
                        };
			//실제 콜리션을 체크하는 라이브러리 함수인데
			//서로 부디치는지 그냥 채크하는것이다
			//CheckCollisionCircleRec(현 위치,플레이어의 두깨, 콜리젼 체커)
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
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold)
{
    //z축은 -가 붙는 이유는 앞으로 가는게 - 그리고 뒤로 가는게 +다
    Vector2 input = (Vector2){ (float)side, (float)-forward };
    //normalize즉 백터의 방향을 정하는거다 --> 백터를 다 0~1로 맞추는 과정이다
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input);

    //Frame단위 시간
    float delta = GetFrameTime();

    //body가 만약 바닥에 있지 않는다면 GRAVITY를 적용한다
    if (!body->isGrounded) body->velocity.y -= GRAVITY * delta;
    
    //만약 점프를 눌렀다면??
    if (body->isGrounded && jumpPressed)
    {
	//그거를 실제로 적용하는 과정
	//솔직히 너 이정도는 알아야 해
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;
    }


    //여기는 방향을 정하는거임
    //front는 앞 direction이다
    Vector3 front = (Vector3){ sinf(rot), 0.f, cosf(rot) };
    //right는 옆 direction이다
    Vector3 right = (Vector3){ cosf(-rot), 0.f, sinf(-rot) };
    //이거는 모든것을 고려하는것이다
    //이것이 진짜 플레이어의 다이렉션이다
    Vector3 desiredDir = (Vector3){ input.x * right.x + input.y * front.x, 0.0f, input.x * right.z + input.y * front.z, };
    
    //기존 body direction에서 desiredDir(우리가 적은것 방금)을 부드럽게 넘어가겠금 하는 변수이다
    body->dir = Vector3Lerp(body->dir, desiredDir, CONTROL * delta);

    //만약에 바닥에 붙어있으면 FRICTION을 붙침 아니면 AIR_DRAG를 적용
    float decel = (body->isGrounded ? FRICTION : AIR_DRAG);
    //FRICTION을 이제 곱해주는거임 velocity값에
    Vector3 hvel = (Vector3){ body->velocity.x * decel, 0.0f, body->velocity.z * decel };

    //그리고 그 velocity의 기리를 구하는거임
    float hvelLength = Vector3Length(hvel);
    //만약 velocity*FRICTION의 길이가 MAX_SPEED를 넘어가면 안됌
    if (hvelLength < (MAX_SPEED * 0.01f)) hvel = (Vector3){ 0 };

    //내적을 구하면 스피드가 나옴
    //이거는 노트에 더 디테일하게 나와있다
    float speed = Vector3DotProduct(hvel, body->dir);

    //MAX_SPEED는 얼마인지를 본다
    float maxSpeed = (crouchHold ? CROUCH_SPEED : MAX_SPEED);
    //이거는 가속도인데 만약 speed가 80이고 maxSpeed가 100이면 20임 그러면 0 과 20 으로 Clamp를 하는것이다
    float accel = Clamp(maxSpeed - speed, 0.f, MAX_ACCEL * delta);
    //그리고 가속도를 붙치는것이다 실제 dir에다가 곱하는것이다
    hvel.x += body->dir.x * accel;
    hvel.z += body->dir.z * accel;

    //이제 모든것을 
    //veclocity.x = FRICTION * dir * accel 까지 진짜 speed가 만들어지는 과정이다
    body->velocity.x = hvel.x;
    body->velocity.z = hvel.z;

    float playerRadius = 0.2f;

    //이동을 시도하는 과정
    //현 dir그리고 스피드(velocity)를 넘겨준다
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
    //그냥 바닥에 있거나 바닥을 뚫고 밑으로 들어가려고 하면????
    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true; 
    }
}

static void UpdateCameraFPS(Camera *camera)
{
    //위를 바라보는 백터
    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    //앞을 바라보는 백터
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };
    // 앞쪽 화살표를 Y축(up) 기준으로 좌우(lookRotation.x)로만 돌려놓은 내 몸통의 정면 방향--> 그냥 내가 현제 발아보고 있는 방향이다
    // 고개를 도리도리 하게 하는것
    Vector3 horizontalForward = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    //하지만 이거는 어디까지나 그냥 90도 수직이다 up은 어디까지나 90도고
    //horizontalForward는 어디까지나 z축 방향이다
    
    //우리는 현제 도리도리를 보는거기는 하지만 혹시 어디까지 끄덕끄덕을 할 수 있어
    float maxAngleUp = Vector3Angle(up, horizontalForward);
    //약간 줄이는거임 만약 80도라고 가정을 하면 79.999도로 줄이는 거임
    maxAngleUp -= 0.001f;
    //이거는 90도를 못 넘어가게 잡아주는 것이다
    if ( -(lookRotation.y) > maxAngleUp) { lookRotation.y = -maxAngleUp; }

    //이거는 -90도다 근데 어짜피 다시 90도로 바꿀거긴 하다
    //밑에하고 완전히 똑같은 코드이지만 밑을 -90도까지 돌리지 못하겠금 한다.
    float maxAngleDown = Vector3Angle(Vector3Negate(up), horizontalForward);
    maxAngleDown *= -1.0f; 
    maxAngleDown += 0.001f; 
    if ( -(lookRotation.y) < maxAngleDown) { lookRotation.y = -maxAngleDown; }

    //이제 외적을 구하는건데... 이거는 그냥 노트에 더 디테일하게 나와있음
    //새로운 백터를 구하는 것이다 horizontalForward와 up 기준으로
    //우리가 maxAngleDown/maxAngleUp을 89.999로 설정한 이유가 여기서 나온다
    Vector3 right = Vector3Normalize(Vector3CrossProduct(horizontalForward, up));

    //player이 실제로 움직임 즉 head bobbing도 고려
    float pitchAngle = -lookRotation.y - lean.y;
    //이제 실제 끄덕끄던 적용하는 공간임
    pitchAngle = Clamp(pitchAngle, -PI/2 + 0.0001f, PI/2 - 0.0001f); 
    //z는 horizontalForward 그리고 x는 right 그리고 pitchAngle만큼 위아래로 회전을 시켜라
    //이것이 최종 형태다 끄덕끄덕 + 도리로리
    Vector3 pitch = Vector3RotateByAxisAngle(horizontalForward, right, pitchAngle);

    // 카메라 기울임 및 출렁임 연출 (역동감 복구!)
    // sin그리고 cos는 알다시피 파동 물결이다
    // t가 올라갈수록 파동 효과가 일어나거 마치 head가 움직이는 것처럼 보인다
    float headSin = sinf(headTimer * PI);
    float headCos = cosf(headTimer * PI);
    const float stepRotation = 0.01f;
    //이제 모든것을 합하는것이다
    camera->up = Vector3RotateByAxisAngle(up, pitch, headSin * stepRotation + lean.x);

    //좌우 흔들림 최대치
    const float bobSide = 0.1f;
    const float bobUp = 0.15f;
    //이제 모든것을 다 합치는 과정
    Vector3 bobbing = Vector3Scale(right, headSin * bobSide);
    bobbing.y = fabsf(headCos * bobUp);

    //fabsf는 그냥 절댓값이다
    camera->position = Vector3Add(camera->position, Vector3Scale(bobbing, walkLerp));
    camera->target = Vector3Add(camera->position, pitch);
}
