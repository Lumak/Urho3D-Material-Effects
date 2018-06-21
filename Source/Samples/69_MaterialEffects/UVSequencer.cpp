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
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Material.h>
#include <stdio.h>
#include <SDL/SDL_log.h>

#include "UVSequencer.h"

#include <Urho3D/DebugNew.h>
//=============================================================================
//=============================================================================
void UVSequencer::RegisterObject(Context* context)
{
    context->RegisterFactory<UVSequencer>();

    // type
    URHO3D_ATTRIBUTE("uvSeqType",       int,        uvSeqType_,      0,             AM_DEFAULT );
    URHO3D_ATTRIBUTE("enabled",         bool,       enabled_,        false,         AM_DEFAULT );
    URHO3D_ATTRIBUTE("repeat",          bool,       repeat_,         false,         AM_DEFAULT );

    // uv scroll
    URHO3D_ATTRIBUTE("uScrollSpeed",    float,      uScrollSpeed_,   0.0f,          AM_DEFAULT );
    URHO3D_ATTRIBUTE("vScrollSpeed",    float,      vScrollSpeed_,   0.0f,          AM_DEFAULT );
    URHO3D_ATTRIBUTE("timerFraction",   float,      timerFraction_,  1.0f,          AM_DEFAULT );

    // uv offset
    URHO3D_ATTRIBUTE("rows",            int,        rows_,           0,             AM_DEFAULT );
    URHO3D_ATTRIBUTE("cols",            int,        cols_,           0,             AM_DEFAULT );
    URHO3D_ATTRIBUTE("numFrames",       int,        numFrames_,      0,             AM_DEFAULT );
    URHO3D_ATTRIBUTE("timePerFrame",    unsigned,   timePerFrame_,   0,             AM_DEFAULT );

    // image swap - this doesn't belong but it's here to support the original demo
    URHO3D_ATTRIBUTE("swapTUEnum",      unsigned,   swapTUenum_,     0,             AM_DEFAULT );
    URHO3D_ATTRIBUTE("swapBegIdx",      int,        swapBegIdx_,     0,             AM_DEFAULT );
    URHO3D_ATTRIBUTE("swapEndIdx",      int,        swapEndIdx_,     0,             AM_DEFAULT );
    URHO3D_ATTRIBUTE("swapPrefixName",  String,     swapPrefixName_, String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("swapFileExt",     String,     swapFileExt_,    String::EMPTY, AM_DEFAULT );
    URHO3D_ATTRIBUTE("swapDecFormat",   String,     swapDecFormat_,  String::EMPTY, AM_DEFAULT );
}

UVSequencer::UVSequencer(Context* context) 
    : LogicComponent(context)
    , uvSeqType_(0)
    , uScrollSpeed_(0.0f)
    , vScrollSpeed_(0.0f)
    , timerFraction_(1.0f)
    , rows_(0)
    , cols_(0)      
    , numFrames_(0)
    , timePerFrame_(0)
    , enabled_(false)
    , repeat_(false)
    , curUVOffset_(Vector2::ZERO)
    , curFrameIdx_(0)
    , curImageIdx_(0)
    , swapTUenum_(0)
    , swapBegIdx_(0)
    , swapEndIdx_(0)
    , decFormat_(NULL)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void UVSequencer::DelayedStart()
{
    // currently only looks for billboardset and staticmodel, add more if you need
    // **sadly, the drawable base class doesn't have a virutal GetMaterial() function
    if (node_->GetComponent<BillboardSet>())
    {
        drawableComponent_ = node_->GetComponent<BillboardSet>();
        componentMat_ = node_->GetComponent<BillboardSet>()->GetMaterial();
    }
    else if (node_->GetComponent<StaticModel>())
    {
        componentMat_ = node_->GetComponent<StaticModel>()->GetMaterial();
        drawableComponent_ = node_->GetComponent<StaticModel>();
    }

    // init
    Reset();

    // auto start
    if (!enabled_)
    {
        SetUpdateEventMask(0);
    }
}

bool UVSequencer::SetEnabled(bool enable)
{
    if (enable == enabled_)
    {
        return false;
    }

    enabled_ = enable;

    if (enabled_)
    {
        SetUpdateEventMask(USE_FIXEDUPDATE);
    }
    else
    {
        SetUpdateEventMask(0);
    }

    return true;
}

bool UVSequencer::Reset()
{
    // init common
    curFrameIdx_ = 0;
    curImageIdx_ = 0;
    curUVOffset_ = Vector2::ZERO;
    seqTimer_.Reset();

    // and specifics 
    switch (uvSeqType_)
    {
    case UVSeq_UScroll:
        componentMat_->SetShaderParameter("UOffset", Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        break;

    case UVSeq_VScroll:
        componentMat_->SetShaderParameter("VOffset", Vector4(0.0f, 1.0f, 0.0f, 1.0f));
        break;

    case UVSeq_UVFrame:
        InitUVFrameSize();
        UpdateUVFrame();
        break;

    case UVSeq_SwapImage:
        InitSwapDecFormat();
        break;
    }

    return true;
}

void UVSequencer::InitSwapDecFormat()
{
    // init idx
    curImageIdx_ = swapBegIdx_;

    // set dec format
    if ( swapDecFormat_.StartsWith("0") )
    {
        Variant var(VAR_INT, swapDecFormat_.CString()+1);
        decFormat_ = GetDecFormat(var.GetInt(), true);
    }
    else
    {
        Variant var(VAR_INT, swapDecFormat_.CString());
        decFormat_ = GetDecFormat(var.GetInt(), false);
    }
}

void UVSequencer::InitUVFrameSize()
{
    uvFrameSize_.x_ = 1.0f/(float)cols_;
    uvFrameSize_.y_ = 1.0f/(float)rows_;
}

void UVSequencer::FixedUpdate(float timeStep)
{
    // skip if not in view
    if (!drawableComponent_->IsInView())
        return;

    // update
    switch (uvSeqType_)
    {
    case UVSeq_UScroll:
        UpdateUScroll(timeStep);
        break;

    case UVSeq_VScroll:
        UpdateVScroll(timeStep);
        break;

    case UVSeq_UVFrame:
        UpdateUVFrame();
        break;

    case UVSeq_SwapImage:
        UpdateSwapImage();
        break;
    }
}

void UVSequencer::UpdateUScroll(float timeStep)
{
    curUVOffset_.x_ += uScrollSpeed_ + Sign(uScrollSpeed_) *  timeStep * timerFraction_;
    componentMat_->SetShaderParameter("UOffset", Vector4(1.0f, 0.0f, 0.0f, curUVOffset_.x_));
}

void UVSequencer::UpdateVScroll(float timeStep)
{
    curUVOffset_.y_ += vScrollSpeed_ + Sign(vScrollSpeed_) * timeStep * timerFraction_;
    componentMat_->SetShaderParameter("VOffset", Vector4(0.0f, 1.0f, 0.0f, curUVOffset_.y_));
}

void UVSequencer::UpdateUVFrame()
{
    if (seqTimer_.GetMSec(false) > timePerFrame_)
    {
        if ( ++curFrameIdx_ < numFrames_ )
        {
            UpdateUVFrameShader();
        }
        else
        {
            if (repeat_)
            {
                curFrameIdx_ = 0;

                UpdateUVFrameShader();
            }
            else
            {
                SetUpdateEventMask(0);
            }
        }

        seqTimer_.Reset();
    }
}

void UVSequencer::UpdateUVFrameShader()
{
    float curRow = (float)(curFrameIdx_ / cols_);
    float curCol = (float)(curFrameIdx_ % cols_);

    componentMat_->SetShaderParameter("CurRowCol", Vector2(curRow, curCol));
}

void UVSequencer::UpdateSwapImage()
{
    if (seqTimer_.GetMSec(false) > timePerFrame_)
    {
        if ( ++curImageIdx_ < swapEndIdx_ )
        {
            UpdateSwapImageTexture();
        }
        else
        {
            if (repeat_)
            {
                curImageIdx_ = swapBegIdx_;

                UpdateSwapImageTexture();
            }
            else
            {
                SetUpdateEventMask(0);
            }
        }

        seqTimer_.Reset();
    }
}

void UVSequencer::UpdateSwapImageTexture()
{
    char buf[10];
    sprintf(buf, decFormat_, curImageIdx_);
    String diffName = swapPrefixName_ + String(buf) + String(".") + swapFileExt_;

    // update texture
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    componentMat_->SetTexture((TextureUnit)swapTUenum_, cache->GetResource<Texture2D>(diffName));
}

const char *UVSequencer::GetDecFormat(int idx, bool leadingZero)
{
    const char *dec1  = "%1d";
    const char *dec2  = "%2d";
    const char *dec3  = "%3d";
    const char *dec02 = "%02d";
    const char *dec03 = "%03d";

    if (idx == 1)
    {
        return dec1;
    }
    else if (idx == 2)
    {
        return leadingZero?dec02:dec2;
    }

    return leadingZero?dec03:dec3;
}



