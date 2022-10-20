/**
 * $Id: script.h 3048 2012-06-23 11:50:23Z twschulz $
 */

#ifndef SCRIPT_H
#define SCRIPT_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "xml.h"

/**
 * An xml-scripting struct. It loads and runs xml scripts that
 * take information and interact with the game environment itself.
 * Currently, it is mainly useful for writing vendor code; however,
 * it should be possible to write scripts for other parts of the
 * game.
 *
 * @todo
 * <ul>
 *      <li>Strip vendor-specific code from the language</li>
 *      <li>Fill in some of the missing integration with the game</li>
 * </ul>
 */
struct Script {
public:
    /**
     * A struct that provides information to a script.  It is designed to
     * translate qualifiers and identifiers in a script to another value.
     * Each provider is assigned a qualifier that the script uses to
     * select a provider.  The provider then uses the rest of the information
     * to translate the information to something useful.
     */
    struct Provider {
    public:
        virtual ~Provider() {}
        virtual std::string translate(std::vector<std::string>& parts) = 0;
    };

private:
    /**
     * A struct that represents a script variable
     */
    struct Variable {
    public:
        Variable();
        Variable(const std::string &v);
        Variable(const int &v);

        int&    getInt();
        std::string& getString();

        void    setValue(const int &v);
        void    setValue(const std::string &v);
        void    unset();

        bool    isInt() const;
        bool    isString() const;
        bool    isSet() const;

    private:
        int i_val;
        std::string s_val;
        bool set;
    };

public:
    /**
     * A script return code
     */
    enum ReturnCode {
        RET_OK,
        RET_REDIRECTED,
        RET_STOP
    };

    /**
     * The current state of the script
     */
    enum State {
        STATE_UNLOADED,
        STATE_NORMAL,
        STATE_DONE,
        STATE_INPUT
    };

    /**
     * The type of input the script is requesting
     */
    enum InputType {
        INPUT_CHOICE,
        INPUT_NUMBER,
        INPUT_STRING,
        INPUT_DIRECTION,
        INPUT_PLAYER,
        INPUT_KEYPRESS
    };

    /**
     * The action that the script is taking
     */
    enum Action {
        ACTION_SET_CONTEXT,
        ACTION_UNSET_CONTEXT,
        ACTION_END,
        ACTION_REDIRECT,
        ACTION_WAIT_FOR_KEY,
        ACTION_WAIT,
        ACTION_STOP,
        ACTION_INCLUDE,
        ACTION_FOR_LOOP,
        ACTION_RANDOM,
        ACTION_MOVE,
        ACTION_SLEEP,
        ACTION_CURSOR,
        ACTION_PAY,
        ACTION_IF,
        ACTION_INPUT,
        ACTION_ADD,
        ACTION_LOSE,
        ACTION_HEAL,
        ACTION_CAST_SPELL,
        ACTION_DAMAGE,
        ACTION_KARMA,
        ACTION_MUSIC,
        ACTION_SET_VARIABLE,
        ACTION_ZTATS
    };

    Script();
    ~Script();

    void addProvider(const std::string &name, Provider *p);
    bool load(const std::string &filename, const std::string &baseId, const std::string &subNodeName = "", const std::string &subNodeId = "");
    void unload();
    void run(const std::string &script);
    ReturnCode execute(xmlNodePtr script, xmlNodePtr currentItem = NULL, std::string *output = NULL);
    void _continue();

    void resetState();
    void setState(State state);
    State getState();

    void setTarget(const std::string &val);
    void setChoices(const std::string &val);
    void setVar(const std::string &name, const std::string &val);
    void setVar(const std::string &name, int val);
    void unsetVar(const std::string &name);

