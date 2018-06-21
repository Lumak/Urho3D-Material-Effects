//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Engine/DebugHud.h>
#include <SDL/SDL_log.h>

#include "Character.h"
#include "CharacterDemo.h"
#include "SplashHandler.h"
#include "UVSequencer.h"
#include "Touch.h"
#include "CollisionLayer.h"

#include <Urho3D/DebugNew.h>
//=============================================================================
//=============================================================================
URHO3D_DEFINE_APPLICATION_MAIN(CharacterDemo)

//=============================================================================
//=============================================================================
CharacterDemo::CharacterDemo(Context* context)
    : Sample(context)
    , firstPerson_(false)
    , drawDebug_(false)
{
    SplashHandler::RegisterObject(context);
    UVSequencer::RegisterObject(context);
    Character::RegisterObject(context);

    // emission
    emissionColor_ = Color::BLACK;
    emissionState_ = EmissionState_R;

    // lightmap
    lightmapPathName_ = "Data/MaterialEffects/Textures/checkers-lightmap";
    lightmapIdx_ = 0;

    // vcol
    vertIdx_ = 0;
    vcolColorIdx_ = 0;
}

CharacterDemo::~CharacterDemo()
{
}

void CharacterDemo::Setup()
{
    engineParameters_["WindowTitle"]   = GetTypeName();
    engineParameters_["LogName"]       = GetSubsystem<FileSystem>()->GetProgramDir() + "graphicsFX.log";
    engineParameters_["FullScreen"]    = false;
    engineParameters_["Headless"]      = false;
    engineParameters_["WindowWidth"]   = 1280; 
    engineParameters_["WindowHeight"]  = 720;
    engineParameters_["ResourcePaths"] = "Data;CoreData;Data/MaterialEffects;";
}

void CharacterDemo::Start()
{
    // Execute base class startup
    Sample::Start();
    if (touchEnabled_)
        touch_ = new Touch(context_, TOUCH_SENSITIVITY);

    ChangeDebugHudText();

    // Create static scene content
    CreateScene();

    InitSplashHandler();

    CreateSequencers();

    CreateWaterRefection();

    // Create the controllable character
    CreateCharacter();

    // Create the UI content
    CreateInstructions();

    // Subscribe to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void CharacterDemo::ChangeDebugHudText()
{
    // change profiler text
    if (GetSubsystem<DebugHud>())
    {
        Text *dbgText = GetSubsystem<DebugHud>()->GetProfilerText();
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);

        dbgText = GetSubsystem<DebugHud>()->GetStatsText();
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);

        dbgText = GetSubsystem<DebugHud>()->GetMemoryText();
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);

        dbgText = GetSubsystem<DebugHud>()->GetModeText();
        dbgText->SetColor(Color::CYAN);
        dbgText->SetTextEffect(TE_NONE);
    }
}

void CharacterDemo::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Renderer* renderer = GetSubsystem<Renderer>();
    Graphics* graphics = GetSubsystem<Graphics>();

    scene_ = new Scene(context_);

    cameraNode_ = new Node(context_);
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));
    renderer->SetViewport(0, viewport);

    // post-process glow
    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/Glow.xml"));

    // set BlurHInvSize to proper value
    // **note** be sure to do this if screen size changes (not done for this demo)
    effectRenderPath->SetShaderParameter("BlurHInvSize", Vector2(1.0f/(float)(graphics->GetWidth()), 1.0f/(float)(graphics->GetHeight())));
    effectRenderPath->SetEnabled("Glow", true);
    viewport->SetRenderPath(effectRenderPath);

    // load scene
    XMLFile *xmlLevel = cache->GetResource<XMLFile>("Data/MaterialEffects/Level1.xml");
    scene_->LoadXML(xmlLevel->GetRoot());
}

void CharacterDemo::InitSplashHandler()
{
    SplashHandler *splashHandler = scene_->CreateComponent<SplashHandler>();
    splashHandler->LoadSplashList("Data/MaterialEffects/SplashData/splashDataList.xml");
}

