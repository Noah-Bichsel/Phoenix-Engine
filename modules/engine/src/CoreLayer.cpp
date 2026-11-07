#include "../include/engine/CoreLayer.h"
#include <core/Time.h>

using namespace Phoenix;

void CoreLayer::OnAttach()
{

}

void CoreLayer::OnDetach()
{

}

void CoreLayer::OnUpdate()
{
    Time::Update();
}
