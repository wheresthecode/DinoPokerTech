#include "pch.h"
#include "Utilities.h"
#include <sstream>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

bool ReadEntireFile(const char *path, std::vector<uint8_t> &outData)
{
    FILE *f = fopen(path, "rb");

    if (f == NULL)
        return false;

    const int fileSize = 0;
    fseek(f, 0L, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0L, SEEK_SET);

    outData.resize(sz);

    if (sz != 0)
    {
        size_t s = fread(&outData[0], 1, sz, f);
        if (s != outData.size())
        {
            return false;
        }
    }

    fclose(f);
    return true;
}

bool WriteDataToFile(const char *path, const void *data, int size)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL)
    {
        std::cerr << "Cannot open " << path << " for writing" << std::endl;
        return false;
    }

    size_t result = fwrite(data, size, 1, f);

    if (fclose(f) != 0)
    {
        std::cerr << "Cannot close " << path << " after writing" << std::endl;
        return false;
    }
    if (result != 1)
    {
        std::cerr << "Failed to write data of size " << size << " to file " << path << std::endl;
        return false;
    }
    return true;
}

bool WriteDataToFile(const std::string &path, std::vector<uint8_t> data)
{
    return WriteDataToFile(path.c_str(), data.size() != 0 ? &data[0] : NULL, data.size());
}
bool CreateDirectory(const char *path)
{
    if (!fs::is_directory(path) || !fs::exists(path))
    {                                      // Check if src folder exists
        return fs::create_directory(path); // create src folder
    }
    return true;
}