void CharacterDemo::CreateSequencers()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // uv frame sequencers
    Node *explNode = scene_->GetChild("explosion", true);
    if (explNode)
    {
        UVSequencer *uvSequencer = explNode->CreateComponent<UVSequencer>();
        XMLFile *xmlLevel = cache->GetResource<XMLFile>("Data/MaterialEffects/UVSequencerData/explosionUVFrameSeqData.xml");
        uvSequencer->LoadXML(xmlLevel->GetRoot());
    }

    Node *fireNode = scene_->GetChild("bgfire", true);
    if (fireNode)
    {
        UVSequencer *uvSequencer = fireNode->CreateComponent<UVSequencer>();
        XMLFile *xmlLevel = cache->GetResource<XMLFile>("Data/MaterialEffects/UVSequencerData/bgfireUVFrameSeqData.xml");
        uvSequencer->LoadXML(xmlLevel->GetRoot());
    }

    Node *torchNode = scene_->GetChild("torch", true);
    if (torchNode)
    {
        UVSequencer *uvSequencer = torchNode->CreateComponent<UVSequencer>();
        XMLFile *xmlLevel = cache->GetResource<XMLFile>("Data/MaterialEffects/UVSequencerData/torchUVFrameSeqData.xml");
        uvSequencer->LoadXML(xmlLevel->GetRoot());
    }

    // uv scroll sequencers
    Node *plateUNode = scene_->GetChild("transpPlateU", true);
    if (plateUNode)
    {
        UVSequencer *uvSequencer = plateUNode->CreateComponent<UVSequencer>();
        XMLFile *xmlLevel = cache->GetResource<XMLFile>("Data/MaterialEffects/UVSequencerData/plateUScrollSeqData.xml");
        uvSequencer->LoadXML(xmlLevel->GetRoot());
    }

    Node *plateVNode = scene_->GetChild("transpPlateV", true);
    if (plateVNode)
    {
        UVSequencer *uvSequencer = plateVNode->CreateComponent<UVSequencer>();
        XMLFile *xmlLevel = cache->GetResource<XMLFile>("Data/MaterialEffects/UVSequencerData/plateVScrollSeqData.xml");
        uvSequencer->LoadXML(xmlLevel->GetRoot());
    }

    Node *lavaUNode = scene_->GetChild("lava", true);
    if (lavaUNode)
    {
        UVSequencer *uvSequencer = lavaUNode->CreateComponent<UVSequencer>();
        XMLFile *xmlLevel = cache->GetResource<XMLFile>("Data/MaterialEffects/UVSequencerData/lavaVScrollSeqData.xml");
        uvSequencer->LoadXML(xmlLevel->GetRoot());
    }

}

void CharacterDemo::CreateCharacter()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    Node *spawnNode = scene_->GetChild("playerSpawn");
    Node* objectNode = scene_->CreateChild("Player");
    objectNode->SetPosition(spawnNode->GetPosition());

    // spin node
    Node* adjustNode = objectNode->CreateChild("spinNode");
    adjustNode->SetRotation( Quaternion(180, Vector3(0,1,0) ) );
    
    // Create the rendering component + animation controller
    AnimatedModel* object = adjustNode->CreateComponent<AnimatedModel>();
    object->SetModel(cache->GetResource<Model>("Platforms/Models/BetaLowpoly/Beta.mdl"));
    object->SetMaterial(0, cache->GetResource<Material>("Platforms/Materials/BetaBody_MAT.xml"));
    object->SetMaterial(1, cache->GetResource<Material>("Platforms/Materials/BetaBody_MAT.xml"));
    object->SetMaterial(2, cache->GetResource<Material>("Platforms/Materials/BetaJoints_MAT.xml"));
    object->SetCastShadows(true);
    adjustNode->CreateComponent<AnimationController>();

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    RigidBody* body = objectNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(ColLayer_Character);
    body->SetCollisionMask(ColMask_Character);
    body->SetMass(1.0f);

    body->SetAngularFactor(Vector3::ZERO);
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.94f, 0.0f));

    // character
    character_ = objectNode->CreateComponent<Character>();

    // set rotation
    character_->controls_.yaw_ = -199.7f;
    character_->controls_.pitch_ = 1.19f;

}

