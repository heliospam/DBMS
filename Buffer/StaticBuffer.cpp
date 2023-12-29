#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

// declare the blockAllocMap array
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
  // copy blockAllocMap blocks from disk to buffer (using readblock() of disk)
  // blocks 0 to 3
  for(int i = 0;i < 4;i++){
    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer, i);
    for(int j = 0;j < BLOCK_SIZE;j++){
      blockAllocMap[BLOCK_SIZE*i + j] = buffer[i];
    }
  }

  for (int i = 0; i < BUFFER_CAPACITY; ++i) {
    // set metainfo[i] with the following values
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
  for(int i = 0;i < 4;i++){
    unsigned char buffer[BLOCK_SIZE];
    for(int j = 0;j < BLOCK_SIZE;j++){
      buffer[i] = blockAllocMap[BLOCK_SIZE*i + j];
    }
    Disk::writeBlock(buffer, i);
  }
  	// iterate through all the buffer blocks, write back blocks 
	// with metainfo as free=false,dirty=true using Disk::writeBlock()

	for (int i = 0; i < BUFFER_CAPACITY; i++) {
		if (metainfo[i].free == false 
			&& metainfo[i].dirty == true)
			Disk::writeBlock(blocks[i], metainfo[i].blockNum);
	}
}


int StaticBuffer::getFreeBuffer(int blockNum) {
	if (blockNum < 0 || blockNum >= DISK_BLOCKS) return E_OUTOFBOUND;
	for (int i = 0; i < BUFFER_CAPACITY; i++){
    metainfo[i].timeStamp += 1;
  }

	int bufferNum = 0;

	// iterate through all the blocks in the StaticBuffer
	// find the first free block in the buffer (check metainfo)
	// assign bufferNum = index of the free block
	for (; bufferNum < BUFFER_CAPACITY; bufferNum++){
    if (metainfo[bufferNum].free){
      break;
    }
  }

	if (bufferNum == BUFFER_CAPACITY) {
		int maxi = -1, buffer = -1;
		for (int i = 0; i < BUFFER_CAPACITY; i++) {
			if (metainfo[i].timeStamp > maxi) {
				maxi = metainfo[i].timeStamp;
				buffer = i;
			}
		}

		bufferNum = buffer;
		if (metainfo[bufferNum].dirty == true) {
			Disk::writeBlock(StaticBuffer::blocks[bufferNum], metainfo[bufferNum].blockNum);
		}

		// return FAILURE;
	}

	metainfo[bufferNum].free = false, 
	metainfo[bufferNum].dirty = false,
	metainfo[bufferNum].timeStamp = 0, 
	metainfo[bufferNum].blockNum = blockNum;

	return bufferNum;
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

  // find and return the i which corresponds to blockNum (check metainfo)
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