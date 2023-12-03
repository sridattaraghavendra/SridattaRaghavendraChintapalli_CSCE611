/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");
    fileSystem = _fs;

    inode = _fs->LookupFile(_id);

    if(inode == NULL){
        Console::puts("File not found\n");
        assert(false);
    }

    fileSystem->disk->read(inode->start_block,block_cache);

    currentPosition = 0;
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */

    /*Check if the block is changed*/
    if(currentPosition > 0){
        fileSystem->disk->write(inode->start_block, block_cache);
    }
    
    /*Clear the cache so that it will not result in unintented issues*/
    memset(block_cache,0,SimpleDisk::BLOCK_SIZE);
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    
    /*Bytes to read*/
    unsigned int bytesToRead = (inode->block_size * SimpleDisk::BLOCK_SIZE) - currentPosition;

    if(currentPosition >= inode->block_size * SimpleDisk::BLOCK_SIZE){
        Console::puts("End of file reached.\n");
        return 0;
    }

    /*Bytes to read will be limited to the size of one block*/
    bytesToRead = (_n < bytesToRead) ? _n : bytesToRead;


    memcpy(_buf, block_cache + currentPosition, bytesToRead);

    currentPosition += bytesToRead;

    return bytesToRead;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    
    /*Bytes to write will be limited to single block's size*/
    unsigned int remainingBytes = SimpleDisk::BLOCK_SIZE - currentPosition;

    unsigned int bytesToBeWritten = (_n < remainingBytes) ? _n : remainingBytes;

    memcpy(block_cache + currentPosition, _buf, bytesToBeWritten);

    currentPosition += bytesToBeWritten;

    /*This is to ensure that we write the data to the disk, when we encounter 
        the current position greater than or equal to block size else
        data is captured in the cache and written to the disk when the
        file is closed*/
    if(currentPosition >= SimpleDisk::BLOCK_SIZE){
        fileSystem->disk->write(inode->start_block,block_cache);
        currentPosition = 0;
    }

    return bytesToBeWritten;
}

void File::Reset() {
    Console::puts("resetting file\n");
    currentPosition = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    return currentPosition >= (inode->block_size * SimpleDisk::BLOCK_SIZE);
}
