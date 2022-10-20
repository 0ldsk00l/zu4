/*
 * $Id: menuitem.h 2700 2007-08-02 08:53:48Z solus $
 */

#ifndef MENUITEM_H
#define MENUITEM_H

#include <string>
#include <set>
#include <vector>

struct MenuEvent;

/**
 * custom output types for with menu items that need
 * to perform special calculations before displaying
 * its associated value
 */
typedef enum {
    MENU_OUTPUT_INT,
    MENU_OUTPUT_GAMMA,
    MENU_OUTPUT_SHRINE,
    MENU_OUTPUT_SPELL,
    MENU_OUTPUT_VOLUME,
    MENU_OUTPUT_REAGENT
} menuOutputType;

struct MenuItem {
public:
    MenuItem(std::string text, short x, short y, int shortcutKey = -1);
    virtual ~MenuItem() {}

    virtual void activate(MenuEvent &event) {}

    // Accessor Methods
    int getId() const;
    short getX() const;
    short getY() const;
    int getScOffset() const;

    virtual std::string getText() const;
    bool isHighlighted() const;
    bool isSelected() const;
    bool isVisible() const;
    const std::set<int> &getShortcutKeys() const;
    bool getClosesMenu() const;

    void setId(int id);
    void setX(int xpos);
    void setY(int ypos);
    void setText(std::string text);
    void setHighlighted(bool h = true);
    void setSelected(bool s = true);
    void setVisible(bool v = true);
    void addShortcutKey(int shortcutKey);
    void setClosesMenu(bool closesMenu);

protected:
    int id;
    short x, y;
    std::string text;
    bool highlighted;
    bool selected;
    bool visible;
    int scOffset;
    std::set<int> shortcutKeys;
    bool closesMenu;
};

/**
 * A menu item that toggles a boolean value, and displays the current
 * setting as part of the text.
 */
struct BoolMenuItem : public MenuItem {
public:
    BoolMenuItem(std::string text, short x, short y, int shortcutKey, bool *val);

    BoolMenuItem *setValueStrings(const std::string &onString, const std::string &offString);

    virtual void activate(MenuEvent &event);
    virtual std::string getText() const;

protected:
    bool *val;
    std::string on, off;
};

/**
 * A menu item that cycles through a list of possible string values, and
 * displays the current setting as part of the text.
 */
struct StringMenuItem : public MenuItem {
public:
    StringMenuItem(std::string text, short x, short y, int shortcutKey, std::string *val, const std::vector<std::string> &validSettings);

    virtual void activate(MenuEvent &event);
    virtual std::string getText() const;

protected:
    std::string *val;
    std::vector<std::string> validSettings;
};

/**
 * A menu item that cycles through a list of possible integer values,
 * and displays the current setting as part of the text.
 */
struct IntMenuItem : public MenuItem {
public:
    IntMenuItem(std::string text, short x, short y, int shortcutKey, int *val, int min, int max, int increment, menuOutputType output=MENU_OUTPUT_INT);

    virtual void activate(MenuEvent &event);
    virtual std::string getText() const;

protected:
    int *val;
    int min, max, increment;
    menuOutputType output;
};

#endif
