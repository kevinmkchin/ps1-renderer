
struct EditorObject
{
    // PARENT
    // CHILDREN
    // WE CAN HAVE EMPTY EDITOR OBJECT WHICH HAS NON-EMPTY EDITOR OBJECTS AS CHILDREN

    virtual std::pair<std::vector<float>, std::vector<u32>> GetBufferObjects() = 0;
    virtual std::vector<vec3> GetColliders() = 0;
};

// map from GameObject to EditorObject

struct EditorCubeObject : EditorObject
{
// - cube with arbitrary xyz lengths
//      - need to set vertex position XYZ based on size
//      - need to set texture coordinates UVs based on size
//      - normal vectors are constant
//      - indices into the above ^ can stay constant
//      - we need to set collider values appropriately based on above vao

    vec3 dimensions; // width, height, length
    vec3 originAsOffsetFromMinCorner = vec3(2.f, 0.2f, 1.f);
    vec2 textureScale = vec2(4.f, 2.f); // i.e. how many world units is one UV unit
    bool textureTileFlag = true;
    bool textureOrientationFlag = true;

    EditorCubeObject(vec3 initDimensions)
    {
        dimensions = initDimensions;
    }

    std::pair<std::vector<float>, std::vector<u32>> GetBufferObjects() override
    {
        // Get Mesh VBO for vertex position, texture coords, normals, indices
        // Use Mesh VBOs to rebind a mesh on a Mesh+GameObject in the scene.
        // So there is a 1 to 1 mapping between the EditorObjects and GameObjects in the scene.
#pragma region MakingBufferObjects
        vec3 posArray[24] = {
                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {0.f, dimensions.y, dimensions.z},
                {0.f, dimensions.y, 0.f},

                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z },
                {dimensions.x, dimensions.y, dimensions.z},
                {0.f, dimensions.y, dimensions.z},

                {dimensions.x, 0.f, 0.f},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {dimensions.x, 0.f, 0.f},
                {dimensions.x, dimensions.y, 0.f},
                {0.f, dimensions.y, 0.f},

                {0.f, dimensions.y, 0.f},
                {0.f, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, 0.f, 0.f}
        };
        vec2 uvArray[24] = {
                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.x / textureScale.x, 0.f},
                {dimensions.x / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.x / textureScale.x, 0.f},
                {dimensions.x / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.x / textureScale.y},
                {0.f, dimensions.x / textureScale.y},

                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.x / textureScale.y},
                {0.f, dimensions.x / textureScale.y},
        };

        if(!textureTileFlag)
        {
            uvArray[0] = {0.f, 0.f};
            uvArray[1] = {1.f, 0.f};
            uvArray[2] = {1.f, 1.f};
            uvArray[3] = {0.f, 1.f};

            uvArray[4] = {0.f, 0.f};
            uvArray[5] = {1.f, 0.f};
            uvArray[6] = {1.f, 1.f};
            uvArray[7] = {0.f, 1.f};

            uvArray[8] = {0.f, 0.f};
            uvArray[9] = {1.f, 0.f};
            uvArray[10] = {1.f, 1.f};
            uvArray[11] = {0.f, 1.f};

            uvArray[12] = {0.f, 0.f};
            uvArray[13] = {1.f, 0.f};
            uvArray[14] = {1.f, 1.f};
            uvArray[15] = {0.f, 1.f};

            uvArray[16] = {0.f, 0.f};
            uvArray[17] = {1.f, 0.f};
            uvArray[18] = {1.f, 1.f};
            uvArray[19] = {0.f, 1.f};

            uvArray[20] = {0.f, 0.f};
            uvArray[21] = {1.f, 0.f};
            uvArray[22] = {1.f, 1.f};
            uvArray[23] = {0.f, 1.f};
        }

        const vec3 normalArray[24] = {
                {-1.f, 0.f, 0.f},
                {-1.f, 0.f, 0.f},
                {-1.f, 0.f, 0.f},
                {-1.f, 0.f, 0.f},

                {0.f, 0.f, 1.f},
                {0.f, 0.f, 1.f},
                {0.f, 0.f, 1.f},
                {0.f, 0.f, 1.f},

                {1.f, 0.f, 0.f},
                {1.f, 0.f, 0.f},
                {1.f, 0.f, 0.f},
                {1.f, 0.f, 0.f},

                {0.f, 0.f, -1.f},
                {0.f, 0.f, -1.f},
                {0.f, 0.f, -1.f},
                {0.f, 0.f, -1.f},

                {0.f, 1.f, 0.f},
                {0.f, 1.f, 0.f},
                {0.f, 1.f, 0.f},
                {0.f, 1.f, 0.f},

                {0.f, -1.f, 0.f},
                {0.f, -1.f, 0.f},
                {0.f, -1.f, 0.f},
                {0.f, -1.f, 0.f}
        };

        std::vector<float> vbo;
        std::vector<u32> indices = {
                0, 1, 2,
                3, 0, 2,

                4, 5, 6,
                7, 4, 6,

                8, 10, 9,
                11, 10, 8,

                12, 14, 13,
                15, 14, 12,

                16, 17, 18,
                19, 16, 18,

                20, 22, 21,
                23, 22, 20
        };

        for(int i = 0; i < 24; ++i)
        {
            // and THEN we use the origin offset to go and offset the vertex positions
            posArray[i].x -= originAsOffsetFromMinCorner.x;
            posArray[i].y -= originAsOffsetFromMinCorner.y;
            posArray[i].z -= originAsOffsetFromMinCorner.z;
        }

        if(textureOrientationFlag)
        {
            if(textureTileFlag)
            {
                uvArray[16] = {0.f, dimensions.z / textureScale.y};
                uvArray[17] = {0.f, 0.f};
                uvArray[18] = {dimensions.x / textureScale.x, 0.f};
                uvArray[19] = {dimensions.x / textureScale.x, dimensions.z / textureScale.y};

                uvArray[20] = {0.f, dimensions.z / textureScale.y};
                uvArray[21] = {0.f, 0.f};
                uvArray[22] = {dimensions.x / textureScale.x, 0.f};
                uvArray[23] = {dimensions.x / textureScale.x, dimensions.z / textureScale.y};
            }
            else
            {
                uvArray[16] = {0.f, 1.f};
                uvArray[17] = {0.f, 0.f};
                uvArray[18] = {1.f, 0.f};
                uvArray[19] = {1.f, 1.f};

                uvArray[20] = {0.f, 1.f};
                uvArray[21] = {0.f, 0.f};
                uvArray[22] = {1.f, 0.f};
                uvArray[23] = {1.f, 1.f};
            }
        }

        // populate vbo
        for(int i = 0; i < 24; ++i)
        {
            vbo.push_back(posArray[i].x);
            vbo.push_back(posArray[i].y);
            vbo.push_back(posArray[i].z);
            vbo.push_back(uvArray[i].x);
            vbo.push_back(uvArray[i].y);
            vbo.push_back(normalArray[i].x);
            vbo.push_back(normalArray[i].y);
            vbo.push_back(normalArray[i].z);
        }
