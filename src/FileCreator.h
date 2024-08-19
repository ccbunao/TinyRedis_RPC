#ifndef FILECREATOR_H
#define FILECREATOR_H
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
    文件创建类
*/
class FileCreator {
    static bool createDirectory(const std::string& path);

    static void createAllParentDirectories(const std::string& path);

    static void createFilesInDirectory(const std::string& path, const std::string& fileName, int numberOfFiles);

    static bool isFileExists(const std::string& path);
public:
    static void createFolderAndFiles(const std::string& folderPath, const std::string& fileName, int numberOfFiles);
};

/**
 * 在给定路径下创建目录。如果目录已存在，则不进行任何操作。
 *
 * @param path 需要创建的目录的路径。
 * @return 如果成功创建或目录已存在，返回true；否则返回false。
 */
bool FileCreator::createDirectory(const std::string& path) {
    struct stat st = {0};
    if (stat(path.c_str(), &st) == -1) {
        // 目录不存在，尝试创建
        if (mkdir(path.c_str(), 0755) != -1) {
            return true; // 目录创建成功
        } else {
            std::cerr << "Failed to create directory: " << path << std::endl;
            return false; // 创建目录失败
        }
    }
    return true; // 目录已存在
}

/**
 * 创建给定路径中的所有父目录。
 *
 * @param path 需要创建父目录的完整文件路径。
 */
void FileCreator::createAllParentDirectories(const std::string& path) {
    size_t pos = 0;
    std::string currentPath;
    while ((pos = path.find_first_of("/\\", pos + 1)) != std::string::npos) {
        currentPath = path.substr(0, pos);
        createDirectory(currentPath);
    }
}

/**
 * 在指定路径下创建指定数量的文件。
 *
 * @param path 文件需要被创建的目录路径。
 * @param fileName 文件名，不包括扩展名。
 * @param numberOfFiles 需要创建的文件数量。
 */
void FileCreator::createFilesInDirectory(const std::string& path, const std::string& fileName, int numberOfFiles) {
    for (int i = 0; i < numberOfFiles; ++i) {
        std::ostringstream filePath;
        filePath << path << "/" << fileName << i;

        if (isFileExists(filePath.str())) {
            continue;
        }
        std::ofstream file(filePath.str());
        if (file.is_open()) {
            file.close();
        } else {
            std::cerr << "Unable to create file: " << filePath.str() << std::endl;
        }
    }
}

/**
 * 在指定的文件夹路径下创建指定数量的文件。
 *
 * @param folderPath 要创建文件的文件夹路径。
 * @param fileName   要创建的文件的名称。
 * @param numberOfFiles 要创建的文件的数量。
 */
void FileCreator::createFolderAndFiles(const std::string& folderPath, const std::string& fileName, int numberOfFiles) {
    createAllParentDirectories(folderPath);
    if (createDirectory(folderPath)) {
        
        createFilesInDirectory(folderPath, fileName, numberOfFiles);
    } else {
        std::cerr << "Unable to create or find folder: " << folderPath << std::endl;
    }
}

/**
 * 检查给定路径的文件是否存在。
 *
 * @param path 要检查的文件的路径。
 * @return 如果文件存在，则返回true；否则返回false。
 */
bool FileCreator::isFileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && !S_ISDIR(buffer.st_mode));
}


#endif 