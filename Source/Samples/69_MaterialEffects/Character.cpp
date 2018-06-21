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

#include "Character.h"
#include "SplashHandler.h"
#include "CollisionLayer.h"

#include <Urho3D/DebugNew.h>
//=============================================================================
//=============================================================================
#define MAX_STEPDOWN_HEIGHT     0.5f

//=============================================================================
//=============================================================================
Character::Character(Context* context) :
    LogicComponent(context),
    onGround_(false),
    okToJump_(true),
    inAirTimer_(0.0f),
    onMovingPlatform_(false),
    jumpStarted_(false)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Character::RegisterObject(Context* context)
{
    context->RegisterFactory<Character>();

    // These macros register the class attributes to the Context for automatic load / save handling.
    // We specify the Default attribute mode which means it will be used both for saving into file, and network replication
    URHO3D_ATTRIBUTE("Controls Yaw", float, controls_.yaw_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Controls Pitch", float, controls_.pitch_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("On Ground", bool, onGround_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("OK To Jump", bool, okToJump_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("In Air Timer", float, inAirTimer_, 0.0f, AM_DEFAULT);
}

void Character::Start()
{
    // init char anim, so we don't see the t-pose char as it's spawned
    AnimationController* animCtrl = node_->GetComponent<AnimationController>(true);
    animCtrl->PlayExclusive("Platforms/Models/BetaLowpoly/Beta_JumpLoop1.ani", 0, true, 0.0f);

    // anim trigger event
    AnimatedModel *animModel = node_->GetComponent<AnimatedModel>(true);
    SubscribeToEvent(animModel->GetNode(), E_ANIMATIONTRIGGER, URHO3D_HANDLER(Character, HandleAnimationTrigger));

    // Component has been inserted into its scene node. Subscribe to events now
    SubscribeToEvent(GetNode(), E_NODECOLLISION, URHO3D_HANDLER(Character, HandleNodeCollision));
}

void Character::FixedUpdate(float timeStep)
{
    /// \todo Could cache the components for faster access instead of finding them each frame
    RigidBody* body = GetComponent<RigidBody>();
    AnimationController* animCtrl = node_->GetComponent<AnimationController>(true);

    // Update the in air timer. Reset if grounded
    if (!onGround_)
        inAirTimer_ += timeStep;
    else
        inAirTimer_ = 0.0f;
    // When character has been in air less than 1/10 second, it's still interpreted as being on ground
    bool softGrounded = inAirTimer_ < INAIR_THRESHOLD_TIME;

    // Update movement & animation
    const Quaternion& rot = node_->GetRotation();
    Vector3 moveDir = Vector3::ZERO;
    const Vector3& velocity = body->GetLinearVelocity();
    // Velocity on the XZ plane
    Vector3 planeVelocity(velocity.x_, 0.0f, velocity.z_);

    if (controls_.IsDown(CTRL_FORWARD))
        moveDir += Vector3::FORWARD;
    if (controls_.IsDown(CTRL_BACK))
        moveDir += Vector3::BACK;
    if (controls_.IsDown(CTRL_LEFT))
        moveDir += Vector3::LEFT;
    if (controls_.IsDown(CTRL_RIGHT))
        moveDir += Vector3::RIGHT;

    // Normalize move vector so that diagonal strafing is not faster
    if (moveDir.LengthSquared() > 0.0f)
        moveDir.Normalize();

    // rotate movedir
    curMoveDir_ = rot * moveDir;

    if (!onMovingPlatform_ )
    {
        body->ApplyImpulse(curMoveDir_ * (softGrounded ? MOVE_FORCE : INAIR_MOVE_FORCE));
    }
    else
    {
        if (curMoveDir_.LengthSquared() > 0.0f)
        {
            Vector3 moveForce(curMoveDir_ *(softGrounded ? MOVE_FORCE : INAIR_MOVE_FORCE));

            if (onGround_)
            {
                Vector3 deltaLinVel = platformBody_->GetLinearVelocity() - body->GetLinearVelocity();
                deltaLinVel.y_ = 0.0f; // ignore vertical velocity
                deltaLinVel += (moveForce * FORCE_MULTIPLYER_ON_PLATFORM);
                body->ApplyImpulse(deltaLinVel);
            }
            else
            {
                body->ApplyImpulse(moveForce);
            }
        }
    }

    if (softGrounded)
    {
        // apply braking force when not on a moving platform
        if (!onMovingPlatform_ )
        {
            // brake force
            Vector3 upDirCenterWorld = Vector3::UP;
            Vector3 linVel = body->GetLinearVelocity();
            float relVel = upDirCenterWorld.DotProduct(linVel);
            Vector3 lateralFrictionDir = linVel - upDirCenterWorld * relVel;
            if (lateralFrictionDir.LengthSquared() > M_EPSILON)
            {
                lateralFrictionDir *= 1.0f/lateralFrictionDir.Length();
                Vector3 brakeForce = (linVel.Length() * -lateralFrictionDir) * BRAKE_FORCE;
                body->ApplyImpulse(brakeForce);
            }
        }

        isJumping_ = false;
        // Jump. Must release jump control between jumps
        if (controls_.IsDown(CTRL_JUMP))
        {
            isJumping_ = true;
            if (okToJump_)
            {
                okToJump_ = false;
                jumpStarted_ = true;
                body->ApplyImpulse(Vector3::UP * JUMP_FORCE);

                animCtrl->StopLayer(0);
                animCtrl->PlayExclusive("Platforms/Models/BetaLowpoly/Beta_JumpStart.ani", 0, false, 0.2f);
                animCtrl->SetTime("Platforms/Models/BetaLowpoly/Beta_JumpStart.ani", 0);
            }
        }
        else
        {
            okToJump_ = true;
        }
    }

    if ( !onGround_ || jumpStarted_ )
    {
        if (jumpStarted_)
        {
            if (animCtrl->IsAtEnd("Platforms/Models/BetaLowpoly/Beta_JumpStart.ani"))
            {
                animCtrl->PlayExclusive("Platforms/Models/BetaLowpoly/Beta_JumpLoop1.ani", 0, true, 0.3f);
                animCtrl->SetTime("Platforms/Models/BetaLowpoly/Beta_JumpLoop1.ani", 0);
                jumpStarted_ = false;
            }
        }
        else
        {
            const float rayDistance = 50.0f;
            PhysicsRaycastResult result;
            GetScene()->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(node_->GetPosition(), Vector3::DOWN), rayDistance, 0xff);

            if (result.body_ && result.distance_ > MAX_STEPDOWN_HEIGHT )
            {
                animCtrl->PlayExclusive("Platforms/Models/BetaLowpoly/Beta_JumpLoop1.ani", 0, true, 0.2f);
            }
            else if (result.body_ == NULL)
            {
                // fall to death animation
                animCtrl->PlayExclusive("Platforms/Models/BetaLowpoly/Beta_JumpLoop1.ani", 0, true, 0.2f);
            }
        }
    }
    else
    {
        // Play walk animation if moving on ground, otherwise fade it out
        if ((softGrounded) && !moveDir.Equals(Vector3::ZERO))
        {
            animCtrl->PlayExclusive("Platforms/Models/BetaLowpoly/Beta_Run.ani", 0, true, 0.2f);
        }
        else
        {
            animCtrl->PlayExclusive("Platforms/Models/BetaLowpoly/Beta_Idle.ani", 0, true, 0.2f);
        }

        // Set walk animation speed proportional to velocity
        //animCtrl->SetSpeed("Platforms/Models/BetaLowpoly/Beta_Run.ani", planeVelocity.Length() * 0.3f);
    }

    // Reset grounded flag for next frame
    onGround_ = false;
    inWater_ = false;
}

