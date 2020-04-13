#ifndef XMLPARSE_H
#define XMLPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

void zu4_xmlparse_init(const char *filename);
void zu4_xmlparse_deinit();
int zu4_xmlparse_find(char *value, const char *elemname, const char *attrname);

#ifdef __cplusplus
}
#endif

#endif
