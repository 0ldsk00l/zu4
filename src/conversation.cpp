/*
 * $Id: conversation.cpp 3048 2012-06-23 11:50:23Z twschulz $
 */

#include <string.h>
#include <algorithm>

#include "conversation.h"
#include "error.h"
#include "person.h"
#include "random.h"
#include "script.h"

/* Static variable initialization */
const ResponsePart ResponsePart::NONE("<NONE>", "", true);
const ResponsePart ResponsePart::ASK("<ASK>", "", true);
const ResponsePart ResponsePart::END("<END>", "", true);
const ResponsePart ResponsePart::ATTACK("<ATTACK>", "", true);
const ResponsePart ResponsePart::BRAGGED("<BRAGGED>", "", true);
const ResponsePart ResponsePart::HUMBLE("<HUMBLE>", "", true);
const ResponsePart ResponsePart::ADVANCELEVELS("<ADVANCELEVELS>", "", true);
const ResponsePart ResponsePart::HEALCONFIRM("<HEALCONFIRM>", "", true);
const ResponsePart ResponsePart::STARTMUSIC_LB("<STARTMUSIC_LB>", "", true);
const ResponsePart ResponsePart::STARTMUSIC_HW("<STARTMUSIC_HW>", "", true);
const ResponsePart ResponsePart::STOPMUSIC("<STOPMUSIC>", "", true);
const ResponsePart ResponsePart::HAWKWIND("<HAWKWIND>", "", true);
const unsigned int Conversation::BUFFERLEN = 16;

static const char *strwhitespace = "\t\013\014 \n\r";

Response::Response(const string &response) : references(0) {
    add(response);
}

void Response::add(const ResponsePart &part) {
    parts.push_back(part);
}

const vector<ResponsePart> &Response::getParts() const {
    return parts;
}

Response::operator string() const {
    string result;
    for (vector<ResponsePart>::const_iterator i = parts.begin(); i != parts.end(); i++) {
        result += *i;
    }
    return result;
}

Response *Response::addref() {
    references++;
    return this;
}

void Response::release() {
    references--;
    if (references <= 0)
        delete this;
}

ResponsePart::ResponsePart(const string &value, const string &arg, bool command) {
    this->value = value;
    this->arg = arg;
    this->command = command;
}

ResponsePart::operator string() const {
    return value;
}

bool ResponsePart::operator==(const ResponsePart &rhs) const {
    return value == rhs.value;
}

bool ResponsePart::isCommand() const {
    return command;
}

DynamicResponse::DynamicResponse(Response *(*generator)(const DynamicResponse *), const string &param) : 
    Response(""), param(param) {
    this->generator = generator;
    currentResponse = NULL;
}

DynamicResponse::~DynamicResponse() {
    if (currentResponse)
        delete currentResponse;
}

const vector<ResponsePart> &DynamicResponse::getParts() const {
    // blah, must cast away constness
    const_cast<DynamicResponse *>(this)->currentResponse = (*generator)(this);
    return currentResponse->getParts();
}

/*
 * Dialogue::Question class
 */
Dialogue::Question::Question(const string &txt, Response *yes, Response *no) :
    text(txt), yesresp(yes->addref()), noresp(no->addref()) {}

string Dialogue::Question::getText() {
    return text;
}

Response *Dialogue::Question::getResponse(bool yes) {
    if (yes)
        return yesresp;
    return noresp;
}

            
/*
 * Dialogue::Keyword class
 */ 
Dialogue::Keyword::Keyword(const string &kw, Response *resp) :
    keyword(kw), response(resp->addref()) {
    keyword.erase(keyword.find_last_not_of(strwhitespace) + 1);
    keyword.erase(0, keyword.find_first_not_of(strwhitespace));
    transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
}

Dialogue::Keyword::Keyword(const string &kw, const string &resp) :
    keyword(kw), response((new Response(resp))->addref()) {
    keyword.erase(keyword.find_last_not_of(strwhitespace) + 1);
    keyword.erase(0, keyword.find_first_not_of(strwhitespace));
    transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
}

Dialogue::Keyword::~Keyword() {
    response->release();
}

bool Dialogue::Keyword::operator==(const string &kw) const {
    // minimum 4-character "guessing"
    int testLen = (keyword.size() < 4) ? keyword.size() : 4;

    // exception: empty keyword only matches empty string (alias for 'bye')
    if (testLen == 0 && kw.size() > 0)
        return false;

    if (strncasecmp(kw.c_str(), keyword.c_str(), testLen) == 0)
        return true;
    return false;
}

/*
 * Dialogue class 
 */ 

Dialogue::Dialogue()
	: intro(NULL)
	, longIntro(NULL)
	, defaultAnswer(NULL)
	, question(NULL) {
}

Dialogue::~Dialogue() {
    for (KeywordMap::iterator i = keywords.begin(); i != keywords.end(); i++) {
        delete i->second;
    }
}

void Dialogue::addKeyword(const string &kw, Response *response) {
    if (keywords.find(kw) != keywords.end())
        delete keywords[kw];

    keywords[kw] = new Keyword(kw, response);
}

Dialogue::Keyword *Dialogue::operator[](const string &kw) {
    KeywordMap::iterator i = keywords.find(kw);
    
    // If they entered the keyword verbatim, return it!
    if (i != keywords.end())
        return i->second;
    // Otherwise, go find one that fits the description.
    else {            
        for (i = keywords.begin(); i != keywords.end(); i++) {
            if ((*i->second) == kw)
                return i->second;
        }            
    }
    return NULL;
}

const ResponsePart &Dialogue::getAction() const { 
    int prob = zu4_random(0x100);

    /* Does the person turn away from/attack you? */
    if (prob >= turnAwayProb)
        return ResponsePart::NONE;
    else {
        if (attackProb - prob < 0x40)
            return ResponsePart::END;
        else
            return ResponsePart::ATTACK;
    }
}

string Dialogue::dump(const string &arg) {
    string result;
    if (arg == "") {
        result = "keywords:\n";
        for (KeywordMap::iterator i = keywords.begin(); i != keywords.end(); i++) {
            result += i->first + "\n";
        }
    } else {
        if (keywords.find(arg) != keywords.end())
            result = static_cast<string>(*keywords[arg]->getResponse());
    }

    return result;
}

/*
 * Conversation class 
 */ 

Conversation::Conversation() : state(INTRO), script(new Script()) {
}

Conversation::~Conversation() {
    delete script;
}

Conversation::InputType Conversation::getInputRequired(int *bufferlen) {    
    switch (state) {
    case BUY_QUANTITY:
    case SELL_QUANTITY:
        {
            *bufferlen = 2;
            return INPUT_STRING;
        }

    case TALK:
    case BUY_PRICE:
    case TOPIC:
        {
            *bufferlen = BUFFERLEN;
            return INPUT_STRING;
        }

    case GIVEBEGGAR:
        {
            *bufferlen = 2;
            return INPUT_STRING;
        }

    case ASK:
    case ASKYESNO:
        {
            *bufferlen = 3;
            return INPUT_STRING;
        }

    case VENDORQUESTION:
    case BUY_ITEM:
    case SELL_ITEM:
    case CONFIRMATION:
    case CONTINUEQUESTION:
    case PLAYER:
        return INPUT_CHARACTER;

    case ATTACK:
    case DONE:
    case INTRO:
    case FULLHEAL:
    case ADVANCELEVELS:
        return INPUT_NONE;
    }

    zu4_assert(0, "invalid state: %d", state);
    return INPUT_NONE;
}

