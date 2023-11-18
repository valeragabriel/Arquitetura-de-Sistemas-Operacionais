#ifndef fs_aux_hpp
#define fs_aux_hpp

#include "fs.h"
#include <stdio.h>
#include "math.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>

const int CHAR_SIZE = sizeof(char);
const int INODE_SIZE = sizeof(INODE);
const char USED_BLOCK = 0x01;
const char UNUSED_BLOCK = 0x00;
const char DIRECTORY_FLAG = 0x01;
const char FILE_FLAG = 0x00;

void writeHeader(std::ofstream& fs, int blockSize, int numBlocks, int numInodes) {
    fs.write(reinterpret_cast<char*>(&blockSize), CHAR_SIZE);
    fs.write(reinterpret_cast<char*>(&numBlocks), CHAR_SIZE);
    fs.write(reinterpret_cast<char*>(&numInodes), CHAR_SIZE);
    fs.write(&USED_BLOCK, CHAR_SIZE);
}

void writeBitmap(std::ofstream& fs, int numBlocks) {
    int sizeOfBitmap = (numBlocks - 1) / 8;
    std::vector<char> empty(sizeOfBitmap, 0);
    fs.write(empty.data(), CHAR_SIZE * sizeOfBitmap);
}

void writeInodes(std::ofstream& fs, int numInodes){
    INODE root = {
        USED_BLOCK,     
        DIRECTORY_FLAG, 
        "/",             
        0x00,            
        {0x00, 0x00, 0x00}, 
        {0x00, 0x00, 0x00}, 
        {0x00, 0x00, 0x00}, 
    };

    fs.write(reinterpret_cast<char*>(&root), CHAR_SIZE * INODE_SIZE);
    std::vector<char> empty((numInodes - 1) * INODE_SIZE, 0);
    fs.write(empty.data(), CHAR_SIZE * empty.size());
    fs.write(empty.data(), CHAR_SIZE);
}

void writeBlocks(std::ofstream& fs, int numBlocks, int blockSize) {
    std::vector<char> empty(numBlocks * blockSize, 0);
    fs.write(empty.data(), CHAR_SIZE * empty.size());
}

void initFile(std::string fsFileName, int blockSize, int numBlocks, int numInodes) {
    std::ofstream fs(fsFileName, std::ios::binary);

    writeHeader(fs, blockSize, numBlocks, numInodes);
    writeBitmap(fs, numBlocks);
    writeInodes(fs, numInodes);
    writeBlocks(fs, numBlocks, blockSize);

}

bool readFileSystemInfo(std::fstream& fs, char& blockSize, char& numBlocks, char& numInodes) {
    size_t sizeOfChar = sizeof(char);

    if (fs.read(reinterpret_cast<char*>(&blockSize), sizeOfChar) &&
        fs.read(reinterpret_cast<char*>(&numBlocks), sizeOfChar) &&
        fs.read(reinterpret_cast<char*>(&numInodes), sizeOfChar)) {
        return true;
    }

    std::cout << "Error reading block size, number of blocks, or number of inodes" << std::endl;
    return false;
}