void CharacterDemo::CreateWaterRefection()
{
    // right out of 23_Water sample
    Graphics* graphics = GetSubsystem<Graphics>();
    Renderer* renderer = GetSubsystem<Renderer>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    waterNode_ = scene_->GetChild("waterGround", true);
    waterNode_->GetComponent<StaticModel>()->SetViewMask(0x80000000);

    waterPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition());
    waterClipPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition() - Vector3(0.0f, 0.1f, 0.0f));

    reflectionCameraNode_ = cameraNode_->CreateChild();
    Camera* reflectionCamera = reflectionCameraNode_->CreateComponent<Camera>();
    reflectionCamera->SetFarClip(750.0);
    reflectionCamera->SetViewMask(0x7fffffff); // Hide objects with only bit 31 in the viewmask (the water plane)
    reflectionCamera->SetAutoAspectRatio(false);
    reflectionCamera->SetUseReflection(true);
    reflectionCamera->SetReflectionPlane(waterPlane_);
    reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
    reflectionCamera->SetClipPlane(waterClipPlane_);
    reflectionCamera->SetAspectRatio((float)graphics->GetWidth() / (float)graphics->GetHeight());

    int texSize = 1024;
    SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
    renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    renderTexture->SetFilterMode(FILTER_BILINEAR);
    RenderSurface* surface = renderTexture->GetRenderSurface();
    SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, reflectionCamera));
    surface->SetViewport(0, rttViewport);
    Material* waterMat = cache->GetResource<Material>("MaterialEffects/Materials/waterGroundMat.xml");
    waterMat->SetTexture(TU_SPECULAR, renderTexture);

}
void CharacterDemo::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void CharacterDemo::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CharacterDemo, HandleUpdate));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(CharacterDemo, HandlePostUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(CharacterDemo, HandlePostRenderUpdate));
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    Input* input = GetSubsystem<Input>();
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    if (character_)
    {
        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

        // Update controls using touch utility class
        if (touch_)
            touch_->UpdateTouches(character_->controls_);

        // Update controls using keys
        UI* ui = GetSubsystem<UI>();
        if (!ui->GetFocusElement())
        {
            if (!touch_ || !touch_->useGyroscope_)
            {
                character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
                character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
                character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A));
                character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D));
            }
            character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));

            // Add character yaw & pitch from the mouse motion or touch input
            if (touchEnabled_)
            {
                for (unsigned i = 0; i < input->GetNumTouches(); ++i)
                {
                    TouchState* state = input->GetTouch(i);
                    if (!state->touchedElement_)    // Touch on empty space
                    {
                        Camera* camera = cameraNode_->GetComponent<Camera>();
                        if (!camera)
                            return;

                        Graphics* graphics = GetSubsystem<Graphics>();
                        character_->controls_.yaw_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.x_;
                        character_->controls_.pitch_ += TOUCH_SENSITIVITY * camera->GetFov() / graphics->GetHeight() * state->delta_.y_;
                    }
                }
            }
            else
            {
                character_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
                character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
            }
            // Limit pitch
            character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
            // Set rotation already here so that it's updated every rendering frame instead of every physics frame
            character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));

            // Turn on/off gyroscope on mobile platform
            if (touch_ && input->GetKeyPress(KEY_G))
                touch_->useGyroscope_ = !touch_->useGyroscope_;
        }
    }

    // update material effects
    UpdateEmission(timeStep);
    UpdateLightmap(timeStep);
    UpdateVertexColor(timeStep);

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_F4))
        drawDebug_ = !drawDebug_;

    // In case resolution has changed, adjust the reflection camera aspect ratio
    Graphics* graphics = GetSubsystem<Graphics>();
    Camera* reflectionCamera = reflectionCameraNode_->GetComponent<Camera>();
    reflectionCamera->SetAspectRatio((float)graphics->GetWidth() / (float)graphics->GetHeight());

}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character_)
        return;

    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    Node* characterNode = character_->GetNode();
    Quaternion rot = characterNode->GetRotation();
    Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);

    {
        Vector3 aimPoint = characterNode->GetPosition() + rot * Vector3(0.0f, 1.7f, 0.0f);
        Vector3 rayDir = dir * Vector3::BACK;
        float rayDistance = touch_ ? touch_->cameraDistance_ : CAMERA_INITIAL_DIST;
        PhysicsRaycastResult result;
        Vector3 wallNormal;

        scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(aimPoint, rayDir), rayDistance, ColMask_Camera);
        if (result.body_)
        {
            const float minWallDist = 0.0f;
            wallNormal = result.normal_ * 0.12f;

            if (wallNormal.DotProduct(wallHitNormal_) < 0.1f)
            {
                wallHitNormal_ = wallNormal;
            }
            rayDistance = Min(rayDistance, result.distance_ + minWallDist);
        }

        wallHitNormal_ = wallHitNormal_.Lerp(wallNormal, timeStep * 8.0f);
        rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

        cameraNode_->SetPosition(aimPoint + rayDir * rayDistance + wallHitNormal_);
        cameraNode_->SetRotation(dir);
    }
}


void CharacterDemo::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (drawDebug_)
    {
        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
    }
}

