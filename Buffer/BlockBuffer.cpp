#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace std;

// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
/*
Used to get the header of the block into the location pointed to by `head`
NOTE: this function expects the caller to allocate memory for `head`
*/
int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }

  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries, buffer + 16, 4);
  memcpy(&head->numAttrs, buffer + 20, 4);
  memcpy(&head->rblock, buffer + 12, 4);
  memcpy(&head->lblock, buffer + 8, 4);
  memcpy(&head->pblock, buffer + 4, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;

  // get the header using this.getHeader() function
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  // read the block at this.blockNum into a buffer 
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = buffer + 32 + slotCount + (recordSize * slotNum);

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

//* load the record at slotNum into the argument pointer
int RecBuffer::setRecord(union Attribute *record, int slotNum)
{
	// read the block at this.blockNum into a buffer
	unsigned char *buffer;
	//// Disk::readBlock(buffer, this->blockNum);
	int ret = loadBlockAndGetBufferPtr(&buffer);
	if (ret != SUCCESS)
		return ret;
	
	// get the header using this.getHeader() function
	HeadInfo head;
	BlockBuffer::getHeader(&head);

	// get number of attributes in the block.
	int attrCount = head.numAttrs;

    // get the number of slots in the block.
	int slotCount = head.numSlots;

	//! if input slotNum is not in the permitted range 
	if (slotNum >= slotCount) return E_OUTOFBOUND;

	int recordSize = attrCount * ATTR_SIZE;
	unsigned char *slotPointer = buffer + (HEADER_SIZE + slotCount + (recordSize * slotNum)); // calculate buffer + offset

	// load the record into the rec data structure
	memcpy(slotPointer, record, recordSize);

	ret = StaticBuffer::setDirtyBit(this->blockNum);

	//! The above function call should not fail since the block is already
    //! in buffer and the blockNum is valid. If the call does fail, there
    //! exists some other issue in the code) 
	if (ret != SUCCESS) {
		std::cout << "There is some error in the code!\n";
		exit(1);
	}

	// // Disk::writeBlock(buffer, this->blockNum);

	return SUCCESS;
}

/* NOTE: This function will NOT check if the block has been initialised as a
   record or an index block. It will copy whatever content is there in that
   disk block to the buffer.
   Also ensure that all the methods accessing and updating the block's data
   should call the loadBlockAndGetBufferPtr() function before the access or
   update is done. This is because the block might not be present in the
   buffer due to LRU buffer replacement. So, it will need to be bought back
   to the buffer before any operations can be done.
 */
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
	// check whether the block is already present in the buffer
	// using StaticBuffer.getBufferNum()
	int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
	if (bufferNum == E_OUTOFBOUND)
		return E_OUTOFBOUND;

	// if present (!=E_BLOCKNOTINBUFFER),
	// 		set the timestamp of the corresponding buffer to 0 and increment the
	// 		timestamps of all other occupied buffers in BufferMetaInfo.
	if (bufferNum != E_BLOCKNOTINBUFFER) {
		for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
			StaticBuffer::metainfo[bufferIndex].timeStamp++;
		}
		StaticBuffer::metainfo[bufferNum].timeStamp = 0;
	}
	else if (bufferNum == E_BLOCKNOTINBUFFER) // the block is not present in the buffer
	{ 
		bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

		//! no free space found in the buffer (currently)
		//! or some other error occurred in the process
		if (bufferNum == E_OUTOFBOUND || bufferNum == FAILURE)
			return bufferNum;

		Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
	}

	// store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
	*buffPtr = StaticBuffer::blocks[bufferNum];
	return SUCCESS;
}

/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*  `
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

	// get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
	int ret = loadBlockAndGetBufferPtr(&bufferPtr);
	if (ret != SUCCESS)
		return ret;

	// get the header of the block using getHeader() function
	RecBuffer recordBlock (this->blockNum);
	struct HeadInfo head;
	recordBlock.getHeader(&head);

	int slotCount = head.numSlots;

	// get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
	unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

	// copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
	for (int slot = 0; slot < slotCount; slot++) {
		*(slotMap+slot)= *(slotMapInBuffer+slot);
  }

	return SUCCESS; 
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

    double diff;
    // if attrType == STRING
    //     diff = strcmp(attr1.sval, attr2.sval)
    if(attrType == STRING){
      diff = strcmp(attr1.sVal, attr2.sVal);
    }

    // else
    //     diff = attr1.nval - attr2.nval
    else{
      diff = attr1.nVal - attr2.nVal;
    }
    /*
    if diff > 0 then return 1
    if diff < 0 then return -1
    if diff = 0 then return 0
    */
   if(diff == 0) return 0;
   else if(diff < 0) return -1;
   return 1;
}