void addFs(std::string fsFileName, std::string filePath, std::string fileContent)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);

    char blockSize, numBlocks, numInodes;
    if (!readFileSystemInfo(fs, blockSize, numBlocks, numInodes)) {
        fs.close();
        return;
    }

    size_t sizeOfChar = sizeof(char);
    int sizeOfBitmap = (numBlocks / 8);
    int sizeOfInodeVector = numInodes * sizeof(INODE);
    int sizeOfBlockVector = numBlocks * blockSize;

    INODE inode;
    int freeInodeIndex = -1;
    int fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);

    int i = 0;
    while (i < numInodes) {
        if (!fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE))) {
            std::cout << "Error reading inode" << std::endl;
            fs.close();
            return;
        }

        if (inode.IS_USED == 0) {
            freeInodeIndex = i;
            break;
        }

        i++;
    }

    int directoriesCounter = 0, slashIndex = 0;
    std::string filePathString = filePath.c_str();
    for (size_t i = 0; i < filePathString.size(); i++) {
        if (filePathString[i] == '/') {
            directoriesCounter++;
            slashIndex = i;
        }
    }

    char directoryName[10], pathName[10];
    bool blockDirectoryName = false, blockPathName = false;
    for (size_t i = 0; i < sizeof(pathName); i++) {
        if (directoriesCounter == 1) {
            pathName[i] = (i + 1 < filePath.size()) ? filePath[i + 1] : 0;
            directoryName[i] = (filePath[i] == '/') ? '/' : 0;
        } else {
            if (!blockDirectoryName) {
                directoryName[i] = (i + 1 < filePath.size() && filePath[i + 1] != '/') ? filePath[i + 1] : 0;
            } else {
                directoryName[i] = 0;
            }

            if (!blockPathName) {
                pathName[i] = (slashIndex + 1 < filePath.size() && filePath[slashIndex + 1] != '/') ? filePath[slashIndex + 1] : 0;
            } else {
                pathName[i] = 0;
            }

            if (!blockDirectoryName || !blockPathName) {
                slashIndex++;
            }
        }
    }

    char blockValues[sizeOfBlockVector];
    fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
    fs.seekg(fileOffset, std::ios::beg);
    fs.read(reinterpret_cast<char*>(blockValues), sizeOfChar * sizeOfBlockVector);

    if (directoryName[0] != '/') {
        fileOffset = 3 + sizeOfBitmap;

        fs.seekg(fileOffset, std::ios::beg);
        
        for (int i = 0; i < numInodes; i++) {
            if (fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE))) {
                if (strcmp(inode.NAME, directoryName) == 0) {
                    char sizeOfDirectory = 0;
                    fileOffset = 3 + sizeOfBitmap + i * sizeof(INODE) + 12;
                    fs.seekg(fileOffset, std::ios::beg);
                    fs.read(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar);
                    fs.seekp(-1, std::ios::cur);
                    sizeOfDirectory++;
                    fs.write(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar);
                    fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + blockSize * inode.DIRECT_BLOCKS[0];
                    fs.seekp(fileOffset, std::ios::beg);
                    fs.write(reinterpret_cast<char*>(&freeInodeIndex), sizeOfChar);
                    fileOffset = 3 + sizeOfBitmap + i * sizeof(INODE) + sizeof(INODE);
                    fs.seekp(fileOffset, std::ios::beg);
                }
            } else {
                std::cout << "Error reading inode" << std::endl;
                fs.close();
                return;
            }
        }
    }
    char freeBlockIndex = 0;

    fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);
    for (int i = 0; i < numInodes; i++) {
        if (!fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE))) {
            std::cout << "Error reading inode" << std::endl;
            fs.close();
            return;
        }

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++) {
            if (inode.DIRECT_BLOCKS[j] > freeBlockIndex) {
                freeBlockIndex = inode.DIRECT_BLOCKS[j];
            }
        }
    }
    freeBlockIndex++;

    char setFileInodeUsed[2] = {0x01, 0x00};
    fileOffset = 3 + sizeOfBitmap + sizeof(INODE) * freeInodeIndex;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(setFileInodeUsed), sizeOfChar * 2);
    fs.write(pathName, sizeOfChar * 10);

    int sizeOfContent = strlen(fileContent.c_str());
    fs.write(reinterpret_cast<char*>(&sizeOfContent), sizeOfChar);
    char numberOfBlocks = ceil(sizeOfContent / (double)blockSize);

    for (size_t i = 0; i < numberOfBlocks; i++) {
        fs.write(reinterpret_cast<char*>(&freeBlockIndex), sizeOfChar);
        freeBlockIndex++;
    }

    char contentOfFile[sizeOfContent];
    strcpy(contentOfFile, fileContent.c_str());

    char directoryBlock[2];
    directoryBlock[0] = 1;
    directoryBlock[1] = (directoryName[0] != '/') ? 2 : 0;

    fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(directoryBlock), sizeOfChar * 2);

    fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + blockSize * (freeBlockIndex - numberOfBlocks);
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(contentOfFile, sizeOfContent);
  
    int numberOfChildren = (directoryName[0] != '/') ? 2 : 1;
    fileOffset = 3 + sizeOfBitmap + 12;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&numberOfChildren), sizeOfChar);

    char numberOfBlocksUsed = 0;

    fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);
    for (int i = 0; i < numInodes; i++) {
        if (!fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE))) {
            std::cout << "Error reading inode" << std::endl;
            fs.close();
            return;
        }

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++) {
            if (inode.DIRECT_BLOCKS[j] > numberOfBlocksUsed) {
                numberOfBlocksUsed = inode.DIRECT_BLOCKS[j];
            }
        }
    }

    int accumulatedBinaryValue = 0;
    for (size_t i = 0; i <= numberOfBlocksUsed; i++) {
        accumulatedBinaryValue += pow(2, i);
    }

     fileOffset = 3;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&accumulatedBinaryValue), sizeOfChar);
    
}

