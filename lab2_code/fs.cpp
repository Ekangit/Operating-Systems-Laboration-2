#include <iostream>
#include "fs.h"
#include <vector>

using namespace std;


FS::FS()
{
    cout << "FS::FS()... Creating file system\n";
}

FS::~FS()
{

}

// formats the disk, i.e., creates an empty file system
int
FS::format()
{
    cout << "FS::format()\n";
    this->fat[ROOT_BLOCK] = FAT_EOF;
    this->fat[FAT_BLOCK] = FAT_EOF;
    for(int i = 2; i < this->disk.get_no_blocks(); i++){
        this->fat[i] = FAT_FREE;
    }

    this->cwd = "/";
    this->cwb = ROOT_BLOCK;
    dir_entry root_array[64] = {0};
    this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));
    this->disk.write(ROOT_BLOCK,reinterpret_cast<uint8_t*>(root_array));

    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(string filepath)
{
    cout << "FS::create(" << filepath << ")\n";
    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));
    


    // Kolla om fil redan existerar i directory
    if(resultr == 0 && resultf == 0){
        for(int i = 0; i < 64; i++){
            if(directory_array[i].file_name[0] != '\0'){
                if(directory_array[i].file_name == filepath){
                    cout << "Create(" << filepath<< ") - ERROR: File already exists \n";
                    return -1;
                }  
            }
        }

        
        dir_entry newFile;
        string line;
        string totalfile;

        // Storea filen i buffrar och spara i array
        while (getline(cin, line) && !line.empty()) {
            totalfile += line + "\n";
        }

        // Hur många block behövs
        int blocksNeeded = (totalfile.size() + 4095) / 4096; 
        if (blocksNeeded == 0) {
            blocksNeeded = 1;
        }

        // Hur mucket plats finns i fat
        int freeCount = 0;
        bool first = true;
        vector<int> freeBlocks;
        for(int i = 2; i < 2048; i++) {
            if(fat[i] == FAT_FREE){
                freeCount++;
                freeBlocks.push_back(i);
                if(first){
                    newFile.first_blk = i;
                    first = false;
                }

            } 
            
        }

        // Kolla att vi har plats i fat
        if(freeCount < blocksNeeded){
            cout << "Create(" << filepath<< ") - ERROR: Not enough disk space \n";
            return -1;
        }


        if(filepath.size() > 55){
            cout << "Create(" << filepath<< ") - ERROR: Too long filename \n";
            return -1;
        }
        // Uppdatera struct
        newFile.access_rights = 6;
        for(int i = 0; i <filepath.size(); i++){
            newFile.file_name[i] = filepath[i];
        }
        if(filepath.size() != 55){
            newFile.file_name[filepath.size()] = '\0';
        }
       

        newFile.type = TYPE_FILE;
        newFile.size = totalfile.size();

        // Hitta ledig plats i directory och sätt in ny fil
        int z = 0;
        while(z < 64 && directory_array[z].file_name[0] !='\0'){
            z++;
        }
        if(z < 64){
            directory_array[z] = newFile;
        }else{
            cout << "Create(" << filepath<< ") - ERROR: Directory is full \n";
            return -1;
        }

        
        int Block = freeBlocks[0];
        for(int i = 0; i < blocksNeeded; i++){
            Block = freeBlocks[i];
            if(i == blocksNeeded - 1){
                fat[Block] = FAT_EOF;
            }else{
                fat[Block] = freeBlocks[i+1];
            }
        }


    
        // Skriv filbuffrar till disk
        int size = totalfile.size();
        for(int j = 0; j < blocksNeeded; j++){
            uint8_t dataBlock[4096] = {0};
            for(int i = 0; i < BLOCK_SIZE; i++){
                if(size > i+(j*BLOCK_SIZE)){
                    dataBlock[i] = totalfile[i+(j*BLOCK_SIZE)];
                }
            }
            this->disk.write(freeBlocks[j],dataBlock);
        }
        
        
        // Skriv current directory och fat till disk
        this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
        this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(fat));

    }else{
        return -1;
    }
    return 0;
}



// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(string filepath)
{
    cout << "FS::cat(" << filepath << ")\n";
    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));

    int block = -1;
    int fileSize;
    // find first block
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name == filepath){
            block = directory_array[i].first_blk;
            fileSize = directory_array[i].size;
        }
    }

    if(block == -1){
        cout << "Cat(" << filepath<< ") - ERROR: File not in CD \n";
        return -1;
    }

  
    uint8_t buffer[4096];
    int blocksize = fileSize;
    vector<int> blockSizes;

    while(blocksize > BLOCK_SIZE){
        blockSizes.push_back(BLOCK_SIZE);
        blocksize -= BLOCK_SIZE;
    }
    blockSizes.push_back(blocksize);

    int x = 0;
    while(block != FAT_EOF){
        disk.read(block,buffer);
        block = fat[block];
        for(int i = 0; i < blockSizes[x]; i++){
            cout << static_cast<char>(buffer[i]);
        }
        x++;
    }
    

    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    cout << "FS::ls()\n";
    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));

    cout << "name\t size" << endl;

    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            cout << directory_array[i].file_name << "\t " << directory_array[i].size << endl;
        }
    }


    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(string sourcepath, string destpath)
{
    cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));

    if(sourcepath == destpath){
        cout << "FS::cp(" << sourcepath << "," << destpath << ") - Error same filename for source and copy\n";
        return -1;
    }

    //Gå igenom cd
    bool found = false;
    int firstBlock;
    int fileSize;
    dir_entry newFile;
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            if(directory_array[i].file_name == sourcepath){
                found = true;

                for(int i = 0; i < destpath.size(); i++){
                    newFile.file_name[i] = destpath[i];
                }
                newFile.access_rights = directory_array[i].access_rights;
                newFile.size = directory_array[i].size;
                newFile.type = directory_array[i].type;

                firstBlock = directory_array[i].first_blk;
                fileSize = directory_array[i].size;
            } 
            if(directory_array[i].file_name == destpath){
                cout << "FS::cp(" << sourcepath << "," << destpath << ") - Error Copy filename already exists\n";
                return -1;
            }
        }
    }

    //Kolla så att filen existerar
    if(!found){
        cout << "cp(" << sourcepath << ") - ERROR: File not in CD \n";
        return -1;
    }

    // Kolla hur många blocks behövs
    int blocksNeeded = (fileSize + 4095) / 4096; 
    if (blocksNeeded == 0) {
        blocksNeeded = 1;
    }

    // Hur mucket plats finns i fat
    int freeCount = 0;
    bool first = true;
    vector<int> freeBlocks;
    for(int i = 2; i < 2048; i++) {
        if(fat[i] == FAT_FREE){
            freeCount++;
            freeBlocks.push_back(i);
            if(first){
                newFile.first_blk = i;
                first = false;
            }
        } 
    }

    // Kolla att vi har plats i fat
    if(freeCount < blocksNeeded){
        cout << "cp(" << destpath << ") - ERROR: Not enough disk space \n";
        return -1;
    }

    // Hitta ledig plats i directory och sätt in ny fil
    int z = 0;
    while(z < 64 && directory_array[z].file_name[0] !='\0'){
        z++;
    }
    if(z < 64){
        directory_array[z] = newFile;
    }else{
        cout << "cp(" << destpath<< ") - ERROR: Directory is full \n";
        return -1;
    }

    // Uppdatera FAT
    int Block;
    for(int i = 0; i < blocksNeeded; i++){
        Block = freeBlocks[i];
        if(i == blocksNeeded - 1){
            fat[Block] = FAT_EOF;
        }else{
            fat[Block] = freeBlocks[i+1];
        }
    }

    
    // Kopiera till disk
    int block = firstBlock;
    uint8_t buffer[4096];
    int x = 0;
    while(block != FAT_EOF){
        disk.read(block,buffer);
        this->disk.write(freeBlocks[x],buffer);
        block = fat[block];
        x++;
    }

    this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
    this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(fat));

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(string sourcepath, string destpath)
{
    cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(string filepath)
{
    cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(string filepath1, string filepath2)
{
    cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(string dirpath)
{
    cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(string dirpath)
{
    cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    cout << "FS::pwd()\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(string accessrights, string filepath)
{
    cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}
