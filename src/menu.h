/*
 * $Id: menu.h 2645 2006-01-07 08:30:15Z solus $
 */

#ifndef MENU_H
#define MENU_H

#include <list>
#include <string>
#include "event.h"
#include "menuitem.h"
#include "observable.h"

struct Menu;
struct TextView;

struct MenuEvent {
public:
    enum Type {
        ACTIVATE,
        INCREMENT,
        DECREMENT,
        SELECT,
        RESET
    };

    MenuEvent(const Menu *menu, Type type, const MenuItem *item = NULL) {
        this->menu = menu;
        this->type = type;
        this->item = item;
    }

    const Menu *getMenu() { return menu; }
    Type getType() { return type; }
    const MenuItem *getMenuItem() { return item; }

private:
    const Menu *menu;
    Type type;
    const MenuItem *item;
};

/**
 * Menu struct definition
 */
struct Menu : public Observable<Menu *, MenuEvent &> {
public:
    typedef std::list<MenuItem *> MenuItemList;

public:
    Menu();
    ~Menu();

    void                    removeAll();

    void                    add(int id, std::string text, short x, short y, int shortcutKey = -1);
    MenuItem *              add(int id, MenuItem *item);
    void                    addShortcutKey(int id, int shortcutKey);
    void                    setClosesMenu(int id);
    MenuItemList::iterator  getCurrent();
    void                    setCurrent(MenuItemList::iterator i);
    void                    setCurrent(int id);
    void                    show(TextView *view);
    bool                    isVisible();
    void                    next();
    void                    prev();
    void                    highlight(MenuItem *item);
    MenuItemList::iterator  begin();
    MenuItemList::iterator  end();
    MenuItemList::iterator  begin_visible();
    void                    reset(bool highlightFirst = true);
    MenuItemList::iterator  getById(int id);
    MenuItem*               getItemById(int id);
    void                    activateItem(int id, MenuEvent::Type action);
    bool                    activateItemByShortcut(int key, MenuEvent::Type action);
    bool                    getClosed() const;
    void                    setClosed(bool closed);
    void                    setTitle(const std::string &text, int x, int y);

private:
    MenuItemList items;
    MenuItemList::iterator current;
    MenuItemList::iterator selected;
    bool closed;
    std::string title;
    int titleX, titleY;
};

/**
 * This struct controls a menu.  The value field of WaitableController
 * isn't used.
 */
struct MenuController : public WaitableController<void *> {
public:
    MenuController(Menu *menu, TextView *view);
    bool keyPressed(int key);

protected:
    Menu *menu;
    TextView *view;
};

#endif
