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
enum UVSeqType
{
    UVSeq_UScroll,      // 0
    UVSeq_VScroll,      // 1
    UVSeq_UVFrame,      // 2
    UVSeq_SwapImage,    // 3
};

//=============================================================================
//=============================================================================
class UVSequencer : public LogicComponent
{
    URHO3D_OBJECT(UVSequencer, LogicComponent);

public:
    UVSequencer(Context* context);
    virtual ~UVSequencer(){}

    static void RegisterObject(Context* context);
    virtual void DelayedStart();
    bool SetEnabled(bool enable);
    bool Reset();

protected:
    virtual void FixedUpdate(float timeStep);
    void UpdateUScroll(float timeStep);
    void UpdateVScroll(float timeStep);
    void UpdateUVFrame();
    void UpdateUVFrameShader();
    void UpdateSwapImage();
    void UpdateSwapImageTexture();

    void InitSwapDecFormat();
    void InitUVFrameSize();
    static const char *UVSequencer::GetDecFormat(int idx, bool leadingZero);
    
protected:
    WeakPtr<Drawable> drawableComponent_;
    WeakPtr<Material> componentMat_;

    // type
    int               uvSeqType_;
    bool              enabled_;
    bool              repeat_;

    // uv scroll
    float             uScrollSpeed_;
    float             vScrollSpeed_;
    float             timerFraction_;       // something to even slow the timer (lava)

    // uv offset
    int               rows_;
    int               cols_;
    int               numFrames_;
    unsigned          timePerFrame_;

    // image swap - this doesn't belong but it's here to support the original demo
    unsigned          swapTUenum_;
    int               swapBegIdx_;
    int               swapEndIdx_;
    String            swapPrefixName_;
    String            swapFileExt_;
    String            swapDecFormat_;
    const char        *decFormat_;

    // status update
    Vector2           curUVOffset_;
    Vector2           uvFrameSize_;
    int               curFrameIdx_;
    int               curImageIdx_;
    Timer             seqTimer_;

};