void addDiretorio(std::string fsFileName, std::string dirPath)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);

    char blockSize, numBlocks, numInodes;
    if (!readFileSystemInfo(fs, blockSize, numBlocks, numInodes)) {
        fs.close();
        return;
    }

    size_t sizeOfChar = sizeof(char);
    int sizeOfInodeVector = numInodes * sizeof(INODE);
    int sizeOfBlockVector = numBlocks * blockSize;
    char sizeOfBitmap = static_cast<char>(std::ceil((numBlocks - 1) / 8.0));
    char blockValues[sizeOfBlockVector];
    int fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
    fs.seekg(fileOffset, std::ios::beg);
    fs.read(reinterpret_cast<char*>(blockValues), sizeOfChar * sizeOfBlockVector);

    int freeBlockIndex = -1;

    for (int i = 0; i < numBlocks; i++){
        int blockStart = i * blockSize;
        int blockEnd = blockStart + blockSize;
        bool isBlockFree = true;

        for (int j = blockStart; j < blockEnd; j++){
            if (blockValues[j] != 0){
                isBlockFree = false;
                break;
            }
        }
        if (isBlockFree){
            freeBlockIndex = i;
            break;
        }
    }

    if (freeBlockIndex == -1){
        std::cout << "No free blocks" << std::endl;
        fs.close();
        return;
    }

    INODE inode;
    int freeInodeIndex = -1;
    fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);

    for (int i = 0; i < numInodes && fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE)); i++) {
        if (inode.IS_USED == 0) {
            freeInodeIndex = i;
            break;
        }
    }

    char dirName[10];
    size_t dirPathLength = dirPath.length();

    for (size_t i = 0; i < sizeof(dirName); i++) {
        if (i + 1 < dirPathLength) {
            dirName[i] = dirPath[i + 1];
        } else {
            dirName[i] = 0;
        }
    }

    fileOffset = 3 + sizeOfBitmap + freeInodeIndex * sizeof(INODE);
    char setDirectory[] = {0x01, 0x01};
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(setDirectory), sizeOfChar * 2);
    fs.write(dirName, sizeOfChar * 10);
    fs.seekp(1, std::ios::cur);
    fs.write(reinterpret_cast<char*>(&freeBlockIndex), sizeOfChar * 1);

    int numberOfBlocks = 1;
    fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);
    int numberOfInodesFilled = -1;

    for (int i = 0; i < numInodes && fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE)); i++) {
        if (inode.IS_USED == 1) {
            numberOfInodesFilled++;
            for (int j = 0; j < 3; j++) {
                if (inode.DIRECT_BLOCKS[j] > 0) {
                    numberOfBlocks++;
                }
            }
        }
    }

    int bitMap = (1 << numberOfBlocks) - 1;
    fs.seekp(3, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&bitMap), sizeOfChar * 1);
    fileOffset = 3 + sizeOfBitmap + 12;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&numberOfInodesFilled), sizeOfChar * 1);
    int valueInRootBlock = 2;
    fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 2;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&valueInRootBlock), sizeOfChar * 1);
    
}

