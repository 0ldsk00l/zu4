/*
 * $Id: item.h 2595 2005-10-09 04:28:37Z andrewtaylor $
 */

#ifndef ITEM_H
#define ITEM_H

struct Map;
struct Coords;

enum SearchCondition {
    SC_NONE         = 0x00,
    SC_NEWMOONS     = 0x01,
    SC_FULLAVATAR   = 0x02,
    SC_REAGENTDELAY = 0x04
};

struct ItemLocation {
    const char *name;
    const char *shortname;
    const char *locationLabel;
    bool (*isItemInInventory)(int item);
    void (*putItemInInventory)(int item);
    void (*useItem)(int item);
    int data;
    unsigned char conditions;
};

typedef void (*DestroyAllCreaturesCallback)(void);

void itemSetDestroyAllCreaturesCallback(DestroyAllCreaturesCallback callback);
const ItemLocation *itemAtLocation(const Map *map, const Coords &coords);
void itemUse(const char *shortname);

#endif
