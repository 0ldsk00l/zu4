/*
 * $Id: event.h 2898 2011-04-29 20:58:31Z twschulz $
 */

#ifndef EVENT_H
#define EVENT_H

#ifdef __cplusplus // FIXME: Dirty hack to allow C files to know inputs

#include <list>
#include <string>
#include <vector>

#include "controller.h"
#include "types.h"

using std::string;

#define eventHandler (EventHandler::getInstance())
#endif

#define U4_UP           '['
#define U4_DOWN         '/'
#define U4_LEFT         ';'
#define U4_RIGHT        '\''
#define U4_BACKSPACE    8
#define U4_TAB          9
#define U4_SPACE        ' '
#define U4_ESC          27
#define U4_ENTER        13
#define U4_ALT          128
#define U4_KEYPAD_ENTER 271
#define U4_META         323
#define U4_FKEY         282
#define U4_RIGHT_SHIFT  303
#define U4_LEFT_SHIFT   304
#define U4_RIGHT_CTRL   305
#define U4_LEFT_CTRL    306
#define U4_RIGHT_ALT    307
#define U4_LEFT_ALT     308
#define U4_RIGHT_META   309
#define U4_LEFT_META    310

#ifdef __cplusplus

extern int eventTimerGranularity;

struct _MouseArea;
class EventHandler;
class TextView;

/**
 * A class for handling keystrokes. 
 */
class KeyHandler {
public:
    virtual ~KeyHandler() {}

    /* Typedefs */
    typedef bool (*Callback)(int, void*);

    /** Additional information to be passed as data param for read buffer key handler */
    typedef struct ReadBuffer {
        int (*handleBuffer)(string*);
        string *buffer;
        int bufferLen;
        int screenX, screenY;
    } ReadBuffer;

    /** Additional information to be passed as data param for get choice key handler */
    typedef struct GetChoice {
        string choices;
        int (*handleChoice)(int);
    } GetChoice;

    /* Constructors */
    KeyHandler(Callback func, void *data = NULL, bool asyncronous = true);
    
    /* Static functions */    
    static int setKeyRepeat(int delay, int interval);
    static bool globalHandler(int key);    

    /* Static default key handler functions */
    static bool defaultHandler(int key, void *data);
    static bool ignoreKeys(int key, void *data);

    /* Operators */
    bool operator==(Callback cb) const;
    
    /* Member functions */    
    bool handle(int key); 
    virtual bool isKeyIgnored(int key);

protected:
    Callback handler;
    bool async;
    void *data;
};

/**
 * A controller that wraps a keyhander function.  Keyhandlers are
 * deprecated -- please use a controller instead.
 */
class KeyHandlerController : public Controller {
public:
    KeyHandlerController(KeyHandler *handler);
    ~KeyHandlerController();

    virtual bool keyPressed(int key);
    KeyHandler *getKeyHandler();

private:
    KeyHandler *handler;
};

/**
 * A controller to read a string, terminated by the enter key.
 */
