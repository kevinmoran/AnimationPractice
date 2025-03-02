#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#define D3D11_NO_HELPERS
#include <d3d11_1.h>

#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#undef CGLTF_IMPLEMENTATION

#include "types.h"
#include "3DMaths.h"
#include "D3D11Helpers.h"
#include "Input.h"
#include "ObjLoading.h"
#include "Camera.h"
#include "Player.h"
#include "Collision.h"

#define WINDOW_TITLE L"D3D11"

struct Joint {
    char name[32];
    int parentIndex;
};

struct Skeleton {
    char name[32];
    int numJoints;
    Joint* joints;
    mat4* invBindPoseMats;
};

struct KeyframeData {
    float* translationTimes;
    vec3* translationValues;
    int numTranslationKeys;

    float* rotationTimes;
    vec4* rotationValues;
    int numRotationKeys;

    float* scaleTimes;
    vec3* scaleValues;
    int numScaleKeys;
};

struct Animation {
    char name[64];
    // Skeleton reference? TODO
    float duration;
    KeyframeData* data; // per joint 
};

// Struct to pass data from WndProc to main loop
struct WndProcData {
    bool windowDidResize;
    KeyState keys[KEY_COUNT];
};

void animate(Skeleton skel, Animation anim, float t, mat4** poseMats)
{
    assert(t >= 0);
    assert(t <= anim.duration);
    (*poseMats)[0] = scaleMat(1); // TODO is this a bad idea lol

    for(int i = 0; i <skel.numJoints; ++i)
    {
        KeyframeData* keys = &(anim.data[i]);
        vec3 transLerped;
        {
            int transKeyIndex = 0;
            while(keys->translationTimes[transKeyIndex] > t
                && transKeyIndex < keys->numTranslationKeys)
                ++transKeyIndex;
            
            float timeFrom = keys->translationTimes[transKeyIndex];
            float timeTo = keys->translationTimes[transKeyIndex+1];
            float transT = (t - timeFrom) / (timeTo - timeFrom);

            vec3 transFrom = keys->translationValues[transKeyIndex];
            vec3 transTo = keys->translationValues[transKeyIndex+1];
            transLerped = lerp(transFrom, transTo, transT);
        }
        // todo rotation

        mat4 localPoseMat = translationMat(transLerped);
        // Joint* currJoint = &(skel.joints[i]);
        // Joint* parentJoint = &(skel.joints[currJoint->parentIndex]);
        mat4 currIBPMat = skel.invBindPoseMats[i];

        // if(i==0) TODO we're hoping initialising first poseMat to identity Just Works!
            // parentAnimation // root doesn't have a parent

        (*poseMats)[i] = localPoseMat; // TODO multiply by parent/IBP I think?
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    WndProcData* wndProcData = (WndProcData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch(msg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            KeyState* keys = wndProcData->keys;
            bool isDown = ((lparam & (1 << 31)) == 0);
            // A - Z
            if(wparam >= 'A' && wparam <= 'Z')
                keys[KEY_A + wparam - 'A'].isDown = isDown;
            // 0 - 9
            else if(wparam >= '0' && wparam <= '9')
                keys[KEY_0 + wparam - '0'].isDown = isDown;
            // Numpad 0 - 9
            else if(wparam >= VK_NUMPAD0 && wparam <= VK_NUMPAD9)
                keys[KEY_NUMPAD_0 + wparam - VK_NUMPAD0].isDown = isDown;
            // F keys
            else if(wparam >= VK_F1 && wparam <= VK_F12)
                keys[KEY_F1 + wparam - VK_F1].isDown = isDown;
    
            else if(wparam == VK_BACK)
                keys[KEY_BACKSPACE].isDown = isDown;
            else if(wparam == VK_TAB)
                keys[KEY_TAB].isDown = isDown;
            else if(wparam == VK_RETURN)
                keys[KEY_ENTER].isDown = isDown;
            else if(wparam == VK_SHIFT)
                keys[KEY_SHIFT].isDown = isDown;
            else if(wparam == VK_CONTROL)
                keys[KEY_CTRL].isDown = isDown;
            else if(wparam == VK_MENU)
                keys[KEY_ALT].isDown = isDown;
            // The pause key doesn't work for the simple style of keyboard input,
            // it seems to immediately send a WM_KEYUP message after the WM_KEYDOWN,
            // even if you hold the key, so this flag will just get immediately
            // reset before the game can see it.
            // else if(wparam == VK_PAUSE)
                // keys[KEY_PAUSE].isDown = isDown;
            else if(wparam == VK_CAPITAL)
                keys[KEY_CAPSLOCK].isDown = isDown;
            else if(wparam == VK_ESCAPE)
                keys[KEY_ESC].isDown = isDown;
            else if(wparam == VK_SPACE)
                keys[KEY_SPACE].isDown = isDown;
            else if(wparam == VK_PRIOR)
                keys[KEY_PGUP].isDown = isDown;
            else if(wparam == VK_NEXT)
                keys[KEY_PGDN].isDown = isDown;
            else if(wparam == VK_HOME)
                keys[KEY_HOME].isDown = isDown;
            else if(wparam == VK_END)
                keys[KEY_END].isDown = isDown;
            else if(wparam == VK_LEFT)
                keys[KEY_LEFT].isDown = isDown;
            else if(wparam == VK_UP)
                keys[KEY_UP].isDown = isDown;
            else if(wparam == VK_RIGHT)
                keys[KEY_RIGHT].isDown = isDown;
            else if(wparam == VK_DOWN)
                keys[KEY_DOWN].isDown = isDown;
            // The print screen key seems to only send a WM_KEYUP so this doesn't work. 
            // else if(wparam == VK_SNAPSHOT)
                // keys[KEY_PRINT_SCREEN].isDown = isDown;
            else if(wparam == VK_INSERT)
                keys[KEY_INSERT].isDown = isDown;
            else if(wparam == VK_DELETE)
                keys[KEY_DELETE].isDown = isDown;
            else if(wparam == VK_ADD)
                keys[KEY_NUMPAD_ADD].isDown = isDown;
            else if(wparam == VK_SUBTRACT)
                keys[KEY_NUMPAD_SUBTRACT].isDown = isDown;
            else if(wparam == VK_DECIMAL)
                keys[KEY_NUMPAD_DECIMAL].isDown = isDown;
            else if(wparam == VK_DIVIDE)
                keys[KEY_NUMPAD_DIVIDE].isDown = isDown;
            else if(wparam == VK_OEM_1)
                keys[KEY_SEMICOLON].isDown = isDown;
            else if(wparam == VK_OEM_PLUS)
                keys[KEY_PLUS].isDown = isDown;
            else if(wparam == VK_OEM_COMMA)
                keys[KEY_COMMA].isDown = isDown;
            else if(wparam == VK_OEM_MINUS)
                keys[KEY_MINUS].isDown = isDown;
            else if(wparam == VK_OEM_PERIOD)
                keys[KEY_PERIOD].isDown = isDown;
            else if(wparam == VK_OEM_2)
                keys[KEY_SLASH].isDown = isDown;
            else if(wparam == VK_OEM_3)
                keys[KEY_GRAVE_ACCENT].isDown = isDown;
            else if(wparam == VK_OEM_4)
                keys[KEY_LEFT_BRACKET].isDown = isDown;
            else if(wparam == VK_OEM_5)
                keys[KEY_BACKSLASH].isDown = isDown;
            else if(wparam == VK_OEM_6)
                keys[KEY_RIGHT_BRACKET].isDown = isDown;
            else if(wparam == VK_OEM_7)
                keys[KEY_APOSTROPHE].isDown = isDown;

            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        case WM_SIZE:
        {
            if(wndProcData)
                wndProcData->windowDidResize = true;
            break;
        }
        default:
            result = DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return result;
}

HWND openWindow(LPCWSTR title, int windowWidth, int windowHeight)
{
    WNDCLASSEXW winClass = {};
    winClass.cbSize = sizeof(WNDCLASSEXW);
    winClass.style = CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = &WndProc;
    winClass.hInstance = GetModuleHandleW(NULL);
    winClass.hIcon = LoadIconW(0, IDI_APPLICATION);
    winClass.hCursor = LoadCursorW(0, IDC_ARROW);
    winClass.lpszClassName = title;
    winClass.hIconSm = LoadIconW(0, IDI_APPLICATION);

    if(!RegisterClassExW(&winClass)) {
        MessageBoxA(0, "RegisterClassExW() failed", "Fatal Error", MB_OK);
        return NULL;
    }

    RECT initialRect = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
    LONG initialWidth = initialRect.right - initialRect.left;
    LONG initialHeight = initialRect.bottom - initialRect.top;

    HWND hr = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
                            winClass.lpszClassName,
                            title,
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            initialWidth, 
                            initialHeight,
                            0, 0, winClass.hInstance, 0);

    if(!hr) {
        MessageBoxA(0, "CreateWindowExW() failed", "Fatal Error", MB_OK);
    }
    return hr;
}

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    int windowWidth = 1024;
    int windowHeight = 768;
    float windowAspectRatio = (float)windowWidth / windowHeight;
    HWND hWindow = openWindow(WINDOW_TITLE, windowWidth, windowHeight);
    if(!hWindow) return GetLastError();

    WndProcData wndProcData = {};
    SetWindowLongPtrW(hWindow, GWLP_USERDATA, (LONG)&wndProcData);

    LONGLONG startPerfCount = 0;
    LONGLONG perfCounterFrequency = 0;
    { // Initialise timing data
        LARGE_INTEGER perfCount;
        QueryPerformanceCounter(&perfCount);
        startPerfCount = perfCount.QuadPart;
        LARGE_INTEGER perfFreq;
        QueryPerformanceFrequency(&perfFreq);
        perfCounterFrequency = perfFreq.QuadPart;
    }
    double currentTimeInSeconds = 0.0;
    
    D3D11Data d3d11Data;
    d3d11Init(hWindow, &d3d11Data);
    // TODO: WASAPI init
    // TODO? RawInput init?

    // Create Vertex Shader and Input Layout
    ID3D11VertexShader* vertexShader;
    ID3D11InputLayout* inputLayout;
    {
        // TODO: Parse this from vertex shader code!
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        d3d11CreateVertexShaderAndInputLayout(d3d11Data.device, L"shaders.hlsl", "vs_main", &vertexShader, inputElementDesc, ARRAYSIZE(inputElementDesc), &inputLayout);
    }

    // Create Pixel Shader
    ID3D11PixelShader* pixelShader = d3d11CreatePixelShader(d3d11Data.device, L"shaders.hlsl", "ps_main");

    StaticMeshData cubeObj = loadObj("data/cube.obj");
    StaticMeshData sphereObj = loadObj("data/sphere.obj");
    StaticMeshData cylinderObj = loadObj("data/cylinder.obj");

    Mesh cubeMesh = d3d11CreateMesh(d3d11Data.device, cubeObj);
    Mesh sphereMesh = d3d11CreateMesh(d3d11Data.device, sphereObj);
    Mesh cylinderMesh = d3d11CreateMesh(d3d11Data.device, cylinderObj);
    
    ColliderPolyhedron cubeColliderData = createColliderPolyhedron(cubeObj);

    const char* gltfPath = "data/guy.glb";
    cgltf_options gltfOptions = {};
    cgltf_data* gltfData = NULL;
    cgltf_result gltfParseResult = cgltf_parse_file(&gltfOptions, gltfPath, &gltfData);
    cgltf_result gltfLoadResult = cgltf_load_buffers(&gltfOptions, gltfData,gltfPath);

    AnimatedMeshData gltfObj = {};
    Mesh animatedMesh = {};
    Skeleton skeleton = {};
    Animation* animations = NULL;

    // Load everything from the gltf file
    if (gltfParseResult == cgltf_result_success)
    {
        assert(gltfLoadResult == cgltf_result_success);
        assert(gltfData->meshes_count == 1);
        // Load the mesh data (vertices, indices)
        for(size_t meshIdx=0; meshIdx<gltfData->meshes_count; ++meshIdx)
        {
            cgltf_mesh* mesh = &gltfData->meshes[meshIdx];

            assert(mesh->primitives_count == 1);

            cgltf_primitive* prim = mesh->primitives;
            assert(prim->type == cgltf_primitive_type_triangles);

            { // Find the indices
                cgltf_accessor* indicesAccessor = prim->indices;
                assert(indicesAccessor->component_type == cgltf_component_type_r_16u);
                assert(indicesAccessor->type == cgltf_type_scalar);
                assert(indicesAccessor->offset == 0);
                assert(indicesAccessor->stride == 2);
                assert(indicesAccessor->is_sparse == 0);

                int numIndices = indicesAccessor->count;
                cgltf_buffer_view* bufferView = indicesAccessor->buffer_view;
                assert(bufferView->size == indicesAccessor->count*indicesAccessor->stride);

                cgltf_buffer* buffer = bufferView->buffer;
                uint16_t* indices = (uint16_t*)(((uint8_t*)buffer->data)+bufferView->offset);

                // Copy to StaticMeshData
                gltfObj.numIndices = numIndices;
                gltfObj.indices = (uint16_t*)malloc(numIndices * sizeof(uint16_t));
                CopyMemory(gltfObj.indices, indices, gltfObj.numIndices*sizeof(uint16_t));
            }
            { //Find the vertices
                int numPositions = 0;
                vec3* positions = NULL;
                int numNormals = 0;
                vec3* normals = NULL;
                int numTexCoords = 0;
                vec2* texCoords = NULL;
                int numJoints = 0;
                uint16_t* joints = NULL;
                int numWeights = 0;
                vec4* weights = NULL;
                for(size_t attributeIndex = 0; attributeIndex<prim->attributes_count; ++attributeIndex)
                {
                    cgltf_attribute* a = &prim->attributes[attributeIndex];
                    cgltf_accessor* acc = a->data;
                    assert(a->index == 0);
                    switch(a->type) {
                        case cgltf_attribute_type_position: {
                            assert(acc->component_type == cgltf_component_type_r_32f);
                            assert(acc->type == cgltf_type_vec3);
                            assert(acc->offset == 0);
                            assert(acc->stride == 12);
                            assert(acc->normalized == 0);
                            assert(acc->is_sparse == 0);

                            numPositions = acc->count;

                            cgltf_buffer_view* bufferView = acc->buffer_view;
                            assert(bufferView->size == acc->count*acc->stride);
                            
                            cgltf_buffer* buffer = bufferView->buffer;
                            positions = (vec3*)(((uint8_t*)buffer->data)+bufferView->offset);

                            break;
                        }
                        case cgltf_attribute_type_normal: {
                            assert(acc->component_type == cgltf_component_type_r_32f);
                            assert(acc->type == cgltf_type_vec3);
                            assert(acc->offset == 0);
                            assert(acc->stride == 12);
                            assert(acc->normalized == 0);
                            assert(acc->is_sparse == 0);

                            numNormals = acc->count;

                            cgltf_buffer_view* bufferView = acc->buffer_view;
                            assert(bufferView->size == acc->count*acc->stride);
                            
                            cgltf_buffer* buffer = bufferView->buffer;
                            normals = (vec3*)(((uint8_t*)buffer->data)+bufferView->offset);

                            break;
                        }
                        case cgltf_attribute_type_texcoord: {
                            assert(acc->component_type == cgltf_component_type_r_32f);
                            assert(acc->type == cgltf_type_vec2);
                            assert(acc->offset == 0);
                            assert(acc->stride == 8);
                            assert(acc->normalized == 0);
                            assert(acc->is_sparse == 0);

                            numTexCoords = acc->count;

                            cgltf_buffer_view* bufferView = acc->buffer_view;
                            assert(bufferView->size == acc->count*acc->stride);
                            
                            cgltf_buffer* buffer = bufferView->buffer;
                            texCoords = (vec2*)(((uint8_t*)buffer->data)+bufferView->offset);

                            break;
                        }
                        case cgltf_attribute_type_joints: {
                            assert(acc->component_type == cgltf_component_type_r_16u);
                            assert(acc->type == cgltf_type_vec4);
                            assert(acc->offset == 0);
                            assert(acc->stride == 8);
                            assert(acc->normalized == 0);
                            assert(acc->is_sparse == 0);

                            numJoints = acc->count;

                            cgltf_buffer_view* bufferView = acc->buffer_view;
                            assert(bufferView->size == acc->count*acc->stride);
                            
                            cgltf_buffer* buffer = bufferView->buffer;
                            joints = (uint16_t*)(((uint8_t*)buffer->data)+bufferView->offset);
                            break;
                        }
                        case cgltf_attribute_type_weights: {
                            assert(acc->component_type == cgltf_component_type_r_32f);
                            assert(acc->type == cgltf_type_vec4);
                            assert(acc->offset == 0);
                            assert(acc->stride == 16);
                            assert(acc->normalized == 0);
                            assert(acc->is_sparse == 0);

                            numWeights = acc->count;

                            cgltf_buffer_view* bufferView = acc->buffer_view;
                            assert(bufferView->size == acc->count*acc->stride);
                            
                            cgltf_buffer* buffer = bufferView->buffer;
                            weights = (vec4*)(((uint8_t*)buffer->data)+bufferView->offset);
                            break;
                        }
                        default: {
                            // assert(!(bool)"Unexpected attribute type");
                        }
                    }
                }

                assert(numPositions == numNormals);
                assert(numTexCoords == numNormals);
                // Copy attributes into one interleaved buffer
                gltfObj.numVertices = numPositions;
                gltfObj.vertices = (AnimatedVertexData*)malloc(numPositions * sizeof(AnimatedVertexData));
                for(int vIdx = 0; vIdx < numPositions; ++vIdx)
                {
                    gltfObj.vertices[vIdx] = {positions[vIdx], texCoords[vIdx], normals[vIdx],{},weights[vIdx]}; 
                    gltfObj.vertices[vIdx].boneIds[0] = (uint8_t)joints[4*vIdx+0]; 
                    gltfObj.vertices[vIdx].boneIds[1] = (uint8_t)joints[4*vIdx+1]; 
                    gltfObj.vertices[vIdx].boneIds[2] = (uint8_t)joints[4*vIdx+2]; 
                    gltfObj.vertices[vIdx].boneIds[3] = (uint8_t)joints[4*vIdx+3];
                }
            }

            printf("Found a mesh: %s\n", mesh->name);
        }

        // Load the skeleton
        assert(gltfData->skins_count == 1);
        cgltf_skin* skin = &gltfData->skins[0];
        
        printf("Found a skin: %s\n", skin->name);
        skeleton.numJoints = skin->joints_count;
        skeleton.joints = (Joint*)calloc(skeleton.numJoints, sizeof(Joint));
        strcpy_s(skeleton.name, sizeof(skeleton.name), skin->name);

        skeleton.invBindPoseMats = (mat4*)calloc(skeleton.numJoints, sizeof(mat4));
        { // Find the ibp mats from the gltf sludge
            assert(skin->inverse_bind_matrices->component_type == cgltf_component_type_r_32f);
            assert(skin->inverse_bind_matrices->type == cgltf_type_mat4);
            assert(skin->inverse_bind_matrices->offset == 0);
            assert(skin->inverse_bind_matrices->stride == 64);
            cgltf_buffer_view* ibpMatsBuffView = skin->inverse_bind_matrices->buffer_view;
            cgltf_buffer* ibpMatsBuff = ibpMatsBuffView->buffer;
            mat4* ibpMats = (mat4*)(((uint8_t*)ibpMatsBuff->data)+ibpMatsBuffView->offset);
            memcpy_s(skeleton.invBindPoseMats, skeleton.numJoints*sizeof(mat4), ibpMats, skeleton.numJoints*sizeof(mat4));
        }

        for(size_t i=0; i<skin->joints_count; ++i)
        {
            cgltf_node* node = skin->joints[i];
            Joint* targetJoint = &skeleton.joints[i];
            strcpy_s(targetJoint->name, sizeof(targetJoint->name), node->name);

            for(size_t j=0; j<i; ++j) {
                if(strcmp(skin->joints[j]->name, node->parent->name) == 0)
                    targetJoint->parentIndex = j;
            }

            printf("%u - %s (Parent: %d. %s)\n", i, node->name, targetJoint->parentIndex, node->parent->name);
            // mat4* ibpMats = skeleton.invBindPoseMats;
            // printf("%.2f,%.2f,%.2f,%.2f,\n%.2f,%.2f,%.2f,%.2f,\n%.2f,%.2f,%.2f,%.2f,\n%.2f,%.2f,%.2f,%.2f,\n",
            //         ibpMats[i].m[0][0], ibpMats[i].m[1][0], ibpMats[i].m[2][0], ibpMats[i].m[3][0],
            //         ibpMats[i].m[0][1], ibpMats[i].m[1][1], ibpMats[i].m[2][1], ibpMats[i].m[3][1],
            //         ibpMats[i].m[0][2], ibpMats[i].m[1][2], ibpMats[i].m[2][2], ibpMats[i].m[3][2],
            //         ibpMats[i].m[0][3], ibpMats[i].m[1][3], ibpMats[i].m[2][3], ibpMats[i].m[3][3]);
        }
        fflush(stdout);

        // Find the animations! 
        animations = (Animation*)calloc(gltfData->animations_count, sizeof(Animation));
        printf("Found %u animations\n", gltfData->animations_count);

        for(size_t i=0; i<gltfData->animations_count; ++i)
        {
            animations[i].data = (KeyframeData*)calloc(skeleton.numJoints, sizeof(KeyframeData));
            cgltf_animation* gltfAnimation = &(gltfData->animations[i]);
            strcpy_s(animations[i].name, sizeof(animations[i].name), gltfAnimation->name);
            printf("%s\n", gltfAnimation->name);

            for(size_t j=0; j<gltfAnimation->channels_count; ++j)
            {
                cgltf_animation_channel* channel = &(gltfAnimation->channels[j]);
                int jointIndex = -1;
                for(int k=0; k<skeleton.numJoints; ++k){
                    if(strcmp(skeleton.joints[k].name, channel->target_node->name) == 0) {
                        jointIndex = k; 
                        break;
                    }
                }
                assert(jointIndex >= 0);

                cgltf_animation_sampler* sampler = channel->sampler;

                assert(sampler->interpolation == cgltf_interpolation_type_linear);

                assert(sampler->input->component_type == cgltf_component_type_r_32f);
                assert(sampler->input->normalized == 0);
                assert(sampler->input->type == cgltf_type_scalar);
                assert(sampler->input->offset == 0);
                assert(sampler->input->stride == 4);

                assert(sampler->output->component_type == cgltf_component_type_r_32f);
                assert(sampler->output->normalized == 0);
                assert(sampler->output->offset == 0);
                assert(sampler->output->stride == 4 * (cgltf_size)(sampler->output->type));
                assert(sampler->input->count == sampler->output->count);

                int numKeys = sampler->input->count;
                int numOutKeys = sampler->output->count;

                float* timestamps = (float*)(((uint8_t*)sampler->input->buffer_view->buffer->data) + sampler->input->buffer_view->offset);
                assert(timestamps[0] == 0);
                float duration = timestamps[numKeys-1];
                if(duration > animations[i].duration)
                    animations[i].duration = duration;
                    
                uint8_t* keyValues = ((uint8_t*)sampler->output->buffer_view->buffer->data) + sampler->output->buffer_view->offset;

                KeyframeData* currentKeyFrameData = &animations[i].data[jointIndex];

                int timeKeysSize = numKeys*sizeof(float);

                switch(channel->target_path)
                {
                    case cgltf_animation_path_type_translation: {
                        assert(sampler->output->type == cgltf_type_vec3);
                        vec3* translationValues = (vec3*)keyValues;

                        int valuesSize = numOutKeys*sizeof(vec3);
                        
                        currentKeyFrameData->translationTimes = (float*)malloc(timeKeysSize);
                        currentKeyFrameData->translationValues = (vec3*)malloc(valuesSize);

                        memcpy_s(currentKeyFrameData->translationTimes, timeKeysSize, timestamps, timeKeysSize);
                        memcpy_s(currentKeyFrameData->translationValues, valuesSize, translationValues, valuesSize);
                        currentKeyFrameData->numTranslationKeys = numKeys;
                        break;
                    }
                    case cgltf_animation_path_type_rotation: {
                        assert(sampler->output->type == cgltf_type_vec4);
                        vec4* rotationValues = (vec4*)keyValues;
                        
                        int valuesSize = numOutKeys*sizeof(vec4);
                        
                        currentKeyFrameData->rotationTimes = (float*)malloc(timeKeysSize);
                        currentKeyFrameData->rotationValues = (vec4*)malloc(valuesSize);

                        memcpy_s(currentKeyFrameData->rotationTimes, timeKeysSize, timestamps, timeKeysSize);
                        memcpy_s(currentKeyFrameData->rotationValues, valuesSize, rotationValues, valuesSize);
                        currentKeyFrameData->numRotationKeys = numKeys;
                        break;
                    }
                    case cgltf_animation_path_type_scale: {
                        assert(sampler->output->type == cgltf_type_vec3);
                        break;
                    }
                    default: assert(0);
                }

            }
        }
        animatedMesh = d3d11CreateAnimatedMesh(d3d11Data.device, gltfObj);

        cgltf_free(gltfData);
    }

    freeAnimatedMesh(gltfObj);
    freeStaticMesh(cubeObj);
    freeStaticMesh(sphereObj);
    freeStaticMesh(cylinderObj);

    Texture cubeTexture = d3d11CreateTexture(d3d11Data.device, d3d11Data.deviceContext, "data/test.png");

    Texture whiteTexture;
    {
        unsigned char pixel[4] = {255,255,255,255};
        whiteTexture = d3d11CreateTexture(d3d11Data.device, d3d11Data.deviceContext, 1, 1, 4, pixel);
    }

    // Create Sampler State
    ID3D11SamplerState* samplerState;
    {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;

        d3d11Data.device->CreateSamplerState(&samplerDesc, &samplerState);
    }
    
    struct PerObjectVSConstants
    {
        mat4 modelViewProj;
    };
    struct PerObjectPSConstants
    {
        vec4 tintColour;
    };

    ID3D11Buffer* perObjectVSConstantBuffer = d3d11CreateConstantBuffer(d3d11Data.device, sizeof(PerObjectVSConstants));
    ID3D11Buffer* perObjectPSConstantBuffer = d3d11CreateConstantBuffer(d3d11Data.device, sizeof(PerObjectPSConstants));

    ID3D11RasterizerState* rasterizerState;
    {
        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_BACK;
        rasterizerDesc.FrontCounterClockwise = TRUE;

        d3d11Data.device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
    }

    ID3D11DepthStencilState* depthStencilState;
    {
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable    = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS;

        d3d11Data.device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
    }

    // Camera
    Camera camera = cameraInit({0, 1, 3},  {0, 0, 0});

    bool freeCam = false;

    mat4 perspectiveMat = {};
    wndProcData.windowDidResize = true; // To force initial perspectiveMat calculation

    Player player = playerInit({0,0,0}, normalise({0,0,1}));

    const int NUM_CUBES = 5;
    vec3 cubePositions[NUM_CUBES] = {
        {4,0,0},
        {-1,2,-5},
        {3,1,-8},
        {-2,0.2,7},
        {0,-1,0},
    };
    vec3 cubeScales[NUM_CUBES] = {
        {3,3,3},
        {1,2,5},
        {3,1,8},
        {5,2,4},
        {20,1,20},
    };

    mat4 cubeModelMats[NUM_CUBES];
    ColliderPolyhedron cubeColliderDatas[NUM_CUBES];
    for(int i=0; i<NUM_CUBES; ++i) {
        cubeModelMats[i] = scaleMat(cubeScales[i]) * translationMat(cubePositions[i]);
        mat3 invModelMat = scaleMat3(1/cubeScales[i]);
        cubeColliderDatas[i] = cubeColliderData;
        cubeColliderDatas[i].modelMatrix = cubeModelMats[i];
        cubeColliderDatas[i].normalMatrix = transpose(invModelMat);
    }

    const int NUM_GLTFS = 3;
    vec3 gltfPositions[NUM_GLTFS] = {
        {10,0,-20},
        {-10,0,-20},
        {3,0,-20}
    };
    mat4 gltfModelMats[NUM_GLTFS];
    for(int i=0; i<NUM_GLTFS; ++i) {
        gltfModelMats[i] = scaleMat(2) * translationMat(gltfPositions[i]);
    }

    const int NUM_SPHERES = 4;
    vec3 spherePositions[NUM_SPHERES] = {
        {-4,0,0},
        {4,2,5},
        {-6,1,-6},
        {6,1,-6}
    };
    float sphereScales[NUM_SPHERES] = {
        1,
        1,
        2,
        3
    };

    mat4 sphereModelMats[NUM_SPHERES];
    ColliderSphere sphereColliders[NUM_SPHERES];
    for(int i=0; i<NUM_SPHERES; ++i) {
        sphereModelMats[i] = scaleMat(sphereScales[i]) * translationMat(spherePositions[i]);
        sphereColliders[i] = { spherePositions[i], sphereScales[i] };
    }
        
    float timeStepMultiplier = 1.f;

    // Simple skeleton arm test
    const int NUM_SKEL_BONES = 3;
    mat4 skelBindPoseMats[NUM_SKEL_BONES] = {
        translationMat({0,0,0}),
        translationMat({0,1,0}), // Note, these are relative to parent
        translationMat({0,1,0}) // so this bone is actually 2 units above root
    };
    mat4 skelInvBindPoseMats[NUM_SKEL_BONES] =
    {
        // note: because we're using a cube at the origin as the model 
        // for each bone, they're effectively already in local space
        // so invBindPose is identity for now
        scaleMat(1), scaleMat(1), scaleMat(1)
    };
    vec3 skelTranslations[NUM_SKEL_BONES] = 
    {
        {-3,0,-3}
    };
    vec2 skelRotations[NUM_SKEL_BONES] =
    {
        // {0, 0}
        {-PI32/6, PI32/4},
    };

    // Main Loop
    bool isRunning = true;
    while(isRunning)
    {
        float dt;
        {
            double previousTimeInSeconds = currentTimeInSeconds;
            LARGE_INTEGER perfCount;
            QueryPerformanceCounter(&perfCount);

            currentTimeInSeconds = (double)(perfCount.QuadPart - startPerfCount) / (double)perfCounterFrequency;
            dt = CLAMP_BELOW((float)(currentTimeInSeconds - previousTimeInSeconds), 1.f/60.f);
        }

        keysUpdateWasDownState(wndProcData.keys, KEY_COUNT);

        { // Process Windows message queue
            MSG msg = {};
            while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
            {
                if(msg.message == WM_QUIT)
                    isRunning = false;
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if(wndProcData.keys[KEY_ESC].isDown)
            break;

        if(wndProcData.windowDidResize)
        {
            { // Get window dimensions
                RECT clientRect;
                GetClientRect(hWindow, &clientRect);
                windowWidth = clientRect.right - clientRect.left;
                windowHeight = clientRect.bottom - clientRect.top;
                windowAspectRatio = (float)windowWidth / (float)windowHeight;
            }

            d3d11Data.deviceContext->OMSetRenderTargets(0, 0, 0);
            d3d11Data.mainRenderTarget->Release();
            d3d11Data.mainRenderTargetView->Release();
            d3d11Data.msaaRenderTarget->Release();
            d3d11Data.msaaRenderTargetView->Release();
            d3d11Data.depthStencilView->Release();

            HRESULT hr = d3d11Data.swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            if(FAILED(hr)) {
                assert(!(bool)"ResizeBuffers failed after window resize");
                return -1;
            }
            
            d3d11InitRenderTargetsAndDepthBuffer(&d3d11Data);
            perspectiveMat = makePerspectiveMat(windowAspectRatio, degreesToRadians(100), 0.1f, 1000.f);

            wndProcData.windowDidResize = false;
        }

        if(wndProcData.keys[KEY_TAB].wentDown())
            freeCam = !freeCam;
        if(wndProcData.keys[KEY_R].wentDown()) 
            player = playerInit({}, {0,0,1});
        if(wndProcData.keys[KEY_MINUS].wentDown())
            timeStepMultiplier = CLAMP_ABOVE(timeStepMultiplier*0.5f, 0.25f);
        if(wndProcData.keys[KEY_PLUS].wentDown())
            timeStepMultiplier = CLAMP_BELOW(timeStepMultiplier*2.f, 2.f);
        }

        if(!freeCam) {
            playerUpdate(&player, wndProcData.keys, camera.fwd, dt*timeStepMultiplier);
        }
        
        const float playerRadius = 0.3f;
        const float playerHeight = 1.f;
        assert(playerHeight > 2.f*playerRadius);
        const float playerCapsuleRadius = playerRadius;
        const float playerCapsuleLineSegmentLength = playerHeight - 2.f*playerRadius;
        ColliderCapsule playerColliderData = {
            player.pos + vec3{0, playerCapsuleRadius, 0},
            player.pos + vec3{0, playerCapsuleRadius + playerCapsuleLineSegmentLength, 0},
            playerCapsuleRadius
        };

        vec4 cubeTintColours[NUM_CUBES] = {};
        vec4 sphereTintColours[NUM_SPHERES] = {};
        // Collision Detection
        for(u32 i=0; i<NUM_CUBES; ++i)
        {
            CollisionResult result = checkCollision(playerColliderData, cubeColliderDatas[i]);
            if(result.isColliding) {
                cubeTintColours[i] = {0.1f, 0.8f, 0.2f, 1.f};
                player.pos += result.normal * result.penetrationDistance;
                if (fabsf(dot(result.normal, vec3{0,1,0})) < 0.1f)
                {
                    player.vel.y = 0;
                    player.isOnGround = true;
                }
            }
            else {
                cubeTintColours[i] = {1,1,1,1};
            }
        }
        for(u32 i=0; i<NUM_SPHERES; ++i)
        {
            CollisionResult result = checkCollision(playerColliderData, sphereColliders[i]);
            if(result.isColliding) {
                sphereTintColours[i] = {0.1f, 0.8f, 0.2f, 1.f};
                player.pos += result.normal * result.penetrationDistance;
            }
            else {
                sphereTintColours[i] = {1,1,0,1};
            }
        }

        mat4 viewMat;
        if(freeCam) {
            viewMat = cameraUpdateFreeCam(&camera, wndProcData.keys, dt*timeStepMultiplier);
        }
        else {
            viewMat = cameraUpdateFollowPlayer(&camera, player.pos);
        }

        mat4 viewPerspectiveMat = viewMat * perspectiveMat;

        { // set up draw state guff
            FLOAT backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
            d3d11Data.deviceContext->ClearRenderTargetView(d3d11Data.msaaRenderTargetView, backgroundColor);
            
            d3d11Data.deviceContext->ClearDepthStencilView(d3d11Data.depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

            D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)windowWidth, (FLOAT)windowHeight, 0.0f, 1.0f };
            d3d11Data.deviceContext->RSSetViewports(1, &viewport);

            d3d11Data.deviceContext->RSSetState(rasterizerState);
            d3d11Data.deviceContext->OMSetDepthStencilState(depthStencilState, 0);

            d3d11Data.deviceContext->OMSetRenderTargets(1, &d3d11Data.msaaRenderTargetView, d3d11Data.depthStencilView);

            d3d11Data.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            d3d11Data.deviceContext->IASetInputLayout(inputLayout);

            d3d11Data.deviceContext->VSSetShader(vertexShader, nullptr, 0);
            d3d11Data.deviceContext->PSSetShader(pixelShader, nullptr, 0);

            d3d11Data.deviceContext->VSSetConstantBuffers(0, 1, &perObjectVSConstantBuffer);
            d3d11Data.deviceContext->PSSetConstantBuffers(0, 1, &perObjectPSConstantBuffer);

            d3d11Data.deviceContext->PSSetShaderResources(0, 1, &cubeTexture.d3dShaderResourceView);
            d3d11Data.deviceContext->PSSetSamplers(0, 1, &samplerState);
        }

        { // Draw player capsule collider (just draw 2 spheres and a cylinder between them)

            PerObjectPSConstants psConstants = { {0.8f, 0.1f, 0.3f, 1.0f} };
            d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectPSConstantBuffer, &psConstants, sizeof(PerObjectPSConstants));
            
            { // Draw cylinder
                d3d11Data.deviceContext->IASetVertexBuffers(0, 1, &cylinderMesh.vertexBuffer, &cylinderMesh.stride, &cylinderMesh.offset);
                d3d11Data.deviceContext->IASetIndexBuffer(cylinderMesh.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
                
                mat4 modelMat = scaleMat({playerCapsuleRadius,playerCapsuleLineSegmentLength,playerCapsuleRadius})
                * translationMat(player.pos + vec3{0,playerCapsuleRadius,0});
                PerObjectVSConstants vsConstants = { modelMat * viewPerspectiveMat };
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectVSConstantBuffer, &vsConstants, sizeof(PerObjectVSConstants));

                d3d11Data.deviceContext->DrawIndexed(cylinderMesh.numIndices, 0, 0);
            }
            
            d3d11Data.deviceContext->IASetVertexBuffers(0, 1, &sphereMesh.vertexBuffer, &sphereMesh.stride, &sphereMesh.offset);
            d3d11Data.deviceContext->IASetIndexBuffer(sphereMesh.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
            
            { // Draw sphere 1
                mat4 modelMat = scaleMat(playerCapsuleRadius)
                * translationMat(player.pos + vec3{0,playerCapsuleRadius,0});
                PerObjectVSConstants vsConstants = { modelMat * viewPerspectiveMat };
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectVSConstantBuffer, &vsConstants, sizeof(PerObjectVSConstants));

                d3d11Data.deviceContext->DrawIndexed(sphereMesh.numIndices, 0, 0);
            }
            
            { // Draw sphere 2
                mat4 modelMat = scaleMat(playerCapsuleRadius)
                * translationMat(player.pos + vec3{0,playerCapsuleRadius+playerCapsuleLineSegmentLength,0});
                PerObjectVSConstants vsConstants = { modelMat * viewPerspectiveMat };
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectVSConstantBuffer, &vsConstants, sizeof(PerObjectVSConstants));

                d3d11Data.deviceContext->DrawIndexed(sphereMesh.numIndices, 0, 0);
            }
        }

        { // Draw cubes
            d3d11Data.deviceContext->IASetVertexBuffers(0, 1, &cubeMesh.vertexBuffer, &cubeMesh.stride, &cubeMesh.offset);
            d3d11Data.deviceContext->IASetIndexBuffer(cubeMesh.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
            
            for(int i=0; i<NUM_CUBES; ++i) {
                PerObjectVSConstants vsConstants = { cubeModelMats[i] * viewPerspectiveMat};
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectVSConstantBuffer, &vsConstants, sizeof(PerObjectVSConstants));
            
                PerObjectPSConstants psConstants = { cubeTintColours[i] };
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectPSConstantBuffer, &psConstants, sizeof(PerObjectPSConstants));

                d3d11Data.deviceContext->DrawIndexed(cubeMesh.numIndices, 0, 0);
            }
        }

        { // Draw test skeleton
            d3d11Data.deviceContext->IASetVertexBuffers(0, 1, &cubeMesh.vertexBuffer, &cubeMesh.stride, &cubeMesh.offset);
            d3d11Data.deviceContext->IASetIndexBuffer(cubeMesh.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
            
            mat4 skelModelMats[NUM_SKEL_BONES] = 
            {
                scaleMat(1) // init root bone so when we grab parent matrix it's valid
            };

            skelRotations[0].y = sinf(0.5f*PI32*(float)currentTimeInSeconds);
            skelRotations[1].x = sinf(0.3f*PI32*(float)currentTimeInSeconds);
            skelRotations[2].y = sinf(0.8f*PI32*(float)currentTimeInSeconds);

            for(int i=0; i<NUM_SKEL_BONES; ++i)
            {
                int parentIndex = CLAMP_ABOVE(i-1,0);
                mat4 parentMat = skelModelMats[parentIndex];
                
                mat4 localMat = rotateXMat(skelRotations[i].x) * rotateYMat(skelRotations[i].y) * translationMat(skelTranslations[i]);

                skelModelMats[i] = skelInvBindPoseMats[i] * localMat * skelBindPoseMats[i] * parentMat;

                PerObjectVSConstants vsConstants = { skelModelMats[i] * viewPerspectiveMat};
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectVSConstantBuffer, &vsConstants, sizeof(PerObjectVSConstants));
            
                PerObjectPSConstants psConstants = { 0.8,0,0.8,1 };
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectPSConstantBuffer, &psConstants, sizeof(PerObjectPSConstants));

                d3d11Data.deviceContext->DrawIndexed(cubeMesh.numIndices, 0, 0);
            }
        }
    