void removeFile(std::string fsFileName, std::string path)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);

    char blockSize, numBlocks, numInodes;
    if (!readFileSystemInfo(fs, blockSize, numBlocks, numInodes)) {
        fs.close();
        return;
    }
    size_t sizeOfChar = sizeof(char);
    char sizeOfBitmap = static_cast<char>(std::ceil(numBlocks / 8.0)); 
    int sizeOfInodeVector = numInodes * sizeof(INODE);                 
    int sizeOfBlockVector = numBlocks * blockSize;  

    int directoriesCounter = 0, slashIndex = 0; 
    char directoryName[10], pathName[10];       
    const std::string& pathStr = path; 
    slashIndex = pathStr.find_last_of('/');
    directoriesCounter = (slashIndex != std::string::npos) ? slashIndex + 1 : 0;

    bool blockDirectoryName = false, blockPathName = false;
    size_t pathLength = path.length();

    for (size_t i = 0; i < sizeof(pathName) && i < pathLength; i++) {
        if (directoriesCounter == 1) {
            pathName[i] = pathStr[i + 1];
            directoryName[i] = (pathStr[i] == '/') ? '/' : 0;
        } else {
            if (pathStr[i + 1] != '/' && !blockDirectoryName) {
                directoryName[i] = pathStr[i + 1];
            } else {
                directoryName[i] = 0;
                blockDirectoryName = true;
            }

            if (pathStr[slashIndex + 1] != '/' && !blockPathName) {
                pathName[i] = pathStr[slashIndex + 1];
            } else {
                pathName[i] = 0;
                blockPathName = true;
            }
            slashIndex++;
        }
    }

    INODE inode;
    int fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);
    int indexOfPath = 0, indexOfDirectory = 0;
    bool foundPath = false, foundDirectory = false;
    for (int i = 0; i < numInodes && (!foundPath || !foundDirectory); i++)    {
        fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE));
        if (!foundPath && !strcmp(inode.NAME, pathName)){
            indexOfPath = i;
            foundPath = true;
        }
        if (!foundDirectory && !strcmp(inode.NAME, directoryName)){
            indexOfDirectory = i;
            foundDirectory = true;
        }
    }

    fileOffset = 3 + sizeOfBitmap + sizeof(INODE) * indexOfPath;

    INODE emptyInode;
    memset(&emptyInode, 0, sizeof(INODE));

    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&emptyInode), sizeof(INODE));

    char sizeOfDirectory = 0;
    fileOffset = 3 + sizeOfBitmap + indexOfDirectory * sizeof(INODE) + 12;
    fs.seekg(fileOffset, std::ios::beg);
    fs.read(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar * 1);
    sizeOfDirectory--;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar * 1);
    
    INODE newInode;
    std::vector<char> bitMapValue(numBlocks, 0);
    bitMapValue[0] = 1;

    fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);
    for (int i = 0; i < numInodes; i++) {
        fs.read(reinterpret_cast<char*>(&newInode), sizeof(INODE));
        for (size_t j = 0; j < sizeof(newInode.DIRECT_BLOCKS) / sizeof(newInode.DIRECT_BLOCKS[0]); j++) {
            if (newInode.DIRECT_BLOCKS[j] != 0) {
                bitMapValue[newInode.DIRECT_BLOCKS[j]] = 1;
            }
        }
    }

    int accumulatedBinaryValue = 0;
    for (size_t i = 0; i < bitMapValue.size(); i++) {
        accumulatedBinaryValue |= (bitMapValue[i] << i);
    }
    fileOffset = 3;
    fs.seekp(fileOffset, std::ios::beg);
    fs.write(reinterpret_cast<char*>(&accumulatedBinaryValue), sizeOfChar * 1);
    
    int filledInodeCounter = -1;
    char indexOfInodeUsed = 0;
    fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);
    for (int i = 0; i < numInodes; i++){
        fs.read(reinterpret_cast<char*>(&newInode), sizeof(INODE)); 
        if (newInode.IS_USED == 1)
        {
            filledInodeCounter++;
            indexOfInodeUsed = i;
            if (filledInodeCounter > 1)
            {
                break;
            }
        }
    }

    if (filledInodeCounter == 1){
        fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1;
        fs.seekp(fileOffset, std::ios::beg);
        fs.write(reinterpret_cast<char*>(&indexOfInodeUsed), sizeOfChar * 1);
    }

}

