#include "../header/userInterface.h"
#include "../header/shootingLogic.h"
#include "raylib.h"
#include <stdio.h>

struct playerInterfaceInfo
{
    int health;
    int ammo;
};

//이거는 플레이어의 인터페이스를 그려주는 함수이다
void basicPlayerInterface(struct playerInterfaceInfo playerInfo)
{
    playerInfo.health = 100;
    playerInfo.ammo = total_bullets;
    DrawText(TextFormat("Health: %d", playerInfo.health), 10, 40, 20, BLUE);
    DrawText(TextFormat("Ammo: %d", playerInfo.ammo), 10, 80, 20, BLUE);
}

void Interface()
{
    //이거는 그냥 화면에 텍스트를 띄우는 것이다
    DrawRectangle(325, 300, 150, 150, SKYBLUE);
    basicPlayerInterface((struct playerInterfaceInfo){0});
}