#if 01
        { // Draw animated mesh

            static float animationTimer = 0;
            // animationTimer += dt; // todo timer looping

            mat4* poseMats = (mat4*)malloc(skeleton.numJoints * sizeof(mat4));
            animate(skeleton, animations[0], animationTimer, &poseMats);

            // todo send posemats to gpu somehow
            // todo write skinning shaderc
            // donezo

            d3d11Data.deviceContext->IASetVertexBuffers(0, 1, &animatedMesh.vertexBuffer, &animatedMesh.stride, &animatedMesh.offset);
            d3d11Data.deviceContext->IASetIndexBuffer(animatedMesh.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
            
            for(int i=0; i<NUM_GLTFS; ++i) {
                PerObjectVSConstants vsConstants = { gltfModelMats[i] * viewPerspectiveMat};
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectVSConstantBuffer, &vsConstants, sizeof(PerObjectVSConstants));
            
                PerObjectPSConstants psConstants = { {1,0,0,1} };
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectPSConstantBuffer, &psConstants, sizeof(PerObjectPSConstants));

                d3d11Data.deviceContext->DrawIndexed(animatedMesh.numIndices, 0, 0);
            }
        }

        { // Draw spheres
            d3d11Data.deviceContext->IASetVertexBuffers(0, 1, &sphereMesh.vertexBuffer, &sphereMesh.stride, &sphereMesh.offset);
            d3d11Data.deviceContext->IASetIndexBuffer(sphereMesh.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
            
            d3d11Data.deviceContext->PSSetShaderResources(0, 1, &whiteTexture.d3dShaderResourceView);
            
            for(int i=0; i<NUM_SPHERES; ++i) {
                PerObjectVSConstants vsConstants = { sphereModelMats[i] * viewPerspectiveMat};
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectVSConstantBuffer, &vsConstants, sizeof(PerObjectVSConstants));
            
                PerObjectPSConstants psConstants = { sphereTintColours[i] };
                d3d11OverwriteConstantBuffer(d3d11Data.deviceContext, perObjectPSConstantBuffer, &psConstants, sizeof(PerObjectPSConstants));

                d3d11Data.deviceContext->DrawIndexed(sphereMesh.numIndices, 0, 0);
            }
        }
