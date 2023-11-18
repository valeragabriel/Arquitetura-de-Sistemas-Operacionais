#include "fs_aux.hpp"

void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes) {
       std::ofstream fs(fsFileName, std::ios::binary);

    if (!fs.is_open()) {
        std::cout << "File not found" << std::endl;
        return;
    }

    initFile(fsFileName, blockSize, numBlocks, numInodes);

    fs.close();
}

void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);
    if (!fs){
        std::cout << "File not found" << std::endl;
        return;
    }

    addFs(fsFileName, filePath, fileContent);

    fs.close();
}

void addDir(std::string fsFileName, std::string dirPath)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);

    if (!fs){
        std::cout << "Error opening the file" << std::endl;
        return;
    }

    addDiretorio(fsFileName, dirPath);

    fs.close();
}

void remove(std::string fsFileName, std::string path)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);
    if (!fs){
        std::cout << "File not found" << std::endl;
        return;
    }

    removeFile(fsFileName, path);

    fs.close();
}

void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
    std::fstream fs(fsFileName, std::ios::binary | std::ios::in | std::ios::out);
    if (!fs){
        std::cout << "File not found" << std::endl;
        return;
    }

    moveFile(fsFileName, oldPath, newPath);
    
    fs.close();
}