class ReadStringController : public WaitableController<string> {
public:
    ReadStringController(int maxlen, int screenX, int screenY, const string &accepted_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 \n\r\010");
    ReadStringController(int maxlen, TextView *view, const string &accepted_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 \n\r\010");
    virtual bool keyPressed(int key);

    static string get(int maxlen, int screenX, int screenY, EventHandler *eh = NULL);
    static string get(int maxlen, TextView *view, EventHandler *eh = NULL);

protected:
    int maxlen, screenX, screenY;
    TextView *view;
    string accepted;
};

/**
 * A controller to read a integer, terminated by the enter key.
 * Non-numeric keys are ignored.
 */
class ReadIntController : public ReadStringController {
public:
    ReadIntController(int maxlen, int screenX, int screenY);

    static int get(int maxlen, int screenX, int screenY, EventHandler *eh = NULL);
    int getInt() const;
};

/**
 * A controller to read a single key from a provided list.
 */
class ReadChoiceController : public WaitableController<int> {
public:
    ReadChoiceController(const string &choices);
    virtual bool keyPressed(int key);

    static char get(const string &choices, EventHandler *eh = NULL);

protected:
    string choices;
};

/**
 * A controller to read a direction enter with the arrow keys.
 */
class ReadDirController : public WaitableController<Direction> {
public:    
    ReadDirController();
    virtual bool keyPressed(int key);    
};

/**
 * A controller to pause for a given length of time, ignoring all
 * keyboard input.
 */
class WaitController : public Controller {
public:
    WaitController(unsigned int cycles);
    virtual bool keyPressed(int key);
    virtual void timerFired();

    void wait();
    void setCycles(int c);

private:
    unsigned int cycles;
    unsigned int current;
};

/**
 * A class for handling timed events.
 */ 
class TimedEvent {
public:
    /* Typedefs */
    typedef std::list<TimedEvent*> List;
    typedef void (*Callback)(void *);

    /* Constructors */
    TimedEvent(Callback callback, int interval, void *data = NULL);

    /* Member functions */
    Callback getCallback() const;
    void *getData();
    void tick();
    
    /* Properties */
protected:    
    Callback callback;
    void *data;
    int interval;
    int current;
};

/**
 * A class for managing timed events
 */ 
class TimedEventMgr {
public:
    /* Typedefs */
    typedef TimedEvent::List List;    

    /* Constructors */
    TimedEventMgr(int baseInterval);
    ~TimedEventMgr();

    /* Static functions */
    static unsigned int callback(unsigned int interval, void *param);

    /* Member functions */
    bool isLocked() const;      /**< Returns true if the event list is locked (in use) */    

    void add(TimedEvent::Callback callback, int interval, void *data = NULL);
    List::iterator remove(List::iterator i);
    void remove(TimedEvent* event);
    void remove(TimedEvent::Callback callback, void *data = NULL);
    void tick();
    void stop();
    void start();
    
    void reset(unsigned int interval);     /**< Re-initializes the event manager to a new base interval */

private:
    void lock();                /**< Locks the event list */
    void unlock();              /**< Unlocks the event list */

    /* Properties */
protected:
    /* Static properties */
    static unsigned int instances;

    void *id;
    int baseInterval;
    bool locked;
    List events;
    List deferredRemovals;
};

typedef void(*updateScreenCallback)(void);
/**
 * A class for handling game events. 
 */
class EventHandler {
public:    
    /* Typedefs */
    typedef std::list<_MouseArea*> MouseAreaList;    

    /* Constructors */
    EventHandler();    

    /* Static functions */    
    static EventHandler *getInstance();
    static void sleep(unsigned int usec);
    static void wait_msecs(unsigned int msecs);
    static void wait_cycles(unsigned int cycles);
    static void setControllerDone(bool exit = true);
    static bool getControllerDone();
    static void end();
    static bool timerQueueEmpty();

    /* Member functions */
    TimedEventMgr* getTimer();

    /* Event functions */    
    void run();
    void setScreenUpdate(void (*updateScreen)(void));

    /* Controller functions */
    Controller *pushController(Controller *c);
    Controller *popController();
    Controller *getController() const;
    void setController(Controller *c);

    /* Key handler functions */
    void pushKeyHandler(KeyHandler kh);
    void popKeyHandler();
    KeyHandler *getKeyHandler() const;
    void setKeyHandler(KeyHandler kh);

    /* Mouse area functions */
    void pushMouseAreaSet(_MouseArea *mouseAreas);
    void popMouseAreaSet();
    _MouseArea* getMouseAreaSet() const;
    _MouseArea* mouseAreaForPoint(int x, int y);

protected:    
    static bool controllerDone;
    static bool ended;
    TimedEventMgr timer;
    std::vector<Controller *> controllers;
    MouseAreaList mouseAreaSets;
    updateScreenCallback updateScreen;

private:
    static EventHandler *instance;
};
#endif // ifdef __cplusplus
#endif
