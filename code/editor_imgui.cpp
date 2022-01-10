INTERNAL std::string OpenFilePrompt(u32 fileTypesCount, COMDLG_FILTERSPEC fileTypes[])
{
    std::string filePathString;
    IFileOpenDialog *pFileOpen;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    if (SUCCEEDED(hr))
    {
        hr = pFileOpen->SetFileTypes(fileTypesCount, fileTypes);

        // Show the Open dialog box.
        hr = pFileOpen->Show(NULL);
        // Get the file name from the dialog box.
        if (SUCCEEDED(hr))
        {
            IShellItem *pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr))
                {
                    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
                    filePathString = converterX.to_bytes(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    CoUninitialize();
    return filePathString;
}

INTERNAL std::string OpenFilePromptGetRelativePath(u32 fileTypesCount, COMDLG_FILTERSPEC fileTypes[])
{
    std::string path = OpenFilePrompt(fileTypesCount, fileTypes);
    char cwd[PATH_MAX];
    if (!path.empty() && _getcwd(cwd, sizeof(cwd)) != NULL)
    {
        return path.substr(std::string(cwd).length()+1);
    }
    return "";
}

INTERNAL std::string SaveFilePrompt()
{
    std::string filePathString;
    IFileSaveDialog* pFileSave;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                                  IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
    if (SUCCEEDED(hr))
    {
        // NOTE(Kevin): This is fine. I'm never going to save a non .map file.
        COMDLG_FILTERSPEC fileTypes[] =
        {
            { L"Map files", L"*.map" },
        };
        hr = pFileSave->SetFileTypes(1, fileTypes);
        pFileSave->SetDefaultExtension(L"map");

        // Show the Open dialog box.
        hr = pFileSave->Show(NULL);
        // Get the file name from the dialog box.
        if (SUCCEEDED(hr))
        {
            IShellItem *pItem;
            hr = pFileSave->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr))
                {
                    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
                    filePathString = converterX.to_bytes(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileSave->Release();
    }
    CoUninitialize();
    return filePathString;
}


#pragma region EditorImGuiImGuiStuff

INTERNAL void UpdateEditorImGui()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New map"))
            {
                EditorSwitchMap("");
            }
            if (ImGui::MenuItem("Open..", "Ctrl+O"))
            {
                COMDLG_FILTERSPEC fileTypes[] =
                {
                    { L"Map files", L"*.map" },
                };
                const std::string& mapOpenPath = OpenFilePrompt(ARRAYSIZE(fileTypes), fileTypes);
                if(!mapOpenPath.empty()) EditorSwitchMap(mapOpenPath);
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                const std::string& mapSavePath = SaveFilePrompt();
                if(!mapSavePath.empty()) EditorExportMap(mapSavePath);
            }
            if (ImGui::MenuItem("Shut down"))  { gIsRunning = false; }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Help"))
        {
            if(ImGui::MenuItem("Help Topics"))
            {

            }

            if(ImGui::MenuItem("About"))
            {

            }

            ImGui::EndMenu();
        }

        ImGui::Spacing();

        bool _bTextureLock = false; // TODO(Kevin): Implement texture lock
        ImGui::Checkbox("Texture lock", &_bTextureLock);

        ImGui::Spacing();

        // TODO(Kevin): Implement snap to grid based on selected dropdown item
        const char* gridDropdownItems[] = { "Grid 1", "Grid 2", "Grid 4", "Grid 8", "Grid 16", "Grid 32", "Grid 64", "Grid 128", "Grid 256" };
        LOCAL_PERSIST const char* currentGridDropDownItem = NULL;
        ImGui::PushItemWidth(100);
        if (ImGui::BeginCombo("##custom combo", currentGridDropDownItem))
        {
            for (int n = 0; n < IM_ARRAYSIZE(gridDropdownItems); ++n)
            {
                bool _gridDropDownItemIsSelected = (currentGridDropDownItem == gridDropdownItems[n]);
                if (ImGui::Selectable(gridDropdownItems[n], _gridDropDownItemIsSelected))
                    currentGridDropDownItem = gridDropdownItems[n];
                if (_gridDropDownItemIsSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::EndMainMenuBar();
    }

    ImGuiViewport* _mainImGuiViewport = (ImGuiViewport*)(void*)ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(_mainImGuiViewport->WorkPos.x+_mainImGuiViewport->WorkSize.x-300, _mainImGuiViewport->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(300, _mainImGuiViewport->WorkSize.y));
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextUnformatted(loadedMapPath.c_str());
    ImGui::Separator();

    ImGui::BeginGroup();
    if(ImGui::Button("Select"))
    {
        editorMode = EDITOR_MODE_SELECT;
    }
    ImGui::SameLine();
    if(ImGui::Button("Create"))
    {
        editorMode = EDITOR_MODE_CREATE;
    }
    ImGui::SameLine();
    if(ImGui::Button("Edit"))
    {
        editorMode = EDITOR_MODE_EDIT;
    }
    ImGui::EndGroup();
    ImGui::Separator();

    GameObject* selectedEditObj = GetEditorObjectWithId(selectedEditObjId);
    ImGui::Text("Selected object ID: %d", selectedEditObjId);

    switch(editorMode)
    {
        case EDITOR_MODE_SELECT:{
            if(selectedEditObj)
            {
                vec3 selectedPos = selectedEditObj->GetPos();
                ImGui::DragFloat3("Pos", &selectedPos.x,0.32f);
                selectedEditObj->SetPos(selectedPos);
                vec3 selectedRot = QuatToEuler(selectedEditObj->GetOrientation()) * SML_RAD2DEG;
                ImGui::DragFloat3("Rot", &selectedRot.x,0.2f);
                selectedEditObj->SetOrientation(EulerToQuat(selectedRot * SML_DEG2RAD));
                vec3 selectedScale = selectedEditObj->GetScale();
                ImGui::DragFloat3("Scale", &selectedScale.x, 0.01f);
                selectedEditObj->SetScale(selectedScale);
                ImGui::Separator();

                /* Class specific IMGUI */
                switch(selectedEditObj->GetGameClassCode())
                {
                    case CLASS_PROPSTATIC:{
                        PropStatic* ps = (PropStatic*) selectedEditObj->GetClass();
                        if(ImGui::Button("Load Model"))
                        {
                            COMDLG_FILTERSPEC fileTypes[] =
                            {
                                { L"Supported 3d formats", L"*.obj;*.fbx;*.3ds;*.gltf" },
                            };
                            std::string modelPath = OpenFilePromptGetRelativePath(ARRAYSIZE(fileTypes), fileTypes);
                            if(!modelPath.empty())
                            {
                                auto _loadedMeshGroupPtr = MeshGroupAssimpLoad(modelPath.c_str());
                                if(_loadedMeshGroupPtr)
                                {
                                    printf("PropStatic loaded model: %s\n", modelPath.c_str());
                                    ps->SetModel(_loadedMeshGroupPtr);
                                    // TODO(Kevin): We need to unload the old model or something
                                }
                            }
                        }
                    }break;
                    case BRUSH_VOLUME:{
                        VolumeBrush* lgb = (VolumeBrush*) selectedEditObj->GetClass();
                        // what should we do here?
                    }break;
                    default:{
                        // use code reflection to list class properties
                    }break;
                }

                if(ImGui::Button("Go to Selected"))
                {
                    MoveCameraToSelectedEditorObject();
                }
            }
        }break;

        case EDITOR_MODE_CREATE:{
            enum EditorObjectCreateType {
                EDITOR_CREATE_TYPE_BRUSH,
                EDITOR_CREATE_TYPE_ENTITY
            };
            LOCAL_PERSIST EditorObjectCreateType editorObjectCreateType = EDITOR_CREATE_TYPE_BRUSH;
            ImGui::Text("Select editor object type");
            if(ImGui::Button("Brush"))
            {
                editorObjectCreateType = EDITOR_CREATE_TYPE_BRUSH;
            }
            ImGui::SameLine();
            if(ImGui::Button("Entity"))
            {
                editorObjectCreateType = EDITOR_CREATE_TYPE_ENTITY;
            }
            switch(editorObjectCreateType)
            {
                case EDITOR_CREATE_TYPE_BRUSH:{
                    ImGui::Text("Brush creation mode");

                    LOCAL_PERSIST vec3 createPosition;
                    LOCAL_PERSIST quat createOrientation;
                    LOCAL_PERSIST vec3 createScale = vec3(1.f, 1.f, 1.f);

                    // TODO(Kevin): Add dropdown to choose brush type

                    ImGui::DragFloat3("Pos", &createPosition.x, 0.32f);
                    vec3 selectedRot = QuatToEuler(createOrientation) * SML_RAD2DEG;
                    ImGui::DragFloat3("Rot", &selectedRot.x, 0.2f);
                    createOrientation = EulerToQuat(selectedRot * SML_DEG2RAD);
                    ImGui::DragFloat3("Scale", &createScale.x, 0.01f);

                    if(ImGui::Button("Create Brush"))
                    {
                        GameObject* newEntity = &editorEntities.PushBackDefault();
                        newEntity->SetPos(createPosition);
                        newEntity->SetOrientation(createOrientation);
                        newEntity->SetScale(createScale);

                        auto createdlevelgeombrush = new VolumeBrush();
                        createdlevelgeombrush->MakeVolumeMeshOnGPU();
                        newEntity->SetClass(createdlevelgeombrush);

                        SetSelectedEditObjId(newEntity->GetId());
                    }
                }break;
                case EDITOR_CREATE_TYPE_ENTITY:{
                    ImGui::Text("Entity creation mode");

                    LOCAL_PERSIST vec3 createPosition;
                    LOCAL_PERSIST quat createOrientation;
                    LOCAL_PERSIST vec3 createScale = vec3(1.f, 1.f, 1.f);

                    ImGui::DragFloat3("Pos", &createPosition.x, 0.01f);
                    vec3 selectedRot = QuatToEuler(createOrientation) * SML_RAD2DEG;
                    ImGui::DragFloat3("Rot", &selectedRot.x, 0.2f);
                    createOrientation = EulerToQuat(selectedRot * SML_DEG2RAD);
                    ImGui::DragFloat3("Scale", &createScale.x, 0.01f);

                    if(ImGui::Button("Create Entity"))
                    {
                        GameObject* newEntity = &editorEntities.PushBackDefault();
                        newEntity->SetPos(createPosition);
                        newEntity->SetOrientation(createOrientation);
                        newEntity->SetScale(createScale);

                        PropStatic* propStatic = new PropStatic();
                        newEntity->SetClass(propStatic);
                        propStatic->SetModel(defaultEntityModel);

                        SetSelectedEditObjId(newEntity->GetId());
                    }
                }break;
            }
        }break;

        case EDITOR_MODE_EDIT:{
            ImGui::Text("Hold left CTRL to select multiple vertices or faces.");
            ImGui::Spacing();
            ImGui::Separator();
            if(selectedEditObj)
            {
                switch(selectedEditObj->GetGameClassCode())
                {
                    case BRUSH_VOLUME:{
                        VolumeBrush* lgb = (VolumeBrush*) selectedEditObj->GetClass();

                        ImGui::TextUnformatted("Vertices");
                        for(int i = 0; i < selectedClickableVertices.size(); ++i)
                        {
                            mat4 _selectedEditObjModelMatrix = selectedEditObj->GetModelMatrix();
                            vec3 v = selectedClickableVertices[i]->pos;
                            v = (_selectedEditObjModelMatrix * vec4(v, 1.f)).xyz;
                            ImGui::PushID(i);
                            ImGui::DragFloat3("v", &v.x, 0.32f);
                            if(ImGui::IsItemEdited())
                            {
                                selectedClickableVertices[i]->pos = (_selectedEditObjModelMatrix.GetInverse() * vec4(v, 1.f)).xyz;
                            }
                            ImGui::PopID();

                        }
                        ImGui::Separator();

                        ImGui::TextUnformatted("Faces");
                        ImGui::Text("%zd faces selected.", selectedClickableFaces.size());
                        if(!selectedClickableFaces.empty())
                        {
                            bool sameFilePaths = true;
                            bool sameTextureShift = true;
                            bool sameTextureScale = true;
                            bool sameTextureRotation = true;
                            for(auto facePtr : selectedClickableFaces)
                            {
                                sameFilePaths &= selectedClickableFaces[0]->texture.filePath == facePtr->texture.filePath;
                                sameTextureShift &= selectedClickableFaces[0]->textureShift == facePtr->textureShift;
                                sameTextureScale &= selectedClickableFaces[0]->textureScale == facePtr->textureScale;
                                sameTextureRotation &= selectedClickableFaces[0]->textureRotation == facePtr->textureRotation;
                            }

                            if(!sameFilePaths) { ImGui::BeginDisabled(); }
                            ImGui::Text("Face texture: %s", (sameFilePaths ? selectedClickableFaces[0]->texture.filePath.c_str() : "multiple"));
                            ImGui::Text("%dx%d", selectedClickableFaces[0]->texture.width, selectedClickableFaces[0]->texture.height);
                            if(!sameFilePaths) { ImGui::EndDisabled(); }

                            vec2 _textureShiftImgui = sameTextureShift ? selectedClickableFaces[0]->textureShift : vec2();
                            ImGui::DragFloat2("uv shift", &_textureShiftImgui.x, 0.001f);
                            if(ImGui::IsItemActive())
                            {
                                for(auto facePtr : selectedClickableFaces)
                                {
                                    facePtr->textureShift = _textureShiftImgui;
                                }
                            }
                            vec2 _textureScaleImgui = sameTextureScale ? selectedClickableFaces[0]->textureScale : vec2();
                            ImGui::DragFloat2("uv scale", &_textureScaleImgui.x, 0.001f);
                            if(ImGui::IsItemActive())
                            {
                                for(auto facePtr : selectedClickableFaces)
                                {
                                    facePtr->textureScale = _textureScaleImgui;
                                }
                            }
                            float _textureRotationImgui = sameTextureRotation ? selectedClickableFaces[0]->textureRotation : 0.f;
                            _textureRotationImgui *= SML_RAD2DEG;
                            ImGui::DragFloat("uv rotation", &_textureRotationImgui, 0.1f);
                            if(ImGui::IsItemActive())
                            {
                                _textureRotationImgui *= SML_DEG2RAD;
                                for(auto facePtr : selectedClickableFaces)
                                {
                                    facePtr->textureRotation = _textureRotationImgui;
                                }
                            }

                            LOCAL_PERSIST bool _bSetUVScaleBasedOnTextureSize = true;
                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Checkbox("Set uv scale based on texture size", &_bSetUVScaleBasedOnTextureSize);

                            if(ImGui::Button("Choose texture"))
                            {
                                COMDLG_FILTERSPEC fileTypes[] =
                                {
                                    { L"Supported images", L"*.png;*.jpg;*.jpeg;*.bmp" },
                                };
                                std::string texturePath = OpenFilePromptGetRelativePath(ARRAYSIZE(fileTypes), fileTypes);
                                if(!texturePath.empty())
                                {
                                    Texture _t;
                                    if (TextureCreateFromFile(_t, texturePath.c_str()))
                                    {
                                        printf("Face texture loaded: %s\n", texturePath.c_str());
                                        for (auto facePtr : selectedClickableFaces)
                                        {
                                            facePtr->texture = _t;
                                            if(_bSetUVScaleBasedOnTextureSize)
                                            {
                                                facePtr->textureScale.x = (float)_t.width / (float)TEXTURE_PIXEL_TO_WORLD_UNIT_RATIO;
                                                facePtr->textureScale.y = (float)_t.height / (float)TEXTURE_PIXEL_TO_WORLD_UNIT_RATIO;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            ImGui::TextUnformatted("Select a face to change its texture");
                        }
                    }break;
                    default:{}break;
                }
            }
        }break;
    }

    ImGui::End();
}

#pragma endregion