    std::string getTarget();
    InputType getInputType();
    std::string getInputName();
    std::string getChoices();
    int getInputMaxLen();

private:
    void        translate(std::string *script);
    xmlNodePtr  find(xmlNodePtr node, const std::string &script, const std::string &choice = "", bool _default = false);
    std::string getPropAsStr(std::list<xmlNodePtr>& nodes, const std::string &prop, bool recursive);
    std::string getPropAsStr(xmlNodePtr node, const std::string &prop, bool recursive = false);
    int         getPropAsInt(std::list<xmlNodePtr>& nodes, const std::string &prop, bool recursive);
    int         getPropAsInt(xmlNodePtr node, const std::string &prop, bool recursive = false);
    std::string getContent(xmlNodePtr node);

    /*
     * Action Functions
     */
    ReturnCode pushContext(xmlNodePtr script, xmlNodePtr current);
    ReturnCode popContext(xmlNodePtr script, xmlNodePtr current);
    ReturnCode end(xmlNodePtr script, xmlNodePtr current);
    ReturnCode waitForKeypress(xmlNodePtr script, xmlNodePtr current);
    ReturnCode redirect(xmlNodePtr script, xmlNodePtr current);
    ReturnCode include(xmlNodePtr script, xmlNodePtr current);
    ReturnCode wait(xmlNodePtr script, xmlNodePtr current);
    ReturnCode forLoop(xmlNodePtr script, xmlNodePtr current);
    ReturnCode random(xmlNodePtr script, xmlNodePtr current);
    ReturnCode move(xmlNodePtr script, xmlNodePtr current);
    ReturnCode sleep(xmlNodePtr script, xmlNodePtr current);
    ReturnCode cursor(xmlNodePtr script, xmlNodePtr current);
    ReturnCode pay(xmlNodePtr script, xmlNodePtr current);
    ReturnCode _if(xmlNodePtr script, xmlNodePtr current);
    ReturnCode input(xmlNodePtr script, xmlNodePtr current);
    ReturnCode add(xmlNodePtr script, xmlNodePtr current);
    ReturnCode lose(xmlNodePtr script, xmlNodePtr current);
    ReturnCode heal(xmlNodePtr script, xmlNodePtr current);
    ReturnCode castSpell(xmlNodePtr script, xmlNodePtr current);
    ReturnCode damage(xmlNodePtr script, xmlNodePtr current);
    ReturnCode karma(xmlNodePtr script, xmlNodePtr current);
    ReturnCode music(xmlNodePtr script, xmlNodePtr current);
    ReturnCode setVar(xmlNodePtr script, xmlNodePtr current);
    ReturnCode setId(xmlNodePtr script, xmlNodePtr current);
    ReturnCode ztats(xmlNodePtr script, xmlNodePtr current);

    /*
     * Math and comparison functions
     */
    void mathParseChildren(xmlNodePtr math, std::string *result);
    int mathValue(const std::string &str);
    int math(int lval, int rval, std::string &op);
    bool mathParse(const std::string &str, int *lval, int *rval, std::string *op);
    void parseOperation(const std::string &str, std::string *lval, std::string *rval, std::string *op);
    bool compare(const std::string &str);
    void funcParse(const std::string &str, std::string *funcName, std::string *contents);

    /*
     * Static variables
     */
private:
    typedef std::map<std::string, Action> ActionMap;
    static ActionMap action_map;

private:
    void removeCurrentVariable(const std::string &name);
    xmlDocPtr vendorScriptDoc;
    xmlNodePtr scriptNode;
    FILE *debug;

    State state;                    /**< The state the script is in */
    xmlNodePtr currentScript;       /**< The currently running script */
    xmlNodePtr currentItem;         /**< The current position in the script */
    std::list<xmlNodePtr> translationContext;  /**< A list of nodes that make up our translation context */
    std::string target;             /**< The name of a target script */
    InputType inputType;            /**< The type of input required */
    std::string inputName;          /**< The variable in which to place the input (by default, "input") */
    int inputMaxLen;                /**< The maximum length allowed for input */

    std::string nounName;           /**< The name that identifies a node name of noun nodes */
    std::string idPropName;         /**< The name of the property that uniquely identifies a noun node
                                         and is used to find a new translation context */

    std::string choices;
    int iterator;

    std::map<std::string, Variable*> variables;
    std::map<std::string, Provider*> providers;
};

#endif
