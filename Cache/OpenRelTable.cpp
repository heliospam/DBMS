#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>
#include <iostream>
using namespace std;

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  int start = 0, end = 2;
  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)
  RecBuffer relCatBlock(RELCAT_BLOCK);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  struct RelCacheEntry* relCacheEntry = nullptr;

  for(int i = start;i <= end;i++){
    relCatBlock.getRecord(relCatRecord, i);
    relCacheEntry = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
    relCacheEntry->recId.block = RELCAT_BLOCK;
    relCacheEntry->recId.slot = i;
    RelCacheTable::relCache[i] = relCacheEntry;
  }

  cout << "Relation Cache Done!" << endl;
/************ Setting up Attribute cache entries ************/
// (we need to populate attribute cache with entries for the relation catalog
//  and attribute catalog.)

  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  struct AttrCacheEntry *head, *t;
  int slot = 0;

  for(int i = start;i <= end;i++){
    int numAttrs = RelCacheTable::relCache[i]->relCatEntry.numAttrs;
    head = nullptr;
    for(int j = 0;j < numAttrs;j++, slot++){
        attrCatBlock.getRecord(attrCatRecord, slot);
        t = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &t->attrCatEntry);
        t->recId.block = ATTRCAT_BLOCK;
        t->recId.slot = slot;
        t->next = nullptr;
        if(j == 0){
            head = t;
            AttrCacheTable::attrCache[i] = head;
        }
        else{
            head->next = t;
            head = head->next;
        }
    }
  }
  cout << "Attribute Cache Done!" << endl;
  for(int i = start;i <= end;i++){
    cout << "Relation: " << RelCacheTable::relCache[i]->relCatEntry.relName << endl;
  }
  for(int i = start;i <= end;i++){
    AttrCacheEntry* head = AttrCacheTable::attrCache[i];
    while(head != nullptr){
      cout << "  " << head->attrCatEntry.attrName << ": " << head->attrCatEntry.attrType << endl;
      head = head->next;
    }
  }
}

OpenRelTable::~OpenRelTable() {
  // free all the memory that you allocated in the constructor
}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  // if relname is RELCAT_RELNAME, return RELCAT_RELID
  if(strcmp(relName, RELCAT_RELNAME) == 0) {
    return RELCAT_RELID;
  }
  // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID
  if(strcmp(relName, ATTRCAT_RELNAME) == 0) {
    return ATTRCAT_RELID;
  }
  if (strcmp(relName, "Students") == 0){
    return 2;
  }
  return E_RELNOTOPEN;
}