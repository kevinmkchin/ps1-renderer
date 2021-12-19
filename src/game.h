struct DefaultGameObject : GameObject
{
    void Update() override
    {

    }

    void OnCollision(CollisionEventArgs collisionInfo) override
    {

    }
};

INTERNAL GameCamera gameCamera;
INTERNAL Shader sceneShader;
INTERNAL std::vector<GameObject*> gameObjectsA;

INTERNAL DefaultGameObject objA;
INTERNAL DefaultGameObject objB;
INTERNAL MeshCollider colA;
INTERNAL SphereCollider colB;
INTERNAL vec3 modelColor;
INTERNAL Texture textureA;
