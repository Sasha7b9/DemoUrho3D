#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Resource/ResourceCache.h>

#include "Turret.h"

Turret::Turret(Context* context) :
    LogicComponent(context)
{
    SetUpdateEventMask(USE_UPDATE);
}

void Turret::RegisterObject(Context* context)
{
    context->RegisterFactory<Turret>();
}

void Turret::Start()
{
    Node *lightNode = node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->CreateChild("Beacon");
    Light *light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_POINT);
    light->SetCastShadows(false);
    light->SetRange(0.33f);
    light->SetBrightness(200.0f);
    light->SetColor(Color::RED);

    Vector3 position = {0.0f, 2.5f, -0.075f};

    lightNode->SetPosition(position);

    Node *lightShot = node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->CreateChild("LightL");
    light = lightShot->CreateComponent<Light>();
    light->SetLightType(LIGHT_POINT);
    light->SetCastShadows(false);
    light->SetRange(0.1f);
    light->SetBrightness(200.0f);
    light->SetColor(Color::RED);

    position = {2.4f, 0.7f, -1.1f};
    lightShot->SetPosition(position);

    lightShot = node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->CreateChild("LightR");
    light = lightShot->CreateComponent<Light>();
    light->SetLightMask(LIGHT_POINT);
    light->SetCastShadows(false);
    light->SetRange(0.1f);
    light->SetBrightness(200.0f);
    light->SetColor(Color::RED);

    position = {-2.4f, 0.7f, -1.1f};
    lightShot->SetPosition(position);

    /*
    Node *cubeNode = node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->CreateChild("");
    StaticModel *cube = cubeNode->CreateComponent<StaticModel>();
    cube->SetModel(GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl"));
    cubeNode->SetScale(0.25f);
    cubeNode->SetPosition(position);
    */
}

void Turret::Update(float timeStep)
{
    float speed = 10.0f;

    float maxAngle = speed * timeStep;

    Node *nodeJack = GetScene()->GetChild("Jack");

    if (GetDistance(nodeJack) > 25.0f)
    {
        RotateToDefault(maxAngle);
        beaconEnabled = false;
    }
    else
    {
        RotateToTarget(nodeJack, maxAngle, timeStep);
        beaconEnabled = true;
    }

    UpdateLights();
}

void Turret::AnimateGun(Bone *bone, float timeStep)
{
    rotateGun += timeStep * 360.0f * 2.0f;
    bone->node_->SetRotation(Quaternion(90.0f, Vector3::RIGHT) * Quaternion(rotateGun, Vector3::UP));
}

void Turret::SetRotate(float angle)
{
    Skeleton& skeleton = node_->GetComponent<AnimatedModel>()->GetSkeleton();

    skeleton.GetBone("MasterBone")->node_->SetRotation(Quaternion(angle, Vector3::UP));
    worldRotationDefault = skeleton.GetBone("Bone1")->node_->GetWorldRotation().YawAngle();
}

void Turret::Logging()
{
    float angle = node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->GetWorldRotation().YawAngle();
    URHO3D_LOGINFOF("%f", angle);
}

void Turret::RotateToDefault(float maxAngle)
{
    float worldRotation = node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->GetWorldRotation().YawAngle();

    float angle = AngularDifference(worldRotationDefault, worldRotation);

    if (fabs(angle) > maxAngle)
    {
        if (angle < 0)
        {
            angle = worldRotation - maxAngle;
        }
        else
        {
            angle = worldRotation + maxAngle;
        }
    }
}

void Turret::RotateToTarget(Node *node, float maxAngle, float timeStep)
{
    Bone *bone = node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1");

    Vector3 position = node_->GetPosition();
    Vector3 positionTarget = node->GetPosition();

    float angle = NormalizeAngle(Atan2(positionTarget.x_ - position.x_, positionTarget.z_ - position.z_) + 180.0f);

    float delta = NormalizeAngle(angle - bone->node_->GetWorldRotation().YawAngle());

    gunsEnabled = false;

    if (Abs(delta) < 25.0f)
    {
        AnimateGun(node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("GunR"), timeStep);
        AnimateGun(node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("GunL"), timeStep);
        gunsEnabled = true;

        if (Abs(delta) < 1.0f)
        {
            return;
        }
    }

    float sign = 1.0f;

    if (delta != 0.0f)
    {
        sign = delta / Abs(delta);
    }

    if (Abs(delta) > maxAngle)
    {
        delta = maxAngle;
    }

    bone->node_->SetWorldRotation(Quaternion(bone->node_->GetWorldRotation().YawAngle() + delta * sign, Vector3::UP));
}

float Turret::GetDistance(Node *node)
{
    Vector3 position = node_->GetPosition();
    Vector3 positionJack = node->GetPosition();

    float deltaZ = positionJack.z_ - position.z_;
    float deltaX = positionJack.x_ - position.x_;

    return sqrt(deltaX * deltaX + deltaZ * deltaZ);
}


float Turret::GetDirection(Node *node)
{
    Vector3 position = node_->GetPosition();
    Vector3 positionJack = node->GetPosition();

    float deltaZ = positionJack.z_ - position.z_;
    float deltaX = positionJack.x_ - position.x_;

    float tangens = deltaX / deltaZ;

    float angle = Atan(tangens);

    if (deltaZ > 0.0f)
    {
        angle += 180.0f;
    }

    return angle;
}

float Turret::AngularDifference(float angle1, float angle2)
{
    angle1 = ConvertAngle(angle1);
    angle2 = ConvertAngle(angle2);

    float diff = fabs(angle1 - angle2);

    if (diff > 180.0f)
    {
        diff = 360.0f - diff;
    }

    return diff * (angle1 < angle2 ? 1.0f : -1.0f);
}

float Turret::ConvertAngle(float angle)
{
    if (angle < 0.0f)
    {
        angle = 360 + angle;
    }
    return angle;
}
 
float Turret::NormalizeAngle(float angle)
{
    if (angle < -180.0f)
    {
        return angle + 360.0f;
    }
    if (angle > 180.0f)
    {
        return angle - 360.0f;
    }
    return angle;
}

void Turret::UpdateLights()
{
    node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->GetChild("Beacon")->SetEnabled(beaconEnabled);
    node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->GetChild("LightL")->SetEnabled(gunsEnabled);
    node_->GetComponent<AnimatedModel>()->GetSkeleton().GetBone("Bone1")->node_->GetChild("LightR")->SetEnabled(gunsEnabled);
}
