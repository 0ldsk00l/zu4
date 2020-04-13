/*
 * $Id: config.cpp 3074 2014-07-29 01:11:25Z darren_janeczek $
 */

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cctype>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>

// we rely on xinclude support
#ifndef LIBXML_XINCLUDE_ENABLED
#error "xinclude not available: libxml2 must be compiled with xinclude support"
#endif

#include "config.h"
#include "error.h"
#include "settings.h"
#include "u4file.h"

using namespace std;

extern bool verbose;
Config *Config::instance = NULL;

const Config *Config::getInstance() {
    if (!instance) {
        xmlRegisterInputCallbacks(&xmlFileMatch, &fileOpen, xmlFileRead, xmlFileClose);
        instance = new Config;
    }
    return instance;
}

ConfigElement Config::getElement(const string &name) const {
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    string path = "/config/" + name;
    context = xmlXPathNewContext(doc);
    result = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(path.c_str()), context);
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
        zu4_error(ZU4_LOG_ERR, "no match for xpath %s\n", path.c_str());

    xmlXPathFreeContext(context);

    if (result->nodesetval->nodeNr > 1)
        zu4_error(ZU4_LOG_WRN, "more than one match for xpath %s\n", path.c_str());

    xmlNodePtr node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result);

    return ConfigElement(node);
}

char DEFAULT_CONFIG_XML_LOCATION[] = "config.xml";
char * Config::CONFIG_XML_LOCATION_POINTER = &DEFAULT_CONFIG_XML_LOCATION[0];

Config::Config() {
    doc = xmlParseFile(Config::CONFIG_XML_LOCATION_POINTER);
    if (!doc) {
    	printf("Failed to read core config.xml. Assuming it is located at '%s'", Config::CONFIG_XML_LOCATION_POINTER);
        zu4_error(ZU4_LOG_ERR, "error parsing config.xml");
    }

    xmlXIncludeProcess(doc);

    if (settings.validateXml && doc->intSubset) {
        string errorMessage;
        xmlValidCtxt cvp;

        if (verbose)
            printf("validating config.xml\n");

        cvp.userData = &errorMessage;
        cvp.error = &accumError;

        // Error changed to not fatal due to regression in libxml2
        if (!xmlValidateDocument(&cvp, doc))
            zu4_error(ZU4_LOG_WRN, "xml validation error:\n%s", errorMessage.c_str());
    }
}


vector<string> Config::getGames() {
    vector<string> result;
    result.push_back("Ultima IV");
    return result;
}

void Config::setGame(const string &name) {
}

void *Config::fileOpen(const char *filename) {
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

void Config::accumError(void *l, const char *fmt, ...) {
    string* errorMessage = static_cast<string *>(l);
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    errorMessage->append(buffer);
}

ConfigElement::ConfigElement(xmlNodePtr xmlNode) : node(xmlNode), name(reinterpret_cast<const char *>(xmlNode->name)) {
}

ConfigElement::ConfigElement(const ConfigElement &e) : node(e.node), name(e.name) {
}

ConfigElement::~ConfigElement() {
}

ConfigElement &ConfigElement::operator=(const ConfigElement &e) {
    if (&e != this) {
        node = e.node;
        name = e.name;
    }
    return *this;
}

/**
 * Returns true if the property exists in the current config element
 */
bool ConfigElement::exists(const std::string &name) const {
    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    bool exists = prop != NULL;
    xmlFree(prop);

    return exists;
}

string ConfigElement::getString(const string &name) const {
    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return "";

    string result(reinterpret_cast<const char *>(prop));
    xmlFree(prop);
    
    return result;
}

int ConfigElement::getInt(const string &name, int defaultValue) const {
    long result;
    xmlChar *prop;

    prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return defaultValue;

    result = strtol(reinterpret_cast<const char *>(prop), NULL, 0);
    xmlFree(prop);

    return static_cast<int>(result);
}

bool ConfigElement::getBool(const string &name) const {
    int result;

    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return false;

    if (xmlStrcmp(prop, reinterpret_cast<const xmlChar *>("true")) == 0)
        result = true;
    else
        result = false;

    xmlFree(prop);

    return result;
}

int ConfigElement::getEnum(const string &name, const char *enumValues[]) const {
    int result = -1, i;
    xmlChar *prop;

    prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return 0;

    for (i = 0; enumValues[i]; i++) {
        if (xmlStrcmp(prop, reinterpret_cast<const xmlChar *>(enumValues[i])) == 0)
        result = i;
    }

    if (result == -1)
        zu4_error(ZU4_LOG_ERR, "invalid enum value for %s: %s", name.c_str(), prop);

    xmlFree(prop);

    return result;
}

vector<ConfigElement> ConfigElement::getChildren() const {
    vector<ConfigElement> result;

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE)
            result.push_back(ConfigElement(child));
    }

    return result;
}

