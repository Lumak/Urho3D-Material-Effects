//
// Copyright (c) 2008-2016 the Urho3D project.
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/DrawableEvents.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <SDL/SDL_log.h>

#include "SplashHandler.h"

#include <Urho3D/DebugNew.h>
//=============================================================================
//=============================================================================
void SplashHandler::RegisterObject(Context* context)
{
    context->RegisterFactory<SplashHandler>();
    SplashDataList::RegisterObject(context);
    SplashData::RegisterObject(context);
}

SplashHandler::SplashHandler(Context *context) 
    : LogicComponent(context)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

SplashHandler::~SplashHandler()
{
}

void SplashHandler::Start()
{
}

bool SplashHandler::LoadSplashList(const String &strlist)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    SharedPtr<SplashDataList> splashList( new SplashDataList(context_) );
    XMLFile *xmlList = cache->GetResource<XMLFile>(strlist);

    if (xmlList && splashList->LoadXML(xmlList->GetRoot()) )
    {
        for ( unsigned i = 0; i < splashList->splashList_.Size(); ++i )
        {
            String strSplashData = *splashList->splashList_[i];

            if (!strSplashData.Empty())
            {
                XMLFile *xmlFile = cache->GetResource<XMLFile>(strSplashData);

                if (xmlFile)
                {
                    SharedPtr<SplashData> splashData( new SplashData(context_) );
                    if (splashData->LoadXML(xmlFile->GetRoot()))
                        registeredSplashList_.Push(splashData);
                }
            }
        }
    }

    if (registeredSplashList_.Size() > 0)
    {
        SubscribeToEvent(E_SPLASH, URHO3D_HANDLER(SplashHandler, HandleSplashEvent));
    }
    return true;
}

void SplashHandler::FixedUpdate(float timeStep)
{
    for ( unsigned i = 0; i < activeSplashList_.Size(); ++i )
    {
        SplashData *splashData = activeSplashList_[i];
        splashData->elapsedTime += (unsigned)(timeStep * 1000.0f);

        if (splashData->elapsedTime > splashData->totalDuration)
            continue;

        // update billboard
        Node *splashNode = splashData->node;
        BillboardSet *bbset = splashNode->GetComponent<BillboardSet>();
        Billboard* bboard = bbset->GetBillboard(0);
        Material *mat = bbset->GetMaterial();

        switch (splashData->splashType)
        {
        case Splash_Water:
            break;

        case Splash_Ripple:
            {
                bboard->size_ = bboard->size_ * Vector2(splashData->scaleRate.x_, splashData->scaleRate.y_);
                bbset->Commit();
                Color matCol = mat->GetShaderParameter("MatDiffColor").GetColor();
                matCol.a_ *= splashData->transparencyRate;
                mat->SetShaderParameter("MatDiffColor", matCol);
            }
            break;

        case Splash_WaterfallSplash:
            break;

        case Splash_LavaBubble:
            break;

        }
    }

    // remove expired
    for ( unsigned i = 0; i < activeSplashList_.Size(); ++i )
    {
        SplashData *splashData = activeSplashList_[i];

        if (splashData->elapsedTime > splashData->totalDuration)
        {
            GetScene()->RemoveChild(splashData->node);
            activeSplashList_.Erase(i);
            --i;
        }
    }
}

void SplashHandler::HandleSplashEvent(StringHash eventType, VariantMap& eventData)
{
    using namespace SplashEvent;

	Vector3 pos = eventData[P_POS].GetVector3();
	Vector3 dir = eventData[P_DIR].GetVector3();
	int sptype  = eventData[P_SPL1].GetInt();

    // create splash
    for (unsigned i = 0; i < registeredSplashList_.Size(); ++i)
    {
        if (registeredSplashList_[i]->splashType == sptype)
        {
            SharedPtr<SplashData> newSplashData(new SplashData(context_));
            newSplashData->Copy( registeredSplashList_[i] );

            newSplashData->node = GetScene()->CreateChild();
            newSplashData->node->SetPosition(pos);
            newSplashData->node->SetDirection(Vector3::DOWN);

            if (CreateDrawableObj(newSplashData))
            {
                newSplashData->timer.Reset();
                activeSplashList_.Push(newSplashData);
            }
            break;
        }
    }
}