void moveFile(std::string fsFileName, std::string oldPath, std::string newPath)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);

    char blockSize, numBlocks, numInodes;
    if (!readFileSystemInfo(fs, blockSize, numBlocks, numInodes)) {
        fs.close();
        return;
    }
    size_t sizeOfChar = sizeof(char);
    char sizeOfBitmap = static_cast<char>(std::ceil(numBlocks / 8.0)); 
    int sizeOfInodeVector = numInodes * sizeof(INODE);                 
    int sizeOfBlockVector = numBlocks * blockSize;                     
                  
    int directoriesCounter = 0;
    size_t slashIndex = 0;
    const std::string& oldPathStr = oldPath;
    char directoryName[10], pathName[10]; 

    for (size_t i = 0; i < oldPathStr.length(); i++){
        if (oldPathStr.at(i) == '/'){
            directoriesCounter++;
            slashIndex = 0;
        }
    }

    bool blockDirectoryName = false, blockPathName = false;
    for (size_t i = 0; i < sizeof(pathName); i++){
        if (directoriesCounter == 1){
            if ((i + 1) < oldPath.length())
                pathName[i] = oldPath[i + 1];
            else
                pathName[i] = 0;

            if ((i + 1) < oldPath.length() && oldPath[i] == '/')
                directoryName[i] = '/';
            else
                directoryName[i] = 0;
        } else {
            if ((i + 1) < oldPath.length() && oldPath[i + 1] != '/' && !blockDirectoryName)
                directoryName[i] = oldPath[i + 1];
            else{
                directoryName[i] = 0;
                blockDirectoryName = true;
            }
            if ((slashIndex + 1) < oldPath.length() && oldPath[slashIndex + 1] != '/' && !blockPathName)
                pathName[i] = oldPath[slashIndex + 1];
            else{
                pathName[i] = 0;
                blockPathName = true;
            }
            slashIndex++;
        }
    }

    directoriesCounter = 0;
    slashIndex = newPath.find_last_of('/');

    char newDirectoryName[10] = "";
    char newPathName[10] = "";

    if (slashIndex != std::string::npos){
        directoriesCounter = std::count(newPath.begin(), newPath.end(), '/');
    }

    if (slashIndex != std::string::npos){
        strncpy(newDirectoryName, newPath.c_str(), slashIndex);
        strncpy(newPathName, newPath.c_str() + slashIndex + 1, 9); 
    }

    bool blocknewDirectoryName = false, blocknewPathName = false;
    for (size_t i = 0; i < sizeof(newPathName); i++){
        if (directoriesCounter == 1){
            char nextChar = (i + 1 < newPath.length()) ? newPath[i + 1] : 0;
            newPathName[i] = nextChar;
            newDirectoryName[i] = (nextChar == '/') ? '/' : 0;
        }
        else{
            if (!blocknewDirectoryName && (i + 1 < newPath.length()) && (newPath[i + 1] != '/'))
                newDirectoryName[i] = newPath[i + 1];
            else{
                newDirectoryName[i] = 0;
                blocknewDirectoryName = true;
            }           
        }
    }

    INODE inode;
    int fileOffset = 3 + sizeOfBitmap;
    fs.seekg(fileOffset, std::ios::beg);
    int indexOfPath = -1, indexOfOldDirectory = -1, indexOfNewDirectory = -1;

    for (int i = 0; i < numInodes; i++){
        fs.read(reinterpret_cast<char*>(&inode), sizeof(INODE));

        if (indexOfPath == -1 && strcmp(inode.NAME, pathName) == 0)
            indexOfPath = i;

        if (indexOfOldDirectory == -1 && strcmp(inode.NAME, directoryName) == 0)
            indexOfOldDirectory = i;

        if (indexOfNewDirectory == -1 && strcmp(inode.NAME, newDirectoryName) == 0)
            indexOfNewDirectory = i;
    }

    if (strcmp(pathName, newPathName) && indexOfPath != -1){
        fileOffset = 3 + sizeOfBitmap + indexOfPath * sizeof(INODE) + 2;
        fs.seekp(fileOffset, std::ios::beg);
        fs.write(newPathName, sizeOfChar * sizeof(newPathName));
    }
    else{
        fileOffset = 3 + sizeOfBitmap + indexOfOldDirectory * sizeof(INODE) + 12;
        fs.seekp(fileOffset, std::ios::beg);
        char sizeOfDirectory;
        fs.read(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar);
        sizeOfDirectory--;
        fs.seekp(-1, std::ios::cur);
        fs.write(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar);

        fileOffset = 3 + sizeOfBitmap + indexOfOldDirectory * sizeof(INODE);
        INODE inodeToRemoveBlock;
        fs.seekg(fileOffset, std::ios::beg);
        fs.read(reinterpret_cast<char*>(&inodeToRemoveBlock), sizeof(INODE));

        int usedBlocks = 0;
        for (int i = 0; i < 10; i++) { 
            if (inodeToRemoveBlock.DIRECT_BLOCKS[i] != 0) {
                usedBlocks++;
            }
        }
        if (inodeToRemoveBlock.NAME[0] == '/') {
            usedBlocks++;
        }

        size_t listOfValuesSize = usedBlocks * blockSize;
        char listOfValuesOfBlock[listOfValuesSize];

        int outputIndex = 0;
        for (size_t i = 0; i < usedBlocks; i++){
            char tempValues[blockSize];
            int fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * blockSize;
            fs.seekg(fileOffset, std::ios::beg);
            fs.read(tempValues, sizeOfChar * blockSize);

            memcpy(listOfValuesOfBlock + outputIndex, tempValues, blockSize);

            outputIndex += blockSize;
        }

        size_t i = 0;
        while (i < listOfValuesSize){
            if (i + 1 < listOfValuesSize && listOfValuesOfBlock[i + 1] != 0){
                if (listOfValuesOfBlock[i] == indexOfPath || (i != 0 && listOfValuesOfBlock[i] == listOfValuesOfBlock[i - 1])){
                    listOfValuesOfBlock[i] = listOfValuesOfBlock[i + 1];
                }
            }
            i += 1;
        }
        for (size_t i = 0; i < usedBlocks; i++){
            char tempValues[blockSize];
            for (size_t j = 0; j < blockSize; j++){
                tempValues[j] = listOfValuesOfBlock[i * blockSize + j];
            }

            int fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * blockSize;
            fs.seekp(fileOffset, std::ios::beg);
            fs.write(tempValues, sizeOfChar * blockSize); 
        }

        if (usedBlocks * 2 > sizeOfDirectory)
        {
            int numBlocksToRemove = usedBlocks * 2 - sizeOfDirectory;
            for (int i = sizeOfDirectory - 1; i >= 0 && numBlocksToRemove > 0; i--){
                if (inodeToRemoveBlock.DIRECT_BLOCKS[i] != 0){
                    inodeToRemoveBlock.DIRECT_BLOCKS[i] = 0;
                    numBlocksToRemove--;
                }
            }
            int fileOffset = 3 + sizeOfBitmap + indexOfOldDirectory * sizeof(INODE);
            fs.seekp(fileOffset, std::ios::beg);
            fs.write(reinterpret_cast<char*>(&inodeToRemoveBlock), sizeOfChar * sizeof(INODE)); 
        }

        sizeOfDirectory = 0;
        fileOffset = 3 + sizeOfBitmap + indexOfNewDirectory * sizeof(INODE) + 12;
        fs.seekg(fileOffset, std::ios::beg);
        fs.read(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar);
        sizeOfDirectory++;
        fs.seekp(-1, std::ios::cur);
        fs.write(reinterpret_cast<char*>(&sizeOfDirectory), sizeOfChar); 

        int lastBlockUsed = 0;
        fileOffset = 3 + sizeOfBitmap + indexOfNewDirectory * sizeof(INODE);
        INODE newInode;
        fs.seekg(fileOffset, std::ios::beg);
        fs.read(reinterpret_cast<char*>(&newInode), sizeOfChar * sizeof(INODE));
        for (size_t i = 0; i < sizeof(newInode.DIRECT_BLOCKS) / sizeOfChar; i++) {
            if (newInode.DIRECT_BLOCKS[i] != 0 && indexOfPath != newInode.DIRECT_BLOCKS[i])
                lastBlockUsed = i;
        }

        int remainingSize = sizeOfDirectory - 1;
        while (remainingSize > blockSize) {
            remainingSize -= blockSize;
        }
        
        int indexOfDirectoryBlock = newInode.DIRECT_BLOCKS[lastBlockUsed] * 2 + remainingSize;
        if (sizeOfDirectory > blockSize) {
            int availableBlockIndex = 0; 
            fs.seekg(3 + sizeOfBitmap, std::ios::beg);
            for (int i = 0; i < numInodes; i++) {
                fs.read(reinterpret_cast<char*>(&inode), sizeOfChar * sizeof(INODE)); 
                for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS) / sizeOfChar; j++) {
                    if (inode.DIRECT_BLOCKS[j] > availableBlockIndex) {
                        availableBlockIndex = inode.DIRECT_BLOCKS[j];
                    }
                }
            }
            indexOfDirectoryBlock = availableBlockIndex * 2;            
            int indexOfNewDirectoryPosition = 3 + sizeOfBitmap + indexOfNewDirectory * sizeof(INODE);
            fs.seekg(indexOfNewDirectoryPosition, std::ios::beg);
            fs.read(reinterpret_cast<char*>(&newInode), sizeOfChar * sizeof(INODE));

            for (size_t i = 1; i < sizeof(newInode.DIRECT_BLOCKS) / sizeOfChar; i++) {
                if (newInode.DIRECT_BLOCKS[i] == 0) {
                    newInode.DIRECT_BLOCKS[i] = availableBlockIndex;
                    break;
                }
            }

            fs.seekp(indexOfNewDirectoryPosition, std::ios::beg);
            fs.write(reinterpret_cast<char*>(&newInode), sizeOfChar * sizeof(INODE));
        }

        fileOffset = 3 + sizeOfBitmap + sizeOfInodeVector + 1 + indexOfDirectoryBlock;
        fs.seekp(fileOffset, std::ios::beg);
        fs.write(reinterpret_cast<char*>(&indexOfPath), sizeOfChar); 

        char bitMapValue[numBlocks];
        memset(bitMapValue, 0, numBlocks); 
        bitMapValue[0] = 1; 

        fileOffset = 3 + sizeOfBitmap;
        fs.seekg(fileOffset, std::ios::beg);
        for (int i = 0; i < numInodes; i++) {
            fs.read(reinterpret_cast<char*>(&newInode), sizeOfChar * sizeof(INODE)); 

            for (int j = 0; j < sizeof(newInode.DIRECT_BLOCKS) / sizeOfChar; j++) {
                if (newInode.DIRECT_BLOCKS[j] != 0) {
                    bitMapValue[newInode.DIRECT_BLOCKS[j]] = 1;
                }
            }
        }

        int accumulatedBinaryValue = 0; 
        int bitPosition = 1;

        for (int i = 0; i < numBlocks; i++) {
            if (bitMapValue[i] != 0) {
                accumulatedBinaryValue |= bitPosition;
            }
            bitPosition <<= 1;
        }

        fileOffset = 3;
        fs.seekp(fileOffset, std::ios::beg);
        fs.write(reinterpret_cast<char*>(&accumulatedBinaryValue), sizeOfChar); 
        
    }
}

#endif /* fs_aux_hpp */