void Character::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    // Check collision contacts and see if character is standing on ground (look for a contact that has near vertical normal)
    using namespace NodeCollision;

    // handle triggers
    RigidBody *otherBody = (RigidBody*)eventData[P_OTHERBODY].GetVoidPtr();
    bool isWater = false;
    if (otherBody->IsTrigger())
    {
        if (otherBody->GetCollisionLayer() == ColLayer_Water)
        {
            isWater = true;
        }
        else
        {
            return;
        }
    }

    MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());

    while (!contacts.IsEof())
    {
        Vector3 contactPosition = contacts.ReadVector3();
        Vector3 contactNormal = contacts.ReadVector3();
        /*float contactDistance = */contacts.ReadFloat();
        /*float contactImpulse = */contacts.ReadFloat();

        // If contact is below node center and pointing up, assume it's a ground contact
        if (contactPosition.y_ < (node_->GetPosition().y_ + 1.0f))
        {
            if (isWater)
            {
                waterContatct_ = contactPosition;
                inWater_ = true;
                break;
            }

            float level = contactNormal.y_;
            if (level > 0.75)
            {
                onGround_ = true;
            }
        }
    }
}

void Character::HandleAnimationTrigger(StringHash eventType, VariantMap& eventData)
{
    using namespace AnimationTrigger;

    Animation *animation = (Animation*)eventData[P_ANIMATION].GetVoidPtr();
    String strAction = eventData[P_DATA].GetString();

    // footsetp
    if (strAction.Find("foot", 0, false) != String::NPOS)
    {
        // footstep particle
        Node *footNode = node_->GetChild(strAction, true);

        if (footNode)
        {
            Vector3 fwd = node_->GetWorldDirection();
            Vector3 pos = footNode->GetWorldPosition();// + Vector3(0.0, 0.2f, 0.0f);

            if (inWater_)
            {
                pos.y_ = waterContatct_.y_;
                SendSplashEvent(pos, fwd);
            }
        }
    }
}

void Character::SendSplashEvent(const Vector3 &pos, const Vector3 &dir)
{
    using namespace SplashEvent;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_POS] = pos;
    eventData[P_DIR] = dir;
    eventData[P_SPL1] = Splash_Ripple;
    SendEvent(E_SPLASH, eventData);
}