bool SplashHandler::CreateDrawableObj(SplashData *splashData)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    switch (splashData->splashType)
    {
    case Splash_Water:
        break;

    case Splash_Ripple:
        {
            BillboardSet *bbset = splashData->node->CreateComponent<BillboardSet>();
            bbset->SetNumBillboards(1);
            Material *mat = cache->GetResource<Material>(splashData->matFile);
            bbset->SetMaterial(mat->Clone());
            bbset->SetFaceCameraMode((FaceCameraMode)splashData->faceCamMode);
            Billboard* bboard = bbset->GetBillboard(0);
            bboard->size_ = Vector2(splashData->scale.x_, splashData->scale.y_);
            bboard->enabled_ = true;
        }
        break;

    case Splash_WaterfallSplash:
        break;

    case Splash_LavaBubble:
        break;

    }

    return true;
}


//=============================================================================
//=============================================================================
void SplashData::RegisterObject(Context* context)
{
    context->RegisterFactory<SplashData>();

    URHO3D_ATTRIBUTE("matFile",          String,     matFile,          String::EMPTY,  AM_DEFAULT );
    URHO3D_ATTRIBUTE("splashType",       int,        splashType,       0,              AM_DEFAULT );
    URHO3D_ATTRIBUTE("maxImages",        int,        maxImages,        0,              AM_DEFAULT );
    URHO3D_ATTRIBUTE("duration",         unsigned,   totalDuration,    0,              AM_DEFAULT );
    URHO3D_ATTRIBUTE("timePerFrame",     unsigned,   timePerFrame,     0,              AM_DEFAULT );
    URHO3D_ATTRIBUTE("uInc",             float,      uIncrPerFrame,    0.0f,           AM_DEFAULT );
    URHO3D_ATTRIBUTE("vInc",             float,      vIncrPerFrame,    0.0f,           AM_DEFAULT );
    URHO3D_ATTRIBUTE("uOffset",          float,      uOffset,          0.0f,           AM_DEFAULT );
    URHO3D_ATTRIBUTE("vOffset",          float,      vOffset,          0.0f,           AM_DEFAULT );
    URHO3D_ATTRIBUTE("scaleRate",        Vector3,    scaleRate,        Vector3::ZERO,  AM_DEFAULT );
    URHO3D_ATTRIBUTE("transparencyRate", float,      transparencyRate, 0.0f,           AM_DEFAULT );
    URHO3D_ATTRIBUTE("scale",            Vector3,    scale,            Vector3::ONE,   AM_DEFAULT );
    URHO3D_ATTRIBUTE("faceCamMode",      unsigned,   faceCamMode,      0,              AM_DEFAULT );
}

SplashData::SplashData(Context *context)
    : Serializable(context)
    , splashType(0)
    , maxImages(0)
    , totalDuration(0)
    , timePerFrame(0)
    , uOffset(0.0f)
    , vOffset(0.0f)
    , uIncrPerFrame(0.0f)
    , vIncrPerFrame(0.0f)
    , elapsedTime(0)
    , curImageIdx(0)
    , uCur(0.0f)
    , vCur(0.0f)
    , transparencyRate(0.0f)
    , scale(Vector3::ONE)
    , faceCamMode(0)

{
    timer.Reset();
}

//=============================================================================
//=============================================================================
void SplashDataList::RegisterObject(Context* context)
{
    context->RegisterFactory<SplashDataList>();

    // lists
    URHO3D_ATTRIBUTE("item00",  String, item00,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item01",  String, item01,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item02",  String, item02,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item03",  String, item03,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item04",  String, item04,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item05",  String, item05,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item06",  String, item06,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item07",  String, item07,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item08",  String, item08,  String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("item09",  String, item09,  String::EMPTY, AM_DEFAULT );
}

bool SplashDataList::LoadXML(const XMLElement& source, bool setInstanceDefault)
{
    if ( !Serializable::LoadXML(source, setInstanceDefault) )
        return false;

    splashList_.Push(&item00);
    splashList_.Push(&item01);
    splashList_.Push(&item02);
    splashList_.Push(&item03);
    splashList_.Push(&item04);
    splashList_.Push(&item05);
    splashList_.Push(&item06);
    splashList_.Push(&item07);
    splashList_.Push(&item08);
    splashList_.Push(&item09);

    return (splashList_.Size() > 0);
}