void CharacterDemo::UpdateEmission(float timeStep)
{
    Node *emissionNode = scene_->GetChild("emissionSphere1");
    if (emissionNode)
    {
        if (!emissionNode->GetComponent<StaticModel>()->IsInView(cameraNode_->GetComponent<Camera>()))
            return;
    }

    timeStep *= 2.0f;
    switch (emissionState_)
    {
    case EmissionState_R:
        emissionColor_ = emissionColor_.Lerp(Color::RED, timeStep);
        if (emissionColor_.r_ > 0.99f)
        {
            emissionState_ = EmissionState_None1;
        }
        break;

    case EmissionState_G:
        emissionColor_ = emissionColor_.Lerp(Color::GREEN, timeStep);
        if (emissionColor_.g_ > 0.99f)
        {
            emissionState_ = EmissionState_None2;
        }
        break;

    case EmissionState_B:
        emissionColor_ = emissionColor_.Lerp(Color::BLUE, timeStep);
        if (emissionColor_.b_ > 0.99f)
        {
            emissionState_ = EmissionState_None3;
        }
        break;

    case EmissionState_None1:
    case EmissionState_None2:
    case EmissionState_None3:
        emissionColor_ = emissionColor_.Lerp(Color::BLACK, timeStep);
        if (emissionColor_.SumRGB() < 0.01f)
        {
            emissionState_ = ++emissionState_ % EmissionState_MAX;
        }
    }

    if (emissionNode)
    {
        Material *mat = emissionNode->GetComponent<StaticModel>()->GetMaterial();
        mat->SetShaderParameter("MatEmissiveColor", emissionColor_);
    }
}

void CharacterDemo::UpdateLightmap(float timeStep)
{
    if (lightmapTimer_.GetMSec(false) > 1000)
    {
        Node *lightmapNode = scene_->GetChild("lightmapSphere");
        lightmapIdx_ = ++lightmapIdx_ % Max_Lightmaps;

        if (lightmapNode)
        {
            char buf[10];
            sprintf(buf, "%03d.png", lightmapIdx_);
            String diffName = lightmapPathName_ + String(buf);

            ResourceCache* cache = GetSubsystem<ResourceCache>();
            Material *mat = lightmapNode->GetComponent<StaticModel>()->GetMaterial();
            mat->SetTexture(TU_EMISSIVE, cache->GetResource<Texture2D>(diffName));
        }

        lightmapTimer_.Reset();
    }
}

void CharacterDemo::UpdateVertexColor(float timeStep)
{
    const int iNumColors = 13;
    const unsigned uColors[iNumColors] = {
        0xFF00D7FF,  // 0     Gold           = 0xFFFFD700
        0xFF20A5DA,  // 1     Goldenrod      = 0xFFDAA520
        0xFFB9DAFF,  // 2     Peachpuff      = 0xFFFFDAB9
        0xFF008000,  // 3     Green          = 0xFF008000
        0xFF2FFFAD,  // 4     GreenYellow    = 0xFFADFF2F
        0xFFF0FFF0,  // 5     Honeydew       = 0xFFF0FFF0
        0xFFB469FF,  // 6     HotPink        = 0xFFFF69B4
        0xFF5C5CCD,  // 7     IndianRed      = 0xFFCD5C5C
        0xFF82004B,  // 8     Indigo         = 0xFF4B0082
        0xFFD0E040,  // 9     Turquoise      = 0xFF40E0D0
        0xFF8CE6F0,  // 10    Khaki          = 0xFFF0E68C
        0xFF9370DB,  // 11    PaleVioletRed  = 0xFFDB7093
        0xFF1E69D2,  // 12    Chocolate      = 0xFFD2691E
    };

    if (vcolTimer_.GetMSec(false) > 1)
    {
        Node *vcolsphNode = scene_->GetChild("vcolSphere");

        if (vcolsphNode)
        {
            if (!vcolsphNode->GetComponent<StaticModel>()->IsInView(cameraNode_->GetComponent<Camera>()))
                return;

            Model *model = vcolsphNode->GetComponent<StaticModel>()->GetModel();
            Geometry *geometry = model->GetGeometry(0, 0);
            const Vector<SharedPtr<VertexBuffer> > &vbuffers = model->GetVertexBuffers();
            SharedPtr<VertexBuffer> vbuffer = vbuffers[0];
            unsigned elementMask = vbuffer->GetElementMask();

            if (!(elementMask & MASK_COLOR))
            {
                return;
            }

            unsigned vertexSize = vbuffer->GetVertexSize();
            unsigned numVertices = vbuffer->GetVertexCount();
            const unsigned char *vertexData = (const unsigned char*)vbuffer->Lock(0, vbuffer->GetVertexCount());

            if (vertexData)
            {
                unsigned char *dataAlign = (unsigned char *)(vertexData + vertIdx_ * vertexSize);

                // get the alignment to the vertex color
                if ( elementMask & MASK_POSITION ) dataAlign += sizeof(Vector3);
                if ( elementMask & MASK_NORMAL )   dataAlign += sizeof(Vector3);

                unsigned &uColor = *reinterpret_cast<unsigned*>(dataAlign);
                uColor = uColors[vcolColorIdx_];

                vbuffer->Unlock();
            }
            if (++vertIdx_ >= numVertices)
            {
                vertIdx_ = 0;
                vcolColorIdx_ = ++vcolColorIdx_ % iNumColors;
            }
        }

        vcolTimer_.Reset();
    }
}




