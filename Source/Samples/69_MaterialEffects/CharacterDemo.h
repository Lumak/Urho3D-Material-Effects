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

#include "Sample.h"

namespace Urho3D
{

class Node;
class Scene;

}

class Character;
class Touch;
//=============================================================================
//=============================================================================
enum EmissionState
{
    EmissionState_R,
    EmissionState_None1,
    EmissionState_G,
    EmissionState_None2,
    EmissionState_B,
    EmissionState_None3,
    EmissionState_MAX,
};

enum LightmapType
{
    Max_Lightmaps = 3
};

//=============================================================================
//=============================================================================
class CharacterDemo : public Sample
{
    URHO3D_OBJECT(CharacterDemo, Sample);

public:
    /// Construct.
    CharacterDemo(Context* context);
    /// Destruct.
    ~CharacterDemo();

    virtual void Setup();
    /// Setup after engine initialization and before running the main loop.
    virtual void Start();

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    virtual String GetScreenJoystickPatchString() const { return
        "<patch>"
        "    <add sel=\"/element\">"
        "        <element type=\"Button\">"
        "            <attribute name=\"Name\" value=\"Button3\" />"
        "            <attribute name=\"Position\" value=\"-120 -120\" />"
        "            <attribute name=\"Size\" value=\"96 96\" />"
        "            <attribute name=\"Horiz Alignment\" value=\"Right\" />"
        "            <attribute name=\"Vert Alignment\" value=\"Bottom\" />"
        "            <attribute name=\"Texture\" value=\"Texture2D;Textures/TouchInput.png\" />"
        "            <attribute name=\"Image Rect\" value=\"96 0 192 96\" />"
        "            <attribute name=\"Hover Image Offset\" value=\"0 0\" />"
        "            <attribute name=\"Pressed Image Offset\" value=\"0 0\" />"
        "            <element type=\"Text\">"
        "                <attribute name=\"Name\" value=\"Label\" />"
        "                <attribute name=\"Horiz Alignment\" value=\"Center\" />"
        "                <attribute name=\"Vert Alignment\" value=\"Center\" />"
        "                <attribute name=\"Color\" value=\"0 0 0 1\" />"
        "                <attribute name=\"Text\" value=\"Gyroscope\" />"
        "            </element>"
        "            <element type=\"Text\">"
        "                <attribute name=\"Name\" value=\"KeyBinding\" />"
        "                <attribute name=\"Text\" value=\"G\" />"
        "            </element>"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">1st/3rd</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"F\" />"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Jump</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"SPACE\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }

protected:
    /// Create static scene content.
    void CreateScene();
    void ChangeDebugHudText();
    /// Create controllable character.
    void CreateCharacter();
    void InitSplashHandler();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Subscribe to necessary events.
    void SubscribeToEvents();
    /// Handle application update. Set controls to character.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle application post-update. Update camera position after character has moved.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    void CreateSequencers();
    void CreateWaterRefection();

    void UpdateEmission(float timeStep);
    void UpdateLightmap(float timeStep);
    void UpdateVertexColor(float timeStep);

protected:
    /// Touch utility object.
    SharedPtr<Touch> touch_;
    /// The controllable character component.
    WeakPtr<Character> character_;
    /// First person camera flag.
    bool firstPerson_;

    // emission
    Color emissionColor_;
    Timer emissionTimer_;
    int   emissionState_;

    // lightmap
    String lightmapPathName_;
    Timer  lightmapTimer_;
    int    lightmapIdx_;

    // vcol
    int      vcolColorIdx_;
    unsigned vertIdx_;
    Timer    vcolTimer_;

    // water reflection
    SharedPtr<Node> reflectionCameraNode_;
    SharedPtr<Node> waterNode_;
    Plane waterPlane_;
    Plane waterClipPlane_;

    // dbg
    Vector3 wallHitNormal_;
    bool drawDebug_;
};
