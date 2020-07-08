#include <stdbool.h>
#include <string.h>
#include "player.h"

Player player = {0};

void player_init()
{
    memset(&player,0,sizeof(Player));
    player.height = 1.4f; // meters
}
