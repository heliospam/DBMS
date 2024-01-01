#include "BlockAccess.h"

#include <cstring>
#include <iostream>
using namespace std;

bool operator == (RecId a, RecId b) {
	return (a.block == b.block && a.slot == b.slot);
}

bool operator != (RecId a, RecId b) {
	return (a.block != b.block || a.slot != b.slot);
}

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
	// get the previous search index of the relation relId from the relation cache
	// (use RelCacheTable::getSearchIndex() function)
	RecId prevRecId;
	RelCacheTable::getSearchIndex(relId, &prevRecId);

	// let block and slot denote the record id of the record being currently checked
	int block = -1, slot = -1;

	// if the current search index record is invalid(i.e. both block and slot = -1)
	if (prevRecId.block == -1 && prevRecId.slot == -1)
	{
		//* no hits from previous search; 
		//* search should start from the first record itself

		// get the first record block of the relation from the relation cache
		// (use RelCacheTable::getRelCatEntry() function of Cache Layer)
		RelCatEntry relCatBuffer;
		RelCacheTable::getRelCatEntry(relId, &relCatBuffer);

		// block = first block of the relation,
		// slot = 0 (start at the first slot)
		block = relCatBuffer.firstBlk, slot = 0;
	}
	else
	{
		//* there is a hit from previous search; search should start from
		//* the record next to the search index record

		// block = search index's block
		// slot = search index's slot + 1
		block = prevRecId.block, slot = prevRecId.slot + 1;
	}

	/* The following code searches for the next record in the relation
	   that satisfies the given condition:
		* "We start from the record id (block, slot) and iterate over the remaining
		* records of the relation"
	*/

	RelCatEntry relCatBuffer;
	RelCacheTable::getRelCatEntry(relId, &relCatBuffer);
	while (block != -1)
	{
		//  create a RecBuffer object for block (use RecBuffer Constructor for existing block)
		RecBuffer recBuffer(block);

		//  get header of the block using RecBuffer::getHeader() function
		HeadInfo blockHeader;
		recBuffer.getHeader(&blockHeader);

		//  get slot map of the block using RecBuffer::getSlotMap() function
		unsigned char slotMap[blockHeader.numSlots];
		recBuffer.getSlotMap(slotMap);

		// If slot >= the number of slots per block(i.e. no more slots in this block)
		if (slot >= relCatBuffer.numSlotsPerBlk)
		{
			//  update block = right block of block, update slot = 0
			block = blockHeader.rblock, slot = 0;
			continue; // continue to the beginning of this while loop
		}

		// if slot is free skip the loop
		// (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
		if (slotMap[slot] == SLOT_UNOCCUPIED)
		{
			slot++;
			continue;
		}

		//  get the record with id (block, slot) using RecBuffer::getRecord()
		Attribute record[blockHeader.numAttrs];
		recBuffer.getRecord(record, slot);

		//  compare record's attribute value to the the given attrVal as below:
		//* firstly get the attribute offset for the attrName attribute
		//* from the attribute cache entry of the relation using
		//* AttrCacheTable::getAttrCatEntry()

		AttrCatEntry attrCatBuffer;
		AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuffer);

		// use the attribute offset to get the value of the attribute from current record
		int attrOffset = attrCatBuffer.offset;

		// will store the difference between the attributes 
		// set cmpVal using compareAttrs()
		int cmpVal = compareAttrs(record[attrOffset], attrVal, attrCatBuffer.attrType); 

		/* 
		 check whether this record satisfies the given condition.
		* It is determined based on the output of previous comparison and the op value received.
		* The following code sets the cond variable if the condition is satisfied.
		*/
		if (
			(op == NE && cmpVal != 0) || // if op is "not equal to"
			(op == LT && cmpVal < 0) ||	 // if op is "less than"
			(op == LE && cmpVal <= 0) || // if op is "less than or equal to"
			(op == EQ && cmpVal == 0) || // if op is "equal to"
			(op == GT && cmpVal > 0) ||	 // if op is "greater than"
			(op == GE && cmpVal >= 0)	 // if op is "greater than or equal to"
		)
		{
			//  set the search index in the relation cache as
			//  the record id of the record that satisfies the given condition
			// (use RelCacheTable::setSearchIndex function)
			RecId newRecId = {block, slot};
			RelCacheTable::setSearchIndex(relId, &newRecId);

			return RecId{block, slot};
		}

		slot++;
	}

	//! no record in the relation with Id relid satisfies the given condition
	return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    //  reset the relId of the relation catalog using RelCacheTable::resetSearchIndex() 
	RelCacheTable::resetSearchIndex(RELCAT_RELID);

	//  set newRelName with newName
    Attribute newRelName;    
	strcpy(newRelName.sVal, newName);

    //  search the relation catalog for an entry with "RelName" = newRelName
	RecId relId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, newRelName, EQ);

    //! If relation with name newName already exists (result of linearSearch is not {-1, -1})
	if (relId != RecId{-1, -1}){
        cout << "Relation with name " << newName << " already exists\n";
        return E_RELEXIST;
    }


    // reset the relId of the relation catalog using RelCacheTable::resetSearchIndex)
	RelCacheTable::resetSearchIndex(RELCAT_RELID);

	// set oldRelName with oldName
    Attribute oldRelName;
	strcpy(oldRelName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelName
	relId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, oldRelName, EQ);

    //! If relation with name oldName does not exist (result of linearSearch is {-1, -1})
	if (relId.block == -1 && relId.slot == -1){
        return E_RELNOTEXIST;
    }

    //  get the relation catalog record of the relation to rename using a RecBuffer
    //  on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
	RecBuffer relCatBlock (RELCAT_BLOCK);
	
	Attribute relCatRecord [RELCAT_NO_ATTRS];
	relCatBlock.getRecord(relCatRecord, relId.slot);

    //  update the relation name attribute in the record with newName.
	strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, newName);

    //  set back the record value using RecBuffer.setRecord
	relCatBlock.setRecord(relCatRecord, relId.slot);

	//  update all the attribute catalog entries in the attribute catalog corresponding
	//  to the relation with relation name oldName to the relation name newName

    // reset the relId of the attribute catalog using RelCacheTable::resetSearchIndex()
	RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    //for i = 0 to numberOfAttributes :
	for (int attrIndex = 0; attrIndex < relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; attrIndex++) {
		//    linearSearch on the attribute catalog for relName = oldRelName
		//    get the record using RecBuffer.getRecord
		relId = BlockAccess::linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, oldRelName, EQ);
		RecBuffer attrCatBlock (relId.block);

		Attribute attrCatRecord [ATTRCAT_NO_ATTRS];
		attrCatBlock.getRecord(attrCatRecord, relId.slot);

		//    update the relName field in the record to newName
		//    set back the record using RecBuffer.setRecord

		strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
		attrCatBlock.setRecord(attrCatRecord, relId.slot);
	}

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    // reset the relId of the relation catalog using RelCacheTable::resetSearchIndex()
	RelCacheTable::resetSearchIndex(RELCAT_RELID);

	// set relNameAttr to relName
    Attribute relNameAttr;
	strcpy(relNameAttr.sVal, relName);

	// Search for the relation with name relName in relation catalog using linearSearch()
	RecId relId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    
	//! If relation with name relName does not exist (search returns {-1,-1})
	if (relId == RecId{-1, -1})
       return E_RELNOTEXIST;
	
    // reset the relId of the attribute catalog using RelCacheTable::resetSearchIndex()
	RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    // declare variable attrToRenameRecId used to store the attr-cat recId of the attribute to rename
    RecId attrToRenameRecId{-1, -1};
    // Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    //  iterate over all Attribute Catalog Entry record corresponding to the
    //  relation to find the required attribute
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
		relId = BlockAccess::linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
		if (relId == RecId{-1, -1}) break;

        //  Get the record from the attribute catalog using 
		//  RecBuffer.getRecord into attrCatEntryRecord
		RecBuffer attrCatBlock (relId.block);

		Attribute attrCatRecord [ATTRCAT_NO_ATTRS];
		attrCatBlock.getRecord(attrCatRecord, relId.slot);

        // if attrCatEntryRecord.attrName = oldName
		if (strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0){
			attrToRenameRecId = relId;
			break;
		}

        //! if attrCatEntryRecord.attrName = newName
		if (strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
            return E_ATTREXIST;
    }

	// if attribute with the old name does not exist
    if (attrToRenameRecId == RecId{-1, -1})
        return E_ATTRNOTEXIST;

    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord

	RecBuffer attrCatBlock (attrToRenameRecId.block);
	Attribute attrCatRecord [ATTRCAT_NO_ATTRS];
	attrCatBlock.getRecord(attrCatRecord, attrToRenameRecId.slot);
	
	strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName );
	attrCatBlock.setRecord(attrCatRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
	RelCatEntry relCatEntry;
	RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

	// block number of the last element in the linked list = -1 
    int prevBlockNum = -1;

	// Traversing the linked list of existing record blocks of the relation
	// until a free slot is found OR until the end of the list is reached

    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
		RecBuffer recBuffer (blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
		HeadInfo blockHeader;
		recBuffer.getHeader(&blockHeader);

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
		int numSlots = blockHeader.numSlots;
		unsigned char slotMap[numSlots];
		recBuffer.getSlotMap(slotMap);

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
		
		for (int i = 0;i < numSlots;i++) {
        	// if a free slot is found, set rec_id and discontinue the traversal
           	// of the linked list of record blocks (break from the loop) 
			//* slot map stores SLOT_UNOCCUPIED if slot is free and SLOT_OCCUPIED if slot is occupied
			if (slotMap[i] == SLOT_UNOCCUPIED) {
				rec_id.block = blockNum;
				rec_id.slot = i;
				break;
			}
		}

		if(rec_id.block != -1 && rec_id.slot != -1){
			break;
		}

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked list of record blocks)
        */
	   prevBlockNum = blockNum;
	   blockNum = blockHeader.rblock;
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
	if (rec_id.block == -1 && rec_id.slot == -1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
		if (relId == RELCAT_RELID){
			return E_MAXRELATIONS;
		}

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
		RecBuffer recBuffer;

        // get the block number of the newly allocated block
        // (use recBuffer::getBlockNum() function)
        blockNum = recBuffer.getBlockNum();
		
		// let ret be the return value of getBlockNum() function call
        if (blockNum == E_DISKFULL) return E_DISKFULL;

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
		rec_id.block = blockNum, rec_id.slot = 0;

		

		//  set the header of the new record block such that it links with
		//  existing record blocks of the relation
		//  set the block's header as follows:
		// blockType: REC, pblock: -1
		// lblock = -1 (if linked list of existing record blocks was empty
		// 				i.e this is the first insertion into the relation)
		// 		= prevBlockNum (otherwise),
		// rblock: -1, numEntries: 0,
		// numSlots: numOfSlots, numAttrs: numOfAttributes
		// (use recBuffer::setHeader() function)
        
		HeadInfo blockHeader;
		blockHeader.blockType = REC;
		blockHeader.pblock = -1;
		blockHeader.lblock = prevBlockNum;
		blockHeader.rblock = -1;
		blockHeader.numAttrs = numOfAttributes;
		blockHeader.numSlots = numOfSlots;
		blockHeader.numEntries = 0;

		recBuffer.setHeader(&blockHeader);
        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
	   	unsigned char slotMap[numOfSlots];
		for (int i = 0; i < numOfSlots; i++){
			slotMap[i] = SLOT_UNOCCUPIED;
		}

		recBuffer.setSlotMap(slotMap);

        // if prevBlockNum != -1
		if (prevBlockNum != -1)
        {
            //  create a RecBuffer object for prevBlockNum
			RecBuffer prevBlockBuffer (prevBlockNum);

            //  get the header of the block prevBlockNum and
			HeadInfo prevBlockHeader;
			prevBlockBuffer.getHeader(&prevBlockHeader);

            //  update the rblock field of the header to the new block
			prevBlockHeader.rblock = blockNum;
            // number i.e. rec_id.block
            // (use recBuffer::setHeader() function)
			prevBlockBuffer.setHeader(&prevBlockHeader);
        }
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
			relCatEntry.firstBlk = blockNum;
			RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
		relCatEntry.lastBlk = blockNum;
		RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // create a RecBuffer object for rec_id.block
    RecBuffer recBuffer(rec_id.block);

	// insert the record into rec_id'th slot using RecBuffer.setRecord())
	recBuffer.setRecord(record, rec_id.slot);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
	unsigned char slotmap [numOfSlots];
	recBuffer.getSlotMap(slotmap);

	slotmap[rec_id.slot] = SLOT_OCCUPIED;
	recBuffer.setSlotMap(slotmap);

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use recBuffer::getHeader() and recBuffer::setHeader() functions)
	HeadInfo blockHeader;
	recBuffer.getHeader(&blockHeader);

	blockHeader.numEntries++;
	recBuffer.setHeader(&blockHeader);

    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
	relCatEntry.numRecs++;
	RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}

