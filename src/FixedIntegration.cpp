#include "FixedIntegration.h"

using namespace DirectX;
using namespace std;

FixedIntegration::FixedIntegration(float timeStep, GridData& index)
	:TimeIntegration(timeStep, index)
{
}

FixedIntegration::~FixedIntegration()
{
}

float FixedIntegration::computeGridTimeStep(DirectX::XMFLOAT2 vel)
{
	return _timeStep;
}

float FixedIntegration::computeParticleTimeStep(DirectX::XMFLOAT2 vel, int i)
{
	return _timeStep;
}

void FixedIntegration::computeGlobalTimeStep(vector<XMFLOAT2>& vel, vector<STATE>& state)
{
}
