#pragma once

typedef struct
{
    float x,y;
    float w,h;
    char* text;
    int is_highlighted;
    int is_interactive;
    void (*fn)();
} MenuItem;

typedef struct
{
    MenuItem* items;
    int count;
    int capacity;
} MenuItemList;

void menu_init(int num_items,MenuItemList* item_list);
void menu_add_item(MenuItemList* item_list,MenuItem item);
void menu_add_item2(MenuItemList* item_list,float x, float y, float w, float h, char* text, int is_interactive, void* fn);
void menu_clear_all(MenuItemList* item_list);
void menu_render(MenuItemList* item_list);
