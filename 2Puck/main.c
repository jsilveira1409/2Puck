#include <ch.h>
#include <hal.h>
#include <memory_protection.h>
#include "main.h"
#include "console.h"
#include "game.h"


int main(void)
{
    halInit();
    chSysInit();
    mpu_init();

    console_init();
    game_init();
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
