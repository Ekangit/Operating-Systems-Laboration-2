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
    int resultr = this->disk.read(ROOT_BLOCK,reinterpret_cast<uint8_t*>(directory_array));
    int resultf = this->disk.read(FAT_BLOCK,reinterpret_cast<uint8_t*>(this->fat));
    
    vector<string> filepaths; 
    int y = 0;
    char current = cwd[y];
    string directory = "";
    bool first = true;

    while(current != '\0'){
        if(cwd[y] == '/'){
            if(first == false){
                filepaths.push_back(directory);
                directory = "";
            }
            else {
                first = false;
            }
                 
        }else{
            directory += current;
        }
        
        y++;
        current = cwd[y];
    }
    filepaths.push_back(directory);

    for(int k = 0; k < filepaths.size(); k++){
        int j = 0;
        while(j < 64 && filepaths[k] != directory_array[j].file_name){
            j++;
        }
        if(filepaths[k] == directory_array[j].file_name){
            
            cwb = directory_array[j].first_blk;

            this->disk.read(cwb,reinterpret_cast<uint8_t*>(directory_array));
        }else{
            cout << "Create(" << filepath<< ") - ERROR: File path incorrect \n";
            return -1;
        }
    }

    if(resultr == 0 && resultf == 0){
        for(int i = 0; i < 64; i++){
            if(directory_array[i].file_name[0] != '\0'){
                if(directory_array[i].file_name == filepath){
                    cout << "Create(" << filepath<< ") - ERROR: File already exists \n";
                    return -1;
                }  
            }
        }

        bool inStreamEmpty = false;
        int fileSize = 0;
        bool firstBuffer = true;
        int oldOpenBlock;
        vector<char[4096]> totalFileBuffer;
        dir_entry newFile;

        while(inStreamEmpty == false){
            char buffer[4096] = {0};
            cin.read(buffer, 4096);
            int bufferSize = cin.gcount();
            fileSize += bufferSize;
            

            if(bufferSize != 4096){
                inStreamEmpty = true;
            }
            int x = 2;
            while(x < 2048 && fat[x] != 0){
                x++;
            }

            if(fat[x] == 0){
                
                newFile.type = 0;
                if(firstBuffer){
                    newFile.first_blk = x;
                }
                newFile.access_rights = 6;
                for(int i = 0; i < filepath.size(); i++){
                    newFile.file_name[i] = filepath[i];
                }
                
                if(!firstBuffer){
                    if(bufferSize < 4096){
                        fat[x] = FAT_EOF;
                    }
                    fat[oldOpenBlock] = x;
                }
                oldOpenBlock = x;
                
               totalFileBuffer.push_back(buffer);


            }else{
                cout << "Create(" << filepath<< ") - ERROR: File cant fit \n";
                return -1;
            }
            
            if(firstBuffer){
                firstBuffer = false;
            }

           
        }
        int fileBlock = newFile.first_blk;

        
        newFile.size = fileSize;
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
        for(int i = 0; i < totalFileBuffer.size(); i++){
            this->disk.write(fileBlock,reinterpret_cast<uint8_t*>(totalFileBuffer[i]));
            fileBlock = fat[fileBlock];
        }
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
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    cout << "FS::ls()\n";
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(string sourcepath, string destpath)
{
    cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
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
