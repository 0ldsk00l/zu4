/*
 * $Id: city.h 3022 2012-03-18 11:32:01Z daniel_santos $
 */

#ifndef CITY_H
#define CITY_H

#include <list>
#include <string>
#include <vector>

using std::string;

class Person;
class Dialogue;

#include "map.h"

struct PersonRole {
    int role;
    int id;
};

typedef std::vector<Person *> PersonList;
typedef std::list<PersonRole*> PersonRoleList;

class City : public Map {
public:
    City();
    ~City();

    // Members
    virtual string getName();
    Person *addPerson(Person *p);
    void addPeople();
    void removeAllPeople();
    Person *personAt(const Coords &coords);

    // Properties
    string name;
    string type;
    PersonList persons;
    string tlk_fname;
    PersonRoleList personroles;    
    std::vector<Dialogue *> extraDialogues;
};

bool isCity(Map *punknown);

#endif
