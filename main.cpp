#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
using namespace std;

// void printBuffer(unsigned char *buffer, int size) {
//   for (int i = 0; i < size; i++) {
//     cout << (int)buffer[i] << endl; 
//   }
// }

// void printAttrCat(){
//   // create objects for the relation catalog and attribute catalog
//   RecBuffer relCatBuffer(RELCAT_BLOCK);
//   RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

//   HeadInfo relCatHeader;
//   HeadInfo attrCatHeader;

//   // load the headers of both the blocks into relCatHeader and attrCatHeader.
//   // (we will implement these functions later)
//   relCatBuffer.getHeader(&relCatHeader);
//   attrCatBuffer.getHeader(&attrCatHeader);
  
//   int slot = 0;
//   for (int i = 0;i < relCatHeader.numEntries;i++) {

//     Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

//     relCatBuffer.getRecord(relCatRecord, i);

//     printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
//     cout << "No.of entries in relation is: " << relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal << endl;
//     int numAttrs = relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    
//     for (int j = 0;j < numAttrs;j++, slot++) {

//       // declare attrCatRecord and load the attribute catalog entry into it
//       Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//       attrCatBuffer.getRecord(attrCatRecord, slot);

//       // cout << attrCatHeader.rblock << endl;

//       if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) 
//       {
//         const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
//         printf(" %d  %s: %s\n",j, attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
//       }
//       if(slot == attrCatHeader.numSlots - 1) {
//         attrCatBuffer = RecBuffer(attrCatHeader.rblock);
//         attrCatBuffer.getHeader(&attrCatHeader);
//         slot = -1;
//       }
//     }
//     printf("\n");
//   }
// }

// void updateAttr(const char* relName, const char* oldAttrName, const char* newAttrName){
//   RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
//   HeadInfo attrCatHeader;
//   attrCatBuffer.getHeader(&attrCatHeader);

//   int slot = 0;
//   // i is the record index in the attribute catalog
//   for (;slot < attrCatHeader.numEntries;slot++) {

//     Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//     attrCatBuffer.getRecord(attrCatRecord, slot);

//     if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName) == 0) 
//     {
//       if(strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldAttrName) == 0) {
//         strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newAttrName);
//         attrCatBuffer.setRecord(attrCatRecord, slot);
//         cout << "Attribute name updated successfully\n" << endl;
//         break;
//       }
//     }
//     if(slot == attrCatHeader.numSlots - 1) {
//       attrCatBuffer = RecBuffer(attrCatHeader.rblock);
//       attrCatBuffer.getHeader(&attrCatHeader);
//       slot = -1;
//     }
//   }
// }

// int main(int argc, char *argv[]) {
//   Disk disk_run;
//   StaticBuffer buffer; 
//   OpenRelTable cache;

//   /*
//   for i = 0 and i = 1 (i.e RELCAT_RELID and ATTRCAT_RELID)
//       get the relation catalog entry using RelCacheTable::getRelCatEntry()
//       printf("Relation: %s\n", relname);

//       for j = 0 to numAttrs of the relation - 1
//           get the attribute catalog entry for (rel-id i, attribute offset j)
//            in attrCatEntry using AttrCacheTable::getAttrCatEntry()

//           printf("  %s: %s\n", attrName, attrType);
//   */
//  for(int i = 0;i <= 2;i++){
//   RelCatEntry relCatEntry;
//   RelCacheTable::getRelCatEntry(i, &relCatEntry);
//   cout << "Relation: " << relCatEntry.relName << endl;
//   for(int j = 0;j < relCatEntry.numAttrs;j++){
//     AttrCatEntry attrCatEntry;
//     AttrCacheTable::getAttrCatEntry(i, j, &attrCatEntry);
//     const char *attrType = attrCatEntry.attrType == NUMBER ? "NUM" : "STR";
//     printf("  %s: %s\n", attrCatEntry.attrName, attrType);
//   }
//  }
//   return 0;
// }

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;
  cout << "Welcome to MyNitcBase" << endl;
  return FrontendInterface::handleFrontend(argc, argv);
}