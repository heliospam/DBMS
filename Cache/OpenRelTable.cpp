#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>
#include <iostream>
using namespace std;

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

inline bool operator == (const RecId& lhs, const RecId& rhs)
{
    return (lhs.block == rhs.block && lhs.slot == rhs.slot);
}

AttrCacheEntry *createAttrCacheEntryList(int size)
{
	AttrCacheEntry *head = nullptr, *curr = nullptr;
	head = curr = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
	size--;
	while (size--)
	{
		curr->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
		curr = curr->next;
	}
	curr->next = nullptr;

	return head;
}

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  int start = 0, end = 1;
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
  // for(int i = start;i <= end;i++){
  //   cout << "Relation: " << RelCacheTable::relCache[i]->relCatEntry.relName << endl;
  // }
  /************ Setting up tableMetaInfo entries ************/

  // in the tableMetaInfo array
  //   set free = false for RELCAT_RELID and ATTRCAT_RELID
  //   set relname for RELCAT_RELID and ATTRCAT_RELID
  for(int i = 0;i < MAX_OPEN;i++){
    tableMetaInfo[i].free = true;
  }
  for(int i = start;i <= end;i++){
    tableMetaInfo[i].free = false;
    strcpy(tableMetaInfo[i].relName, RelCacheTable::relCache[i]->relCatEntry.relName);
    cout << "tableMetaInfo[" << i << "].relName = " << tableMetaInfo[i].relName << endl;
  }
}

OpenRelTable::~OpenRelTable() {

    // for i from 2 to MAX_OPEN-1:
    // {
    //     if ith relation is still open:
    //     {
    //         // close the relation using openRelTable::closeRel().
    //     }
    // }
    for(int i = 2;i < MAX_OPEN;i++){
      if(tableMetaInfo[i].free == false){
        closeRel(i);
      }
    }
    /**** Closing the catalog relations in the relation cache ****/

    //releasing the relation cache entry of the attribute catalog

    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty == true) {
        cout << "Writing back to the buffer" << endl;

        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        RelCatEntry relCatBuffer;
		    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatBuffer);
        Attribute relCatRecord [RELCAT_NO_ATTRS];
		    RelCacheTable::relCatEntryToRecord(&relCatBuffer, relCatRecord);  

        // declaring an object of RecBuffer class to write back to the buffer
        RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);

        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
        relCatBlock.setRecord(relCatRecord, recId.slot);
        cout << "Writing back to the buffer done" << endl;
    }
    // free the memory dynamically allocated to this RelCacheEntry
    free(RelCacheTable::relCache[ATTRCAT_RELID]);


    //releasing the relation cache entry of the relation catalog

    if(RelCacheTable::relCache[RELCAT_RELID]->dirty == true) {
        cout << "Writing back to the buffer" << endl;

        /* Get the Relation Catalog entry from RelCacheTable::relCache
        Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
        RelCatEntry relCatBuffer;
        RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatBuffer);
        Attribute relCatRecord [RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatBuffer, relCatRecord);
        // declaring an object of RecBuffer class to write back to the buffer
        RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);

        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
        relCatBlock.setRecord(relCatRecord, recId.slot);
        cout << "Writing back to the buffer done" << endl;
    }
    // free the memory dynamically allocated for this RelCacheEntry
    free(RelCacheTable::relCache[RELCAT_RELID]);

    // free the memory allocated for the attribute cache entries of the
    // relation catalog and the attribute catalog
    AttrCacheEntry *head, *t;
    for(int i = 0;i < 2;i++){
      head = AttrCacheTable::attrCache[i];
      for(int j = 0;j < 6;j++){
        t = head->next;
        // moving the dirty attribute cache entries to the buffer
        if(head->dirty){
          cout << "Writing " << i << " " << j << " to the buffer" << endl;
          AttrCatEntry attrCatBuffer;
          AttrCacheTable::getAttrCatEntry(i, j, &attrCatBuffer);
          Attribute attrCatRecord [ATTRCAT_NO_ATTRS];
          AttrCacheTable::attrCatEntryToRecord(&attrCatBuffer, attrCatRecord);
          RecId recId = head->recId;
          RecBuffer attrCatBlock(recId.block);
          attrCatBlock.setRecord(attrCatRecord, recId.slot);
          cout << "Writing " << i << " " << j << " to the buffer done" << endl;
        }
        free(head);
        head = t;
      }
    }
}

int OpenRelTable::getFreeOpenRelTableEntry() {

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
  for(int i = 0;i < MAX_OPEN;i++){
    if(tableMetaInfo[i].free == true){
      return i;
    }
  }

  // if found return the relation id, else return E_CACHEFULL.
  cout << "Error: Cache is full" << endl;
  return E_CACHEFULL;
}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) 
{
  	// traverse through the tableMetaInfo array,
	// find the entry in the Open Relation Table corresponding to relName.*/
	for (int i = 0; i < MAX_OPEN; i++){ 
		if (strcmp(tableMetaInfo[i].relName, relName) == 0 && tableMetaInfo[i].free == false){
			return i;
    }
  }
  	// if found return the relation id, else indicate that the relation do not
  	// have an entry in the Open Relation Table.
	return E_RELNOTOPEN;
}


