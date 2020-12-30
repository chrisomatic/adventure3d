#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "util.h"
#include "math3d.h"
#include "camera.h"
#include "player.h"

Player player = {0};

static void load_player_props();

//
// Global functions
//

void player_init()
{
    memset(&player,0,sizeof(Player));

    player.accel_factor = 0.1f;
    player.height = 1.5f + 1.4f; // meters
    player.mass = 1.0f; // kg

    memset(player.name,0,16);
    strncpy(player.name,"Player",16);

    player.color.x = 103;
    player.color.y = 90;
    player.color.z = 45;

    load_player_props();
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

static void load_player_props()
{
    // open file
    FILE* fp = fopen("player.props","r");

    if (fp == NULL)
        return;

    // Expected:
    // key : value \n
    // key : value \n
    // ...
    // key : value EOF

    bool is_key = true;
    bool is_string = false;

    int ikey = 0;
    int ivalue = 0;

    char key[100] = { 0 };
    char value[100] = { 0 };

    int c;
    do
    {
        c = fgetc(fp);
        if (c == '#')
        {
            // comment, read til end of line or end of file, whichever comes first.
            while(c != EOF && c != '\n')
                c = fgetc(fp);
        }
        
        if (c == '"')
        {
            is_string = !is_string;
        }
        else if (c == ':')
        {
            is_key = false;
            key[ikey] = '\0';
            ikey = 0;
        }
        else if (c == '\n' || c == EOF)
        {
            is_key = true;
            ivalue = 0;
            
            if (STR_EQUAL(key, "name")) strncpy(player.name,value,16);

            memset(key, 0, 100);
            memset(value, 0, 100);
        }
        else
        {
            if(!is_string && (c == ' ' || c == '\t' || c == '\r'))
            {
                // ignore white space unless it's inside a string
            }
            else
            {
                if (is_key)
                    key[ikey++] = c;
                else
                    value[ivalue++] = c;
            }
        }

    } while (c != EOF);

    fclose(fp);
}