#endif
        d3d11Data.deviceContext->ResolveSubresource(d3d11Data.mainRenderTarget, 0, d3d11Data.msaaRenderTarget, 0, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);

        d3d11Data.swapChain->Present(1, 0);
    }

    { // Release all d3d11 stuff
        depthStencilState->Release();
        rasterizerState->Release();
        perObjectPSConstantBuffer->Release();
        perObjectVSConstantBuffer->Release();
        whiteTexture.d3dShaderResourceView->Release();
        cubeTexture.d3dShaderResourceView->Release();
        samplerState->Release();
        cylinderMesh.indexBuffer->Release();
        cylinderMesh.vertexBuffer->Release();
        sphereMesh.indexBuffer->Release();
        sphereMesh.vertexBuffer->Release();
        cubeMesh.indexBuffer->Release();
        cubeMesh.vertexBuffer->Release();
        pixelShader->Release();
        inputLayout->Release();
        vertexShader->Release();
        d3d11Data.depthStencilView->Release();
        d3d11Data.msaaRenderTarget->Release();
        d3d11Data.msaaRenderTargetView->Release();
        d3d11Data.mainRenderTarget->Release();
        d3d11Data.mainRenderTargetView->Release();
        d3d11Data.swapChain->Release();
        d3d11Data.deviceContext->Release();
        d3d11Data.device->Release();
    }

    return 0;
}
