// TODO(Kevin): Once we have custom memory allocators, replace the mallocs/frees in this file.


struct BinaryFileHandle
{
    /** Handle for a file in memory */
    u32     size   = 0;        // size of file in memory
    void*   memory = nullptr;  // pointer to file in memory
};

struct BitmapHandle : BinaryFileHandle
{
    /** Handle for an UNSIGNED BYTE bitmap in memory */
    u32 width    = 0;   // image width
    u32 height   = 0;   // image height
    u8  bitDepth = 0;   // bit depth of bitmap in bytes (e.g. bit depth = 3 means there are 3 bytes in the bitmap per pixel)
};

INTERNAL void FreeFileBinary(BinaryFileHandle& binaryFileToFree)
{
    free(binaryFileToFree.memory);
    binaryFileToFree.memory = nullptr;
    binaryFileToFree.size = 0;
}

/** Allocates memory, stores the binary file data in memory, makes 
    BinaryFileHandle.memory point to it. Pass along a BinaryFileHandle 
    to receive the pointer to the file data in memory and the size in bytes. */
INTERNAL void ReadFileBinary(BinaryFileHandle& memToReadTo, const char* filePath)
{
    if(memToReadTo.memory)
    {
        printf("WARNING: Binary File Handle already points to allocated memory. Freeing memory first...\n");
        FreeFileBinary(memToReadTo);
    }

    SDL_RWops* binaryFileRW = SDL_RWFromFile(filePath, "rb");
    if(binaryFileRW)
    {
        memToReadTo.size = (u32)SDL_RWsize(binaryFileRW); // total size in bytes
        memToReadTo.memory = malloc(memToReadTo.size);
        SDL_RWread(binaryFileRW, memToReadTo.memory, memToReadTo.size, 1);
        SDL_RWclose(binaryFileRW);
    }
    else
    {
        printf("Failed to read %s! File doesn't exist.\n", filePath);
        return;
    }
}

/** Returns the string content of a file as an std::string */
INTERNAL std::string ReadFileString(const char* filePath)
{
    std::string stringContent;

    std::ifstream fileStream(filePath, std::ios::in);
    if (fileStream.is_open() == false)
    {
        printf("Failed to read %s! File doesn't exist.\n", filePath);
    }

    std::string line = "";
    while (fileStream.eof() == false)
    {
        std::getline(fileStream, line);
        stringContent.append(line + "\n");
    }

    fileStream.close();

    return stringContent;
}

INTERNAL void FreeImage(BitmapHandle& imageHandle)
{
    FreeFileBinary(imageHandle);
    imageHandle.width = 0;
    imageHandle.height = 0;
    imageHandle.bitDepth = 0;
}

/** Allocates memory, loads an image file as an UNSIGNED BYTE bitmap, makes BitmapHandle.memory
    point to it. Pass along a BitmapHandle to receive the pointer to the bitmap in memory and
    bitmap information. */
INTERNAL void ReadImage(BitmapHandle& imageHandle, const char* imageFilePath)
{
    if(imageHandle.memory)
    {
        printf("WARNING: Binary File Handle already points to allocated memory. Freeing memory first...\n");
        FreeImage(imageHandle);
    }

    imageHandle.memory = stbi_load(imageFilePath, (int*)&imageHandle.width, (int*)&imageHandle.height, (int*)&imageHandle.bitDepth, 0);
    if(imageHandle.memory)
    {
        imageHandle.size = imageHandle.width * imageHandle.height * imageHandle.bitDepth;
    }
    else
    {
        printf("Failed to find image file at: %s\n", imageFilePath);
        imageHandle.width = 0;
        imageHandle.height = 0;
        imageHandle.bitDepth = 0;
        return;
    }
}