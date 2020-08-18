#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include "math3d.h"
#include "text.h"
#include "menu.h"

void menu_init(int num_items,MenuItemList* item_list)
{
    if(!item_list)
        item_list = calloc(1,sizeof(MenuItemList));

    item_list->items = calloc(num_items,sizeof(MenuItem));
    item_list->capacity = num_items;
}

void menu_add_item(MenuItemList* item_list,MenuItem item)
{
    if(item_list->count == item_list->capacity)
    {
        printf("Item list is already at capacity. Failed to add item.");
        return;
    }

    memcpy(&item_list->items[item_list->count],&item,sizeof(MenuItem));
    item_list->count++;
}

void menu_add_item2(MenuItemList* item_list,float x, float y, float w, float h, char* text, int is_interactive, void* fn)
{
    if(item_list->count == item_list->capacity)
    {
        printf("Item list is already at capacity. Failed to add item.");
        return;
    }

    item_list->items[item_list->count].x = x;
    item_list->items[item_list->count].y = y;
    item_list->items[item_list->count].w = w;
    item_list->items[item_list->count].h = h;
    item_list->items[item_list->count].text = text;
    item_list->items[item_list->count].is_interactive = is_interactive;
    item_list->items[item_list->count].is_highlighted = 0;
    item_list->items[item_list->count].fn = fn;

    item_list->count++;
}

void menu_clear_all(MenuItemList* item_list)
{
    memset(item_list->items,0,item_list->capacity*sizeof(MenuItem));
    item_list->count = 0;
}

void menu_render(MenuItemList* item_list)
{
    for(int i = 0; i < item_list->count; ++i)
    {
        Vector3f color = {0.5f,0.5f,0.5f};
        if(item_list->items[i].is_highlighted)
        {
            color.x = 0.2f; color.y = 1.0f; color.z = 1.0f;
        }
        text_print(item_list->items[i].x,item_list->items[i].y,item_list->items[i].text,color);
    }
}