/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId;

    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */
	recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);

    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
	if(recId.block == -1 && recId.slot == -1){
		return E_NOTFOUND;
	}

    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */
   	RecBuffer recBuffer(recId.block);
   	recBuffer.getRecord(record, recId.slot);

	return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
	if(strcmp(relName, "RELATIONCAT") == 0 || strcmp(relName, "ATTRIBUTECAT") == 0){
		cout << "Cannot delete relation " << relName << "\n";
		return E_NOTPERMITTED;
	}

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
	RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
	strcpy(relNameAttr.sVal, relName);

    //  linearSearch on the relation catalog for RelName = relNameAttr
	RecId recId;
	recId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    // if the relation does not exist (linearSearch returned {-1, -1})
    //     return E_RELNOTEXIST
	if(recId.block == -1 && recId.slot == -1){
		cout << "Relation with name " << relName << " does not exist\n";
		return E_RELNOTEXIST;
	}

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
	RecBuffer recBuffer(recId.block);
	recBuffer.getRecord(relCatEntryRecord, recId.slot);

    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
	int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
	int numAttr = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    /*
     Delete all the record blocks of the relation
    */
    // for each record block of the relation:
    //     get block header using BlockBuffer.getHeader
    //     get the next block from the header (rblock)
    //     release the block using BlockBuffer.releaseBlock
    //
    //     Hint: to know if we reached the end, check if nextBlock = -1
	while(firstBlock != -1){
		RecBuffer recBuffer(firstBlock);
		HeadInfo blockHeader;
		recBuffer.getHeader(&blockHeader);
		firstBlock = blockHeader.rblock;
		recBuffer.releaseBlock();
	}


    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
	RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while(true) {
        RecId attrCatRecId;
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr
		attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;
		if(attrCatRecId.block == -1 && attrCatRecId.slot == -1){
			break;
		}

        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
        // get the header of the block
        // get the record corresponding to attrCatRecId.slot

        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
		RecBuffer recBuffer(attrCatRecId.block);
		HeadInfo attrCatHeader;
		recBuffer.getHeader(&attrCatHeader);
		union Attribute record[attrCatHeader.numAttrs];
		recBuffer.getRecord(record, attrCatRecId.slot);
		int rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        // (This will be used later to delete any indexes if it exists)

        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
		unsigned char slotMap[attrCatHeader.numSlots];
		recBuffer.getSlotMap(slotMap);
		slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
		recBuffer.setSlotMap(slotMap);

        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */
		attrCatHeader.numEntries--;
		recBuffer.setHeader(&attrCatHeader);

        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        if (/*   header.numEntries == 0  */
				attrCatHeader.numEntries == 0) {
            /* Standard Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock
            */

            // create a RecBuffer for lblock and call appropriate methods
			RecBuffer recBuffer(attrCatHeader.lblock);
			HeadInfo leftBlockHeader;
			recBuffer.getHeader(&leftBlockHeader);
			leftBlockHeader.rblock = attrCatHeader.rblock;
			recBuffer.setHeader(&leftBlockHeader);

            if (/* header.rblock != -1 */
				attrCatHeader.rblock != -1) {
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                // create a RecBuffer for rblock and call appropriate methods
				RecBuffer recBuffer(attrCatHeader.rblock);
				HeadInfo rightBlockHeader;
				recBuffer.getHeader(&rightBlockHeader);
				rightBlockHeader.lblock = attrCatHeader.lblock;
				recBuffer.setHeader(&rightBlockHeader);

            } else {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
				RelCatEntry relCatBuffer;
				RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatBuffer);
				relCatBuffer.lastBlk = attrCatHeader.lblock;
            }

            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)

            // call releaseBlock()
			recBuffer.releaseBlock();
        }

        // (the following part is only relevant once indexing has been implemented)
        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        // if (rootBlock != -1) {
        //     // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
        // }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block
	recBuffer = RecBuffer(RELCAT_BLOCK);
	HeadInfo relCatHeader;
	recBuffer.getHeader(&relCatHeader);
    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */
	relCatHeader.numEntries--;
	recBuffer.setHeader(&relCatHeader);

    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */
	unsigned char slotMap[relCatHeader.numSlots];
	recBuffer.getSlotMap(slotMap);
	slotMap[recId.slot] = SLOT_UNOCCUPIED;
	recBuffer.setSlotMap(slotMap);

    /*** Updating the Relation Cache Table ***/
    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
	RelCatEntry relCatEntry;
	RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
	relCatEntry.numRecs--;
	RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted
    // Get the entry corresponding to attribute catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
	RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
	relCatEntry.numRecs -= numberOfAttributesDeleted;
	RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);

    return SUCCESS;
}