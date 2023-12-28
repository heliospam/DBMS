#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {

  for (int i = 0; i < BUFFER_CAPACITY; ++i) {
    // set metainfo[bufferindex] with the following values
    //   free = true
    //   dirty = false
    //   timestamp = -1
    //   blockNum = -1
    metainfo[i].free = true;
    metainfo[i].dirty = false;
    metainfo[i].timeStamp = -1;
    metainfo[i].blockNum = -1;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
// write back all modified blocks on system exit
StaticBuffer::~StaticBuffer() {
  	// iterate through all the buffer blocks, write back blocks 
	// with metainfo as free=false,dirty=true using Disk::writeBlock()

	for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
		if (metainfo[bufferIndex].free == false 
			&& metainfo[bufferIndex].dirty == true)
			Disk::writeBlock(blocks[bufferIndex], metainfo[bufferIndex].blockNum);
	}
}


int StaticBuffer::getFreeBuffer(int blockNum) {
	if (blockNum < 0 || blockNum >= DISK_BLOCKS) return E_OUTOFBOUND;
	for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
		metainfo[bufferIndex].timeStamp++;

	int allocatedBuffer = 0;

	// iterate through all the blocks in the StaticBuffer
	// find the first free block in the buffer (check metainfo)
	// assign allocatedBuffer = index of the free block
	for (; allocatedBuffer < BUFFER_CAPACITY; allocatedBuffer++)
		if (metainfo[allocatedBuffer].free) break;

	if (allocatedBuffer == BUFFER_CAPACITY) {
		int lastTimestamp = -1, bufferNum = -1;
		for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
			if (metainfo[bufferIndex].timeStamp > lastTimestamp) {
				lastTimestamp = metainfo[bufferIndex].timeStamp;
				bufferNum = bufferIndex;
			}
		}

		allocatedBuffer = bufferNum;
		if (metainfo[allocatedBuffer].dirty == true) {
			Disk::writeBlock(StaticBuffer::blocks[allocatedBuffer], 
								metainfo[allocatedBuffer].blockNum);
		}

		// return FAILURE;
	}

	metainfo[allocatedBuffer].free = false, 
	metainfo[allocatedBuffer].dirty = false,
	metainfo[allocatedBuffer].timeStamp = 0, 
	metainfo[allocatedBuffer].blockNum = blockNum;

	return allocatedBuffer;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
  if(blockNum < 0 || blockNum > DISK_BLOCKS){
    return E_OUTOFBOUND;
  }

  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  for(int i = 0;i <  32;i++){
    if(metainfo[i].blockNum == blockNum){
      return i;
    }
  }

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
    int bufferNum = getBufferNum(blockNum);

    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    if(bufferNum == E_BLOCKNOTINBUFFER){
      return E_BLOCKNOTINBUFFER;
    }

    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
    if(bufferNum == E_OUTOFBOUND){
      return E_OUTOFBOUND;
    }

    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo
    metainfo[bufferNum].dirty = true;

    // return SUCCESS
    return SUCCESS;
}