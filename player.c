#include <stdbool.h>
#include <string.h>

#include "util.h"
#include "math3d.h"
#include "camera.h"
#include "player.h"

Player player = {0};

//
// Global functions
//

void player_init()
{
    memset(&player,0,sizeof(Player));

    player.height = 1.5f + 1.4f; // meters
    player.mass = 1.0f; // kg
}

void player_update()
{
   if(camera.mode == CAMERA_MODE_FOLLOW_PLAYER)
   {
       player.velocity.x = camera.velocity.x;
       player.velocity.y = camera.velocity.y;
       player.velocity.z = camera.velocity.z;

       player.position.x = camera.position.x;
       player.position.y = camera.position.y;
       player.position.z = camera.position.z;

       player.target.x = camera.target.x;
       player.target.y = camera.target.y;
       player.target.z = camera.target.z;

       player.up.x = camera.up.x;
       player.up.y = camera.up.y;
       player.up.z = camera.up.z;

       player.angle_h = camera.angle_h;
       player.angle_v = camera.angle_v;
    }
}
