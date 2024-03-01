/*
 * $Id: xml.h 2223 2004-09-26 19:36:15Z andrewtaylor $
 */

#ifndef XML_H
#define XML_H

#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/xmlmemory.h>

xmlDocPtr xmlParse(const char *filename);
bool xmlPropExists(xmlNodePtr node, const char *name);
std::string xmlGetPropAsString(xmlNodePtr node, const char *name);
int xmlGetPropAsBool(xmlNodePtr node, const char *name);
int xmlGetPropAsInt(xmlNodePtr node, const char *name);
int xmlGetPropAsEnum(xmlNodePtr node, const char *name, const char *enumValues[]);
int xmlPropCmp(xmlNodePtr node, const char *name, const char *s);
int xmlPropCaseCmp(xmlNodePtr node, const char *name, const char *s);

#endif /* XML_H */