#pragma endregion
        return { vbo, indices };
    }
    std::vector<vec3> GetColliders() override
    {
#pragma region MakeColliderVertices
        // Returns a vector of vertex positions for use with MeshCollider
        std::vector<vec3> colliderVertices = {
                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {0.f, dimensions.y, dimensions.z},
                {0.f, dimensions.y, 0.f},

                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z },
                {dimensions.x, dimensions.y, dimensions.z},
                {0.f, dimensions.y, dimensions.z},

                {dimensions.x, 0.f, 0.f},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {dimensions.x, 0.f, 0.f},
                {dimensions.x, dimensions.y, 0.f},
                {0.f, dimensions.y, 0.f},

                {0.f, dimensions.y, 0.f},
                {0.f, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, 0.f, 0.f}
        };

        for(int i = 0; i < 24; ++i)
        {
            // and THEN we use the origin offset to go and offset the vertex positions
            colliderVertices[i].x -= originAsOffsetFromMinCorner.x;
            colliderVertices[i].y -= originAsOffsetFromMinCorner.y;
            colliderVertices[i].z -= originAsOffsetFromMinCorner.z;
        }
#pragma endregion
        return colliderVertices;
    }
};

// - sphere with arbitrary radius
// - curves?
// - pyramids

INTERNAL Mesh testM;

INTERNAL void EditorInitialize()
{
    MeshCreate(testM, nullptr, nullptr, 0, 0);
}

INTERNAL void EditorRender()
{
    EditorCubeObject defaultCube = EditorCubeObject(vec3(4.f, 0.4f, 2.f));
    auto bo = defaultCube.GetBufferObjects();
    testM.RebindBufferObjects(bo.first.data(), bo.second.data(), (u32)bo.first.size(), (u32)bo.second.size());
    testM.RenderMesh();
}

INTERNAL void EditorMousePick()
{
    // get perspective and view matrix from camera
    
    
}
