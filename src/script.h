/**
 * $Id: script.h 3048 2012-06-23 11:50:23Z twschulz $
 */

#ifndef SCRIPT_H
#define SCRIPT_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "types.h"
#include "xml.h"

using std::string;

/**
 * An xml-scripting class. It loads and runs xml scripts that
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
class Script {
public:    
    /**
     * A class that provides information to a script.  It is designed to
     * translate qualifiers and identifiers in a script to another value.
     * Each provider is assigned a qualifier that the script uses to
     * select a provider.  The provider then uses the rest of the information
     * to translate the information to something useful.
     */
    class Provider {
    public:
        virtual ~Provider() {}
        virtual string translate(std::vector<string>& parts) = 0;
    };

private:
    /**
     * A class that represents a script variable
     */ 
    class Variable {
    public:
        Variable();
        Variable(const string &v);
        Variable(const int &v);

        int&    getInt();
        string& getString();
        
        void    setValue(const int &v);
        void    setValue(const string &v);
        void    unset();

        bool    isInt() const;
        bool    isString() const;
        bool    isSet() const;
    
    private:
        int i_val;
        string s_val;
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

    void addProvider(const string &name, Provider *p);
    bool load(const string &filename, const string &baseId, const string &subNodeName = "", const string &subNodeId = "");
    void unload();
    void run(const string &script);
    ReturnCode execute(xmlNodePtr script, xmlNodePtr currentItem = NULL, string *output = NULL);    
    void _continue();
    
    void resetState();
    void setState(State state);
    State getState();    
    
    void setTarget(const string &val);
    void setChoices(const string &val);
    void setVar(const string &name, const string &val);
    void setVar(const string &name, int val);
    void unsetVar(const string &name);
    
    string getTarget();
    InputType getInputType();
    string getInputName();
    string getChoices();
    int getInputMaxLen();
    
private:
    void        translate(string *script);
    xmlNodePtr  find(xmlNodePtr node, const string &script, const string &choice = "", bool _default = false);    
    string      getPropAsStr(std::list<xmlNodePtr>& nodes, const string &prop, bool recursive);
    string      getPropAsStr(xmlNodePtr node, const string &prop, bool recursive = false);
    int         getPropAsInt(std::list<xmlNodePtr>& nodes, const string &prop, bool recursive);
    int         getPropAsInt(xmlNodePtr node, const string &prop, bool recursive = false);
    string      getContent(xmlNodePtr node);   

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
    void mathParseChildren(xmlNodePtr math, string *result);
    int mathValue(const string &str);
    int math(int lval, int rval, string &op);
    bool mathParse(const string &str, int *lval, int *rval, string *op);
    void parseOperation(const string &str, string *lval, string *rval, string *op);
    bool compare(const string &str);
    void funcParse(const string &str, string *funcName, string *contents);

    /*
     * Static variables
     */
private:
    typedef std::map<string, Action> ActionMap;
    static ActionMap action_map;

private:
    void removeCurrentVariable(const string &name);
    xmlDocPtr vendorScriptDoc;
    xmlNodePtr scriptNode;
    FILE *debug;
    
    State state;                    /**< The state the script is in */
    xmlNodePtr currentScript;       /**< The currently running script */
    xmlNodePtr currentItem;         /**< The current position in the script */
    std::list<xmlNodePtr> translationContext;  /**< A list of nodes that make up our translation context */
    string target;                  /**< The name of a target script */
    InputType inputType;            /**< The type of input required */
    string inputName;               /**< The variable in which to place the input (by default, "input") */
    int inputMaxLen;                /**< The maximum length allowed for input */

    string nounName;                /**< The name that identifies a node name of noun nodes */
    string idPropName;              /**< The name of the property that uniquely identifies a noun node
                                         and is used to find a new translation context */
    
    string choices;
    int iterator;   

    std::map<string, Variable*> variables;    
    std::map<string, Provider*> providers;
};

#endif
