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


/** ByteBuffer code */

struct ByteBuffer
{
    u8* data = 0;
    u32 position = 0;  // basically the cursor
    u32 size = 0;      // currently populated size
    u32 capacity = 0;  // currently allocated size
};

#define BYTE_BUFFER_DEFAULT_CAPACITY 1024

void ByteBufferInit(ByteBuffer* buffer)
{
    // TODO(Kevin): If using custom memory alloc, replace the following line
    buffer->data = (u8*) calloc(BYTE_BUFFER_DEFAULT_CAPACITY, sizeof(u8));
    buffer->position = 0;
    buffer->size = 0;
    buffer->capacity = BYTE_BUFFER_DEFAULT_CAPACITY;
}

ByteBuffer ByteBufferNew()
{
    ByteBuffer buffer = {0};
    ByteBufferInit(&buffer);
    return buffer;
}

void ByteBufferFree(ByteBuffer* buffer)
{
    if(buffer && buffer->data)
    {
        // TODO(Kevin): use custom mem alloc
        free(buffer->data);
    }
    buffer->size = 0;
    buffer->position = 0;
    buffer->capacity = 0;
}

void ByteBufferClear(ByteBuffer* buffer)
{
    buffer->size = 0;
    buffer->position = 0;
}

void ByteBufferResize(ByteBuffer* buffer, size_t sz)
{
    // TODO(Kevin): use custom mem alloc
    u8* data = (u8*)realloc(buffer->data, sz);
    if(data == NULL)
    {
        return;
    }
    buffer->data = data;
    buffer->capacity = (u32)sz;
}

void ByteBufferSeekToStart(ByteBuffer* buffer)
{
    buffer->position = 0;
}

void ByteBufferSeekToEnd(ByteBuffer* buffer)
{
    buffer->position = buffer->size;
}

void ByteBufferAdvancePosition(ByteBuffer* buffer, size_t sz)
{
    buffer->position += (u32)sz;
}

void __byteBufferWriteImpl(ByteBuffer* buffer, void* data, size_t sz)
{
    size_t totalWriteSize = buffer->position + sz;
    if(totalWriteSize >= buffer->capacity)
    {
        size_t capacity = buffer->capacity ? buffer->capacity * 2 : BYTE_BUFFER_DEFAULT_CAPACITY;
        while(capacity < totalWriteSize)
        {
            capacity *= 2;
        }
        ByteBufferResize(buffer, capacity);
    }
    memcpy(buffer->data + buffer->position, data, sz);
    buffer->position += (u32)sz;
    buffer->size += (u32)sz;
}

// Generic write function
#define ByteBufferWrite(_buffer, T, _val)\
do {\
    ByteBuffer* _bb = (_buffer);\
    size_t _sz = sizeof(T);\
    T _v = _val;\
    __byteBufferWriteImpl(_bb, (void*)&(_v), _sz);\
} while(0)

// Generic read function
#define ByteBufferRead(_buffer, T, _val_p)\
do {\
    T* _v = (T*)(_val_p);\
    ByteBuffer* _bb = (_buffer);\
    *(_v) = *(T*)(_bb->data + _bb->position);\
    _bb->position += sizeof(T);\
} while(0)

#define ByteBufferWriteBulk(_buffer, _data, _size)\
do {\
    __byteBufferWriteImpl(_buffer, _data, _size);\
} while(0)

#define ByteBufferReadBulk(_buffer, _dest_p, _size)\
do {\
    ByteBuffer* _bb = (_buffer);\
    memcpy((_dest_p), _bb->data + _bb->position, (_size));\
    _bb->position += (u32)(_size);\
} while(0)

int ByteBufferWriteToFile(ByteBuffer* buffer, const char* filePath)
{
    FILE* fp;
    fp = fopen(filePath, "w");

    if(!fp)
    {
        return 0;
    }

    fwrite(buffer->data, 1, buffer->size, fp);

    fclose(fp);

    return 1;
}

int ByteBufferReadFromFile(ByteBuffer* buffer, const char* filePath)
{
    if(!buffer)
    {
        return 0;
    }

    FILE* fp;
    fp = fopen(filePath, "r");

    if(!fp)
    {
        return 0;
    }

    if(buffer->data)
    {
        ByteBufferFree(buffer);
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    ByteBuffer bb = ByteBufferNew();
    if(bb.capacity < sz)
    {
        size_t capacity = bb.capacity;
        while(capacity < sz)
        {
            capacity *= 2;
        }
        ByteBufferResize(&bb, capacity);
    }

    fread(bb.data, 1, sz, fp);
    bb.size = (u32)sz;
    *buffer = bb;

    fclose(fp);

    return 1;
}
