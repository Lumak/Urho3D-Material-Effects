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

#pragma once

#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

namespace Urho3D
{
}
//=============================================================================
//=============================================================================
enum SplashTypes
{
    Splash_Invalid,
    Splash_Water,
    Splash_Ripple,
    Splash_WaterfallSplash,
    Splash_LavaBubble,
    Splash_MAX,
};

URHO3D_EVENT(E_SPLASH, SplashEvent)
{
	URHO3D_PARAM(P_POS, Pos);
	URHO3D_PARAM(P_DIR, Dir);
	URHO3D_PARAM(P_SPL1, Type1);
}

//=============================================================================
//=============================================================================
class SplashData : public Serializable
{
    URHO3D_OBJECT(SplashData, Serializable);
public:
    SplashData(Context *context);
    virtual ~SplashData(){}
    static void RegisterObject(Context* context);

    //virtual bool LoadXML(const XMLElement& source, bool setInstanceDefault = false);
    SplashData* Copy(const SplashData *rhs)
    {
        matFile          = rhs->matFile;
        splashType       = rhs->splashType;
        maxImages        = rhs->maxImages;
        totalDuration    = rhs->totalDuration;
        timePerFrame     = rhs->timePerFrame;
        uOffset          = rhs->uOffset;
        vOffset          = rhs->vOffset;
        uIncrPerFrame    = rhs->uIncrPerFrame;
        vIncrPerFrame    = rhs->vIncrPerFrame;
        scaleRate        = rhs->scaleRate;
        transparencyRate = rhs->transparencyRate;
        pos              = rhs->pos;
        direction        = rhs->direction;
        scale            = rhs->scale;
        faceCamMode      = rhs->faceCamMode;

        return this;
    }

public:
    String        matFile;
    int           splashType;
    int           maxImages;
    unsigned      totalDuration;
    unsigned      timePerFrame;
    float         uOffset;
    float         vOffset;
    float         uIncrPerFrame;
    float         vIncrPerFrame;
    Vector3       scaleRate;
    float         transparencyRate;
                  
    Vector3       pos;
    Vector3       direction;
    Vector3       scale;
    unsigned      faceCamMode;

    unsigned      elapsedTime;
    int           curImageIdx;
    float         uCur;
    float         vCur;

    WeakPtr<Node> node;
    Timer         timer;
};

//=============================================================================
//=============================================================================
class SplashHandler : public LogicComponent
{
    URHO3D_OBJECT(SplashHandler, LogicComponent);
public:
    SplashHandler(Context *context);
    virtual ~SplashHandler();
    static void RegisterObject(Context* context);
    bool LoadSplashList(const String &strlist);

protected:
    virtual void Start();
    virtual void FixedUpdate(float timeStep);

    bool CreateDrawableObj(SplashData *splashData);
    void HandleSplashEvent(StringHash eventType, VariantMap& eventData);

protected:
    Vector<SharedPtr<SplashData> > registeredSplashList_;
    Vector<SharedPtr<SplashData> > activeSplashList_;
};

//=============================================================================
//=============================================================================
class SplashDataList : public Serializable
{
    URHO3D_OBJECT(SplashDataList, Serializable);
public:
    SplashDataList(Context *context) : Serializable(context){}
    virtual ~SplashDataList(){}
    static void RegisterObject(Context* context);
    virtual bool LoadXML(const XMLElement& source, bool setInstanceDefault = false);

    Vector<String*> splashList_;

protected:
    String item00;
    String item01;
    String item02;
    String item03;
    String item04;
    String item05;
    String item06;
    String item07;
    String item08;
    String item09;
};