int OpenRelTable::closeRel(int relId) {
  if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }

  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free == true) {
    return E_RELNOTOPEN;
  }

  /****** Releasing the Relation Cache entry of the relation ******/

  if (RelCacheTable::relCache[relId]->dirty == true)
  {

    /* Get the Relation Catalog entry from RelCacheTable::relCache
    Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
    RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];
    union Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCacheEntry->relCatEntry, relCatRecord);

    // declaring an object of RecBuffer class to write back to the buffer
    RecId recId = RelCacheTable::relCache[relId]->recId;
    RecBuffer relCatBlock(recId.block);

    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    relCatBlock.setRecord(relCatRecord, recId.slot);
  }

  /****** Releasing the Attribute Cache entry of the relation ******/

  // free the memory allocated in the attribute caches which was
  // allocated in the OpenRelTable::openRel() function

  // (because we are not modifying the attribute cache at this stage,
  // write-back is not required. We will do it in subsequent
  // stages when it becomes needed)
  AttrCacheEntry *head = AttrCacheTable::attrCache[relId];
  AttrCacheEntry *t;
  while(head != nullptr){
    t = head;
    head = head->next;
    free(t);
  }


  /****** Set the Open Relation Table entry of the relation as free ******/

  // update `metainfo` to set `relId` as a free slot
  tableMetaInfo[relId].free = true;

  return SUCCESS;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::getRelId(relName);
  if(ret != E_RELNOTOPEN){
    // (checked using OpenRelTable::getRelId())
    return ret;
    // return that relation id;
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
  ret = OpenRelTable::getFreeOpenRelTableEntry();

  if (ret == E_CACHEFULL){
    return E_CACHEFULL;
  }

  // let relId be used to store the free slot.
  int relId;
  relId = ret; 
  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/
  Attribute attrval;
  strcpy(attrval.sVal, relName);
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId;
  relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, attrval, EQ);
  if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
  RecBuffer relationBuffer(relcatRecId.block);
  Attribute relationRecord[RELCAT_NO_ATTRS];
  RelCacheEntry *relCacheEntry = nullptr;

  relationBuffer.getRecord(relationRecord, relcatRecId.slot);

  relCacheEntry = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  RelCacheTable::recordToRelCatEntry(relationRecord, &relCacheEntry->relCatEntry);

  relCacheEntry->recId.block = relcatRecId.block;
  relCacheEntry->recId.slot = relcatRecId.slot;

  RelCacheTable::relCache[relId] = relCacheEntry;

  

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.

  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  // {
  //     /* let attrcatRecId store a valid record id an entry of the relation, relName,
  //     in the Attribute Catalog.*/
  //     RecId attrcatRecId;

  //     /* read the record entry corresponding to attrcatRecId and create an
  //     Attribute Cache entry on it using RecBuffer::getRecord() and
  //     AttrCacheTable::recordToAttrCatEntry().
  //     update the recId field of this Attribute Cache entry to attrcatRecId.
  //     add the Attribute Cache entry to the linked list of listHead .*/
  //     // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
  // }
  AttrCacheEntry *t, *head = nullptr;
  Attribute attrcatRecord[ATTRCAT_NO_ATTRS];
  int numAttr = relCacheEntry->relCatEntry.numAttrs;
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
  // cout << "Check point 1" << endl;

  for(int i = 0;i < numAttr;i++){
    RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, const_cast<char*>(ATTRCAT_ATTR_RELNAME), attrval, EQ);
    RecBuffer attrcatBuffer(attrcatRecId.block);
    attrcatBuffer.getRecord(attrcatRecord, attrcatRecId.slot);
    t = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrcatRecord, &t->attrCatEntry);
    t->recId.block = attrcatRecId.block;
    t->recId.slot = attrcatRecId.slot;
    t->next = nullptr;
    if(i == 0){
      head = t;
      AttrCacheTable::attrCache[relId] = head;
    }
    else{
      head->next = t;
      head = head->next;
    }
  }
  // cout << "Check point 2" << endl;
  // set the relIdth entry of the AttrCacheTable to listHead.

  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName, relName);

  return relId;
}
