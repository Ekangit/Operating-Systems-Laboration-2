#include <iostream>
#include "fs.h"
#include <vector>
#include <cstring>

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

        // Uppdatera fat
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
    // Hitta filen och ändra variabler
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name == filepath){
            if(directory_array[i].type == TYPE_FILE){
                block = directory_array[i].first_blk;
                fileSize = directory_array[i].size;
            }else{
                cout << "Cat(" << filepath<< ") - ERROR: File is a directory  \n";
                return -1;
            }
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

    cout << "name\t type \tsize"  << endl;

    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            string type = "";
            if(directory_array[i].type == TYPE_FILE){
                type = "file";
            }else{
                type = "dir";
            }
            cout << directory_array[i].file_name << "\t " << type << "\t" << directory_array[i].size << endl;
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

    if(destpath.size() > 55){
        cout << "Create(" << destpath<< ") - ERROR: Too long filename \n";
        return -1;
    }

    if(sourcepath == destpath){
        cout << "FS::cp(" << sourcepath << "," << destpath << ") - Error same filename for source and copy\n";
        return -1;
    }

    
    int type = TYPE_FILE;
    int destBlock = -1;
    
    //Gå igenom cd
    bool found = false;
    int firstBlock;
    int fileSize;
    dir_entry newFile = {0};
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            if(directory_array[i].file_name == sourcepath){
                found = true;
                if(directory_array[i].type == TYPE_DIR){
                    cout << "FS::cp(" << sourcepath << "," << destpath << ") - Error Sourcepath is a directory\n";
                    return -1;
                }
                
                newFile.access_rights = directory_array[i].access_rights;
                newFile.size = directory_array[i].size;
                newFile.type = directory_array[i].type;

                firstBlock = directory_array[i].first_blk;
                fileSize = directory_array[i].size;
            } 
            if(directory_array[i].file_name == destpath){
                if(directory_array[i].type == TYPE_FILE){
                    cout << "FS::cp(" << sourcepath << "," << destpath << ") - Error Copy filename already exists\n";
                    return -1;
                }else{
                    type = TYPE_DIR;
                    destBlock = directory_array[i].first_blk;
                }
                
            }
        }
    }

    //Kolla så att filen existerar
    if(!found){
        cout << "cp(" << sourcepath << ") - ERROR: File not in CD \n";
        return -1;
    }

    if(type == TYPE_FILE){
        for(int j = 0; j < destpath.size(); j++){
            newFile.file_name[j] = destpath[j];
        }
    }else{
        for(int j = 0; j < sourcepath.size(); j++){
            newFile.file_name[j] = sourcepath[j];
        }
        this->disk.read(destBlock,reinterpret_cast<uint8_t*>(directory_array));
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

    if(type == TYPE_FILE){
        this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
    }else{
        this->disk.write(destBlock,reinterpret_cast<uint8_t*>(directory_array));
    }   
    
    this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(fat));

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(string sourcepath, string destpath)
{
    cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));

    if(destpath.size() > 55){
        cout << "Create(" << destpath<< ") - ERROR: Too long filename \n";
        return -1;
    }

    if(resultr == -1){
        return -1;
    }

    if(sourcepath == destpath){
        return 0;
    }

    int type = TYPE_FILE;
    int destBlock;
    bool found = false;
    int sourceIndex;
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            if(directory_array[i].file_name == sourcepath){
                found = true;
                sourceIndex = i;
            } 
            if(directory_array[i].file_name == destpath){
                if(directory_array[i].type == TYPE_FILE){
                    cout << "FS::mv(" << sourcepath << "," << destpath << ") - Error Copy filename already exists\n";
                    return -1;
                }else{
                    type = TYPE_DIR;
                    destBlock = directory_array[i].first_blk;
                }
                
            }
        }
    }

    //Kolla så att filen existerar
    if(!found){
        cout << "mv(" << sourcepath << ") - ERROR: File not in CD \n";
        return -1;
    }

    if(type == TYPE_FILE){
        // Nollställ namnet
        for(int k = 0; k < 56; k++) {
            directory_array[sourceIndex].file_name[k] = '\0';
        }

        // Ändra namnet
        for(int j = 0; j < destpath.size(); j++){
            directory_array[sourceIndex].file_name[j] = destpath[j];
        }
        

        this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
    }else{
        dir_entry copy = {0};
        dir_entry dest_array[64] = {0};
        copy = directory_array[sourceIndex];
        // Nollställer dir_entry-structen
        memset(&directory_array[sourceIndex], 0, sizeof(dir_entry));
        this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
        this->disk.read(destBlock,reinterpret_cast<uint8_t*>(dest_array));

        int z = 0;
        while(z < 64 && dest_array[z].file_name[0] !='\0'){
            z++;
        }
        if(z < 64){
            dest_array[z] = copy;
        }else{
            cout << "mv(" << destpath<< ") - ERROR: Directory is full \n";
            return -1;
        }
        this->disk.write(destBlock,reinterpret_cast<uint8_t*>(dest_array));
    }

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(string filepath)
{
    cout << "FS::rm(" << filepath << ")\n";
    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));

    if(resultf == -1 || resultr == -1){
        return -1;
    }

    bool found = false;
    int type = TYPE_FILE;
    int Block;
    int index;
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            if(directory_array[i].file_name == filepath){
                if(directory_array[i].type == TYPE_FILE){
                    found = true;
                    Block = directory_array[i].first_blk;
                    // Nollställer dir_entry-structen
                    memset(&directory_array[i], 0, sizeof(dir_entry));
                }else{
                    index = i;
                    found = true;
                    type = TYPE_DIR;
                    Block = directory_array[i].first_blk;
                }
            } 
        }
    }

    //Kolla så att filen existerar
    if(!found){
        cout << "rm(" << filepath << ") - ERROR: File not in CD \n";
        return -1;
    }

    if(type == TYPE_FILE){
        int back;
        while(Block != FAT_EOF){
            back = Block;
            Block = fat[Block];
            fat[back] = FAT_FREE;
        }

        this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
        this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(fat));
    }else{
        dir_entry dest_array[64] = {0};
        this->disk.read(Block,reinterpret_cast<uint8_t*>(dest_array));

        int counter = 0;
        bool foundsomething = false;
        while(counter < 64 && !foundsomething){
            if(dest_array[counter].file_name[0] != '\0'){
                if(dest_array[counter].file_name != ".."){
                    foundsomething = true;
                }
            }
            counter++;
        }
        if(foundsomething){
            cout << "rm(" << filepath << ") - ERROR: Directory is not empty \n";
            return -1;
        }
        
        fat[Block] = FAT_FREE;
        memset(&directory_array[index], 0, sizeof(dir_entry));

        this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
        this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(fat));

    }

    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(string filepath1, string filepath2)
{
    cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));

    if(resultf == -1 || resultr == -1){
        return -1;
    }

    // Gå igenom cd och initiera variabler
    bool foundfirst = false;
    bool foundsecond = false;
    int sourceFirstBlock;
    int destFirstBlock;
    int sourceSize;
    int destSize;
    int destindex;
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            if(directory_array[i].file_name == filepath1){
                foundfirst = true;
                sourceFirstBlock = directory_array[i].first_blk;
                sourceSize = directory_array[i].size;
            } 
            if(directory_array[i].file_name == filepath2){
                foundsecond = true;
                destFirstBlock = directory_array[i].first_blk;
                destSize = directory_array[i].size;
                destindex = i;
            } 
        }
    }

    //Kolla så att filerna existerar
    if(!foundfirst || !foundsecond){
        cout << "mv(" << filepath1 << filepath2 << ") - ERROR: File not in CD \n";
        return -1;
    }

    int restSize = destSize % BLOCK_SIZE;
    int totalSize = sourceSize + destSize;
    directory_array[destindex].size = totalSize;

    // Kolla hur många blocks behövs totalt
    int totalblocksNeeded = (totalSize + 4095) / 4096; 
    if (totalblocksNeeded == 0) {
        totalblocksNeeded = 1;
    }

    // Kolla hur många blocks dest använder
    int destblocks = (destSize + 4095) / 4096; 
    if (destblocks == 0) {
        destblocks = 1;
    }

    int extrablocks = totalblocksNeeded - destblocks;

    // Hur mucket plats finns i fat
    int freeCount = 0;
    bool first = true;
    vector<int> freeBlocks;
    for(int i = 2; i < 2048; i++) {
        if(fat[i] == FAT_FREE){
            freeCount++;
            freeBlocks.push_back(i);
            if(first){
                first = false;
            }
        } 
    }

    // Kolla att vi har plats i fat
    if(freeCount < extrablocks){
        cout << "Append(" << filepath1 << filepath2 << ") - ERROR: Not enough disk space \n";
        return -1;
    }

    
    // Gå till sista  block i destfil
    int block = destFirstBlock;
    int back;
    while(block != FAT_EOF){
        back = block;
        block = fat[block];
    }
    block = back;


    uint8_t destbuffer[4096];
    uint8_t sourcebuffer[4096];
    string totalSourceFile = "";
    int blocksize = sourceSize;
    vector<int> blockSizes;

    //Spara block sizes för sourcefil
    while(blocksize > BLOCK_SIZE){
        blockSizes.push_back(BLOCK_SIZE);
        blocksize -= BLOCK_SIZE;
    }
    blockSizes.push_back(blocksize);

    // Kopiera till fulla buffer för sourcefil
    int x = 0;
    int cBlock = sourceFirstBlock;
    int z = 0;
    while(cBlock != FAT_EOF){
        disk.read(cBlock,sourcebuffer);
        cBlock = fat[cBlock];
        for(int i = 0; i < blockSizes[x]; i++){
            totalSourceFile += sourcebuffer[i];
            z++;
        }
        x++;
    }


    this->disk.read(block,destbuffer);

    //Gör färdigt sista block för destfil
    int j = 0;
    for(int i = restSize; i < BLOCK_SIZE; i++){
        if(j < sourceSize){
            destbuffer[i] = totalSourceFile[j];
        }
        j++;
    }
    this->disk.write(block,destbuffer);

    
    
    for(int x = 0; x < extrablocks; x++){
        memset(destbuffer, 0, BLOCK_SIZE);
        block = freeBlocks[x];
        for(int i = 0; i < BLOCK_SIZE; i++){
            if(j < sourceSize){
                destbuffer[i] = totalSourceFile[j];
                j++;
            }
        }
        this->disk.write(block,destbuffer);
        
        fat[back] = block;
        fat[block] = FAT_EOF;
        back = block;
       
    }
    
    this->disk.write(cwb,reinterpret_cast<uint8_t*>(directory_array));
    this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(fat));

    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(string dirpath)
{
    cout << "FS::mkdir(" << dirpath << ")\n";

    dir_entry directory_array[64] = {0};
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));


    if(dirpath.size() > 55){
        cout << "FS::mkdir(" << dirpath << ") - ERROR: Too long directory name\n";
        return -1;
    }

    // 0 - file, 1 - absolute,  2 - relativ
    int inputtype = 0;
    int directoryBlock = cwb;
    
    if(dirpath[0] == '/'){
        inputtype = 1;
    }else{
        int x = 0;
        bool exists = false;
        while(x < dirpath.size() && !exists){
            if(dirpath[x] == '/'){
                exists = true;
            }
            x++;
        }

        if(exists){
            inputtype = 2;
        }
    }
    
    if(inputtype == 1){
        vector<string> paths;
        this->disk.read(ROOT_BLOCK,reinterpret_cast<uint8_t*>(directory_array));
        bool firsttimea = true;
        string foldername = "";
        for(int i = 0; i < dirpath.size(); i++){
            if(dirpath[i] == '/'){
                if(firsttimea){
                    firsttimea = false;
                }else{
                    paths.push_back(foldername);
                    foldername = "";
                }
            }else{
                foldername += dirpath[i];
            }
        }
        dirpath = foldername;
        for(int i = 0; i < paths.size(); i++){
            bool founda = false;
            int k = 0;
            while(k < 64 && !founda){
                if(directory_array[k].file_name[0] != '\0'){
                    if(directory_array[k].file_name == paths[i]){
                        founda = true;
                        directoryBlock = directory_array[k].first_blk;
                        this->disk.read(directoryBlock,reinterpret_cast<uint8_t*>(directory_array));
                    }
                }
                k++;
            }
            if(!founda){
                cout << "FS::mkdir(" << dirpath << ") - ERROR: A folder in path does not exists\n";
                return -1;
            }
        }


    }else if (inputtype == 2){
        vector<string> paths;
        this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
        string foldername = "";
        for(int i = 0; i < dirpath.size(); i++){
            if(dirpath[i] == '/'){
                paths.push_back(foldername);
                foldername = "";
            }else{
                foldername += dirpath[i];
            }
        }
        dirpath = foldername;
        for(int i = 0; i < paths.size(); i++){
            bool foundr = false;
            int k = 0;
            while(k < 64 && !foundr){
                if(directory_array[k].file_name[0] != '\0'){
                    if(directory_array[k].file_name == paths[i]){
                        foundr = true;
                        directoryBlock = directory_array[k].first_blk;
                        this->disk.read(directoryBlock,reinterpret_cast<uint8_t*>(directory_array));
                    }
                }
                k++;
            }
            if(!foundr){
                cout << "FS::mkdir(" << dirpath << ") - ERROR: A folder in path does not exists\n";
                return -1;
            }
        }
    }else if(inputtype == 0){
        this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    }


    

    // Gå igenom current directory och kolla om filnamnet existerar och om det finns plats för nya directory
    int freeDirectoryIndex;
    bool first = true;
    int parentindex = -1;
    dir_entry newDir = {0};
    dir_entry newParentDir = {0};
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            if(directory_array[i].file_name == dirpath){
                cout << "FS::mkdir(" << dirpath << ") - ERROR: Directory name already exists\n";
                return -1;
            }else if(directory_array[i].file_name == ".."){
                parentindex = i;
            }
        }else{
            if(first){
                first = false;
                freeDirectoryIndex = i;
            }
        }
    }

    if(first){
        cout << "FS::mkdir(" << dirpath << ") - ERROR: Current directory is full\n";
        return -1;
    }

    int blocksNeeded = 1;

    // Hur mucket plats finns i fat
    int freeCount = 0;
    vector<int> freeBlocks;
    for(int i = 2; i < 2048; i++) {
        if(fat[i] == FAT_FREE){
            freeCount++;
            freeBlocks.push_back(i);
        } 
    }

    // Kolla att vi har plats i fat
    if(freeCount < blocksNeeded){
        cout << "FS::mkdir(" << dirpath << ") - ERROR: No blocks available\n";
        return -1;
    }

    int newDirectoryBlock = freeBlocks[0];

    // Uppdatera nya dir_entry
    newDir.access_rights = READ + WRITE;
    newDir.first_blk = newDirectoryBlock;
    newDir.type = TYPE_DIR;
    

    for(int i = 0; i < dirpath.size(); i++){
        newDir.file_name[i] = dirpath[i];
    }
    newDir.file_name[dirpath.size()] = '\0';

    directory_array[freeDirectoryIndex] = newDir;

    fat[newDirectoryBlock] = FAT_EOF;

    // Uppdatera nya .. directory
    
    newParentDir.file_name[0] = '.';
    newParentDir.file_name[1] = '.';
    newParentDir.file_name[2] = '\0';
    newParentDir.first_blk = directoryBlock;
    newParentDir.type = TYPE_DIR;
    newParentDir.access_rights = READ + WRITE;

    this->disk.write(directoryBlock,reinterpret_cast<uint8_t*>(directory_array));
    this->disk.write(FAT_BLOCK,reinterpret_cast<uint8_t*>(fat));

    dir_entry child_array[64] = {0};
    child_array[0] = newParentDir;
    this->disk.write(newDirectoryBlock,reinterpret_cast<uint8_t*>(child_array));


    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(string dirpath)
{
    cout << "FS::cd(" << dirpath << ")\n";


    dir_entry directory_array[64] = {0};
    int resultr = this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));

    if(resultf == -1 || resultr == -1){
        return -1;
    }

    int directoryindex = -1;
    for(int i = 0; i < 64; i++){
        if(directory_array[i].file_name[0] != '\0'){
            if(directory_array[i].file_name == dirpath){
                directoryindex = i;
                if(directory_array[i].type == TYPE_FILE){
                    cout << "FS::cd(" << dirpath << ") - ERROR: Directoryname is a file\n";
                    return -1;
                }
            }
        }
    }
    
    if(directoryindex != -1){
        this->cwb = directory_array[directoryindex].first_blk;
    }else{
        cout << "FS::cd(" << dirpath << ") - ERROR: Directory does not exists\n";
        return -1;
    }

    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    cout << "FS::pwd()\n";
    dir_entry directory_array[64] = {0};
    string fullpath = "";    
    dir_entry parent_array[64] = {0};
    int parentblock = cwb;
    int directoryblock = cwb;
    vector<string> paths;
    if(parentblock == ROOT_BLOCK){
        fullpath = "/";
    }
    this->disk.read(directoryblock,reinterpret_cast<uint8_t*>(directory_array));
    while(parentblock != ROOT_BLOCK){
        directoryblock = parentblock;
        this->disk.read(directoryblock,reinterpret_cast<uint8_t*>(directory_array));
        parentblock = directory_array[0].first_blk;
        this->disk.read(parentblock,reinterpret_cast<uint8_t*>(parent_array));
        for(int i = 0; i < 64; i++){
            if(parent_array[i].first_blk == directoryblock){
                int index = 0;
                string partpath = "/";
                while(parent_array[i].file_name[index] != '\0'){
                    partpath += parent_array[i].file_name[index];
                    index++;
                }
                paths.push_back(partpath);
            }
        }
    }

    for(int i = paths.size() -1; i >= 0; i--){
        fullpath += paths[i];
    }
    cout << fullpath << endl;
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
