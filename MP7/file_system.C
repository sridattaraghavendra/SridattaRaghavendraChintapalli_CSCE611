/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    assert(false);
}

/*--------------------------------------------------------------------------*/
/* UTIL FUNCTIONS */
Inode * FileSystem::GetFreeInode(){
    for(int index = 0; index < MAX_INODES; index++){
        if(inodes[index].id == -1){
            Console::puts("Found a free INODE.\n");
            return &inodes[index];
        }
    }
    return NULL;
}

int FileSystem::GetFreeBlock(){
    memset(buf,0,SimpleDisk::BLOCK_SIZE);
    disk->read(1,buf);

    free_blocks = buf;

    for(int index = 0; index < MAX_BLOCKS; index++){
        if(free_blocks[index] == FREE_BLOCK){
            Console::puts("Found a free block : ");
            Console::puti(index);
            Console::puts("\n");
            return index;
        }
    }
    return -1;
}

/*--------------------------------------------------------------------------*/
/* INODE FUNCTIONS */
void Inode::WriteInodeListToDisk(SimpleDisk * _disk, unsigned char * _buf){
    _disk->write(0,_buf);
}

Inode * Inode::ReadInodeListFromDisk(SimpleDisk * _disk, unsigned char * _buf){
    _disk->read(0,_buf);
    return (Inode *) _buf;
}

void FileSystem::WriteFreeListToDisk(SimpleDisk * _disk, unsigned char * _buf){
    _disk->write(1,buf);
}

void FileSystem::ReadFreeListFromDisk(SimpleDisk * _disk, unsigned char * _buf){
    memset(_buf,0, SimpleDisk::BLOCK_SIZE);
    _disk->read(1,_buf);
}

/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");

    /* Here you read the inode list and the free list into memory */

    /* Read INODE List */
    inodes = Inode::ReadInodeListFromDisk(_disk,buf);
    
    /* Read FREE List */
    _disk->read(1, buf);
    free_blocks = buf;

    disk = _disk;
    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */

    /*Clean the disk buffer*/
    memset(buf,0,SimpleDisk::BLOCK_SIZE);

    for(int index=0; index < MAX_BLOCKS; index++){
        _disk->write(index,buf);
    }

    /*Initialize Inode Information*/
    _disk->read(0, buf);
    Inode *inodeList = (Inode *) buf;
    for(int index=0; index < MAX_INODES; index++){
        inodeList[index].id = -1;
        inodeList[index].start_block = 0;
        inodeList[index].current_position = 0;
    }
    Inode::WriteInodeListToDisk(_disk,buf);

    /*Initialize Free block information*/
    memset(buf,0,SimpleDisk::BLOCK_SIZE);
    _disk->read(1, buf);
    for(int index=0; index < MAX_BLOCKS; index++){
        buf[index] = FREE_BLOCK;
    }

    buf[0] = BLOCK_USED;
    buf[1] = BLOCK_USED;

    _disk->write(1, buf);

    Console::puts("formatting disk successfull\n");

    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /*Read INODES From Disk*/
    memset(buf,0,SimpleDisk::BLOCK_SIZE);

    inodes = Inode::ReadInodeListFromDisk(disk,buf);

    /* Here you go through the inode list to find the file. */
    for(int index=0; index < MAX_INODES; index++){
        if(inodes[index].id == _file_id){
            Console::puts("Found file, returning it to user.\n");
            return &inodes[index];
        }
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    memset(buf,0,SimpleDisk::BLOCK_SIZE);

    inodes = Inode::ReadInodeListFromDisk(disk,buf);

    /*Check if file is already present*/
    for(int index=0; index < MAX_INODES; index++){
        if(inodes[index].id == _file_id){
            Console::puts("File already exists, aborting file creation.\n");
            assert(false);
        }
    }

    Inode * freeInode = GetFreeInode();
    int freeBlock = GetFreeBlock();

    if(freeInode == NULL || freeBlock == -1){
        Console::puts("Available space exceeded, aborting file creation.\n");
        assert(false);
    }

    freeInode->id = _file_id;
    freeInode->start_block = freeBlock;
    freeInode->block_size = 1;

    Inode::WriteInodeListToDisk(disk,buf);
    
    /*Mark the block as used and update free list to disk*/
    free_blocks[freeBlock] = BLOCK_USED;
    FileSystem::WriteFreeListToDisk(disk,free_blocks);

    return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */
    bool isFileFound = false;
    int start_block_number;
    int number_of_blocks;

    memset(buf,0,SimpleDisk::BLOCK_SIZE);

    inodes = Inode::ReadInodeListFromDisk(disk,buf);

    /*Check if file is already present*/
    for(int index=0; index < MAX_INODES; index++){
        if(inodes[index].id == _file_id){
           start_block_number = inodes[index].start_block;
           number_of_blocks = inodes[index].block_size;
           inodes[index].id = -1;
           inodes[index].start_block = 0;
           inodes[index].block_size = 0;
           inodes[index].current_position = 0;
           isFileFound = true;
           break;
        }
    }

    if(!isFileFound){
        Console::puts("File doesn't exist, aborting deletion.\n");
        assert(false);
    }

    disk->write(0,buf);

    /*Free blocks*/
    ReadFreeListFromDisk(disk,buf);

    for(int index = start_block_number; index <= start_block_number + number_of_blocks - 1; index++){
        buf[index] = FREE_BLOCK;
    }

    WriteFreeListToDisk(disk, buf);
    return true;
}
