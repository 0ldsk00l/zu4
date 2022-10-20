/*
 * $Id: city.h 3022 2012-03-18 11:32:01Z daniel_santos $
 */

#ifndef CITY_H
#define CITY_H

#include <list>
#include <string>
#include <vector>

struct Person;
struct Dialogue;

#include "map.h"

struct PersonRole {
    int role;
    int id;
};

typedef std::vector<Person *> PersonList;
typedef std::list<PersonRole*> PersonRoleList;

struct City : public Map {
public:
    City();
    ~City();

    // Members
    virtual std::string getName();
    Person *addPerson(Person *p);
    void addPeople();
    void removeAllPeople();
    Person *personAt(const Coords &coords);

    // Properties
    std::string name;
    std::string type;
    PersonList persons;
    std::string tlk_fname;
    PersonRoleList personroles;
    std::vector<Dialogue *> extraDialogues;
};

bool isCity(Map *punknown);

#endif
