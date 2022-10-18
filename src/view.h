/*
 * $Id: view.h 2951 2011-07-11 12:23:39Z darren_janeczek $
 */

#ifndef VIEW_H
#define VIEW_H

struct Image;

/**
 * Generic base struct for reflecting the state of a game object onto
 * the screen.
 */
struct View {
public:
    View(int x, int y, int width, int height);
    virtual ~View() {}

    virtual void reinit();
    virtual void clear();
    virtual void update();
    virtual void update(int x, int y, int width, int height);
    virtual void highlight(int x, int y, int width, int height);
    virtual void unhighlight();

protected:
    const int x, y, width, height;
    bool highlighted;
    int highlightX, highlightY, highlightW, highlightH;
    void drawHighlighted();
    static Image *screen;
};

#endif /* VIEW_H */
