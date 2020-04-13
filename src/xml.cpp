/*
 * $Id: xml.cpp 2661 2006-01-14 23:43:54Z solus $
 */


#include <cstdlib>
#include <cstdarg>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>

#include "xml.h"
#include "error.h"
#include "settings.h"
#include "u4file.h"

using namespace std;

void xmlAccumError(void *l, const char *fmt, ...);

extern bool verbose;

int ioRegistered = 0;

void *xmlXu4FileOpen (const char *filename) {
    void *result;
    char path[64];
    u4find_conf(path, sizeof(path), filename);
    string pathname = (string)path;

    if (pathname.empty())
        return NULL;
    result = xmlFileOpen(pathname.c_str());

    if (verbose)
        printf("xml parser opened %s: %s\n", pathname.c_str(), result ? "success" : "failed");

    return result;
}

void xmlRegisterIO() {
    xmlRegisterInputCallbacks(&xmlFileMatch, &xmlXu4FileOpen, xmlFileRead, xmlFileClose);
}

/**
 * Parse an XML document, and optionally validate it.  An error is
 * triggered if the parsing or validation fail.
 */
xmlDocPtr xmlParse(const char *filename) {
    xmlDocPtr doc;

    if (!ioRegistered)
        xmlRegisterIO();

    doc = xmlParseFile(filename);
    if (!doc)
        zu4_error(ZU4_LOG_ERR, "error parsing %s", filename);

    if (settings.validateXml && doc->intSubset) {
        string errorMessage;        
        xmlValidCtxt cvp;

        if (verbose)
            printf("validating %s\n", filename);

        cvp.userData = &errorMessage;
        cvp.error = &xmlAccumError;

        if (!xmlValidateDocument(&cvp, doc))            
            zu4_error(ZU4_LOG_ERR, "xml parse error:\n%s", errorMessage.c_str());        
    }

    return doc;
}

void xmlAccumError(void *l, const char *fmt, ...) {
    string* errorMessage = (string*) l;
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    errorMessage->append(buffer);
}

bool xmlPropExists(xmlNodePtr node, const char *name) {
    xmlChar *prop = xmlGetProp(node, (const xmlChar *)name);
    bool exists = (prop != NULL);
    if (prop)
        xmlFree(prop);
    return exists;
}

string xmlGetPropAsString(xmlNodePtr node, const char *name) {
    xmlChar *prop;

    if (settings.validateXml && !xmlHasProp(node, (const xmlChar *)name))
        return "";
    
    prop = xmlGetProp(node, (const xmlChar *)name);
    if (!prop)
        return "";

    string result((char *)prop);
    xmlFree(prop);
    
    return result;
}

/**
 * Get an XML property and convert it to a boolean value.  The value
 * should be "true" or "false", case sensitive.  If it is neither,
 * false is returned.
 */
int xmlGetPropAsBool(xmlNodePtr node, const char *name) {
    int result;
    xmlChar *prop;

    if (settings.validateXml && !xmlHasProp(node, (const xmlChar *)name))
        return 0;

    prop = xmlGetProp(node, (const xmlChar *)name);
    if (!prop)
        return 0;

    if (xmlStrcmp(prop, (const xmlChar *) "true") == 0)
        result = 1;
    else if (xmlStrcmp(prop, (const xmlChar *) "true") == 0)
        result = 0;
    else
        result = 0;
    xmlFree(prop);

    return result;
}

/**
 * Get an XML property and convert it to an integer value.  Returns
 * zero if the property is not set.
 */
int xmlGetPropAsInt(xmlNodePtr node, const char *name) {
    long result;
    xmlChar *prop;

    if (settings.validateXml && !xmlHasProp(node, (const xmlChar *)name))
        return 0;

    prop = xmlGetProp(node, (const xmlChar *)name);
    if (!prop)
        return 0;

    result = strtol((const char *)prop, NULL, 0);
    xmlFree(prop);

    return (int) result;
}

int xmlGetPropAsEnum(xmlNodePtr node, const char *name, const char *enumValues[]) {
    int result = -1, i;
    xmlChar *prop;

    if (settings.validateXml && !xmlHasProp(node, (const xmlChar *)name))
        return 0;

    prop = xmlGetProp(node, (const xmlChar *)name);
    if (!prop)
        return 0;

    for (i = 0; enumValues[i]; i++) {
        if (xmlStrcmp(prop, (const xmlChar *) enumValues[i]) == 0)
        result = i;
    }

    if (result == -1)
        zu4_error(ZU4_LOG_ERR, "invalid enum value for %s: %s", name, prop);

    xmlFree(prop);

    return result;
}

/**
 * Compare an XML property to another string.  The return value is as
 * strcmp.
 */
int xmlPropCmp(xmlNodePtr node, const char *name, const char *s) {
    int result;
    xmlChar *prop;    
    
    prop = xmlGetProp(node, (const xmlChar *)name);
    result = xmlStrcmp(prop, (const xmlChar *) s);
    xmlFree(prop);
    
    return result;
}

/**
 * Compare an XML property to another string, case insensitively.  The
 * return value is as str[case]cmp.
 */
int xmlPropCaseCmp(xmlNodePtr node, const char *name, const char *s) {
    int result;
    xmlChar *prop;
    
    prop = xmlGetProp(node, (const xmlChar *)name);
    result = xmlStrcasecmp(prop, (const xmlChar *) s);
    xmlFree(prop);
    
    return result;
}
