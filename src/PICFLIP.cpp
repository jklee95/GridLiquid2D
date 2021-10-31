#include "PICFLIP.h"

using namespace DirectX;
using namespace std;

PICFLIP::PICFLIP(GridData& index, EX ex, float timeStep)
	:GridLiquid(index, timeStep)
{
	_initialize(ex);
}

PICFLIP::~PICFLIP()
{
}

void PICFLIP::setFlipRatio(int value)
{
	_flipRatio = static_cast<float>(value) / 100.0f;
}

void PICFLIP::_initialize(EX ex)
{
	GridLiquid::_initialize(ex);

	size_t vSize = static_cast<size_t>(_gridCount) * static_cast<size_t>(_gridCount);

	_oldVel.assign(vSize, { 0.0f, 0.0f });
	_tempVel.assign(vSize, { 0.0f, 0.0f });
	_pCount.assign(vSize, 0.0f);
}



void PICFLIP::_update()
{
	_advect(0);

	_force(0);
	_setBoundary(_gridVelocity);
	_setFreeSurface(_gridVelocity);

	_project(0);
	// Solve boundary condition again due to numerical errors in previous step
	_setBoundary(_gridVelocity);
	_updateParticlePos(0);

	_paintGrid();
}

XMINT2 _computeCenterMinMaxIndex(_VALUE vState, XMFLOAT2 particlePos)
{
	// 2.
	switch (vState)
	{
	case _VALUE::MIN:
		return { static_cast<int>(floor(particlePos.x)), static_cast<int>(floor(particlePos.y)) };
		break;
	case _VALUE::MAX:
		// 3.
		return { static_cast<int>(ceil(particlePos.x)), static_cast<int>(ceil(particlePos.y)) };
		break;
	default:
		return { -1, -1 };
		break;
	}
}

void PICFLIP::_advect(int iter)
{
	int N = _gridCount - 2;
	for (int i = 0; i < _particlePosition.size(); i++)
	{
		XMFLOAT2 pos = _particlePosition[i];

		XMINT2 minIndex = _computeCenterMinMaxIndex(_VALUE::MIN, pos);
		XMINT2 maxIndex = _computeCenterMinMaxIndex(_VALUE::MAX, pos);

		XMFLOAT2 ratio = pos - _gridPosition[_INDEX(minIndex.x, minIndex.y)];
		_pCount[_INDEX(minIndex.x, minIndex.y)] += (1.0f - ratio.x) * (1.0f - ratio.y);
		_pCount[_INDEX(minIndex.x, maxIndex.y)] += (1.0f - ratio.x) * ratio.y;
		_pCount[_INDEX(maxIndex.x, minIndex.y)] += ratio.x * (1.0f - ratio.y);
		_pCount[_INDEX(maxIndex.x, maxIndex.y)] += ratio.x * ratio.y;

		XMFLOAT2 minMin_minMax = _particleVelocity[i] * (1.0f - ratio.x);
		XMFLOAT2 maxMin_maxMax = _particleVelocity[i] * ratio.x;
		XMFLOAT2 minMin = minMin_minMax * (1.0f - ratio.y);
		XMFLOAT2 minMax = minMin_minMax * ratio.y;
		XMFLOAT2 maxMin = maxMin_maxMax * (1.0f - ratio.y);
		XMFLOAT2 maxMax = maxMin_maxMax * ratio.y;

		_tempVel[_INDEX(minIndex.x, minIndex.y)] += minMin;
		_tempVel[_INDEX(minIndex.x, maxIndex.y)] += minMax;
		_tempVel[_INDEX(maxIndex.x, minIndex.y)] += maxMin;
		_tempVel[_INDEX(maxIndex.x, maxIndex.y)] += maxMax;
	}

	for (int i = 0; i < _gridCount; i++)
	{
		for (int j = 0; j < _gridCount; j++)
		{

			if (_pCount[_INDEX(i, j)] > EPS_FLOAT)
			{
				_gridVelocity[_INDEX(i, j)] = _oldVel[_INDEX(i, j)] = _tempVel[_INDEX(i, j)] / _pCount[_INDEX(i, j)];
			}
			else
			{
				_gridVelocity[_INDEX(i, j)] = _oldVel[_INDEX(i, j)] = { 0.0f, 0.0f };
			}

			// Reset
			_tempVel[_INDEX(i, j)] = { 0.0f, 0.0f };
			_pCount[_INDEX(i, j)] = 0.0f;

		}
	}
}

void PICFLIP::_force(int iter)
{
	float dt = _timeStep;

	int N = _gridCount - 2;
	for (int i = 1; i <= N; i++)
	{
		for (int j = 1; j <= N; j++)
		{
			if (_gridState[_INDEX(i, j)] == STATE::FLUID)
			{
				_gridVelocity[_INDEX(i, j)].y -= 9.8f * dt;
			}

		}
	}
}


void PICFLIP::_project(int iter)
{
	int N = _gridCount - 2;
	for (int i = 1; i <= N; i++)
	{
		for (int j = 1; j <= N; j++)
		{
			_gridDivergence[_INDEX(i, j)] =
				0.5f * (_gridVelocity[_INDEX(i + 1, j)].x - _gridVelocity[_INDEX(i - 1, j)].x
					+ _gridVelocity[_INDEX(i, j + 1)].y - _gridVelocity[_INDEX(i, j - 1)].y);

			_gridPressure[_INDEX(i, j)] = 0.0f;
		}
	}

	_setBoundary(_gridDivergence);
	_setBoundary(_gridPressure);

	for (int iter = 0; iter < 200; iter++)
	{
		for (int i = 1; i <= N; i++)
		{
			for (int j = 1; j <= N; j++)
			{

				if (_gridState[_INDEX(i, j)] == STATE::FLUID)
				{
					_gridPressure[_INDEX(i, j)] =
						(
							_gridDivergence[_INDEX(i, j)] -
							(_gridPressure[_INDEX(i + 1, j)] + _gridPressure[_INDEX(i - 1, j)] +
								_gridPressure[_INDEX(i, j + 1)] + _gridPressure[_INDEX(i, j - 1)])
							) / -4.0f;
				}
			
			}

		}
		_setBoundary(_gridPressure);
	}

	for (int i = 1; i <= N; i++)
	{
		for (int j = 1; j <= N; j++)
		{
			_gridVelocity[_INDEX(i, j)].x -= (_gridPressure[_INDEX(i + 1, j)] - _gridPressure[_INDEX(i - 1, j)]) * 0.5f;
			_gridVelocity[_INDEX(i, j)].y -= (_gridPressure[_INDEX(i, j + 1)] - _gridPressure[_INDEX(i, j - 1)]) * 0.5f;
		}
	}

}

void PICFLIP::_updateParticlePos(int iter)
{
	float dt = _timeStep;

	int N = _gridCount - 2;
	for (int i = 0; i < _oldVel.size(); i++)
	{
		_oldVel[i] = _gridVelocity[i] - _oldVel[i];
	}

	// 0.5f is the correct value.
	// But we assign a value of 1.1f to minmax for boundary conditions.
	// By doing this, "the velocity of the boundary" is not affected by the interpolation of the particle velocity.
	float yMax = _gridPosition[_INDEX(0, N + 1)].y - 1.1f;
	float yMin = _gridPosition[_INDEX(0, 0)].y + 1.1f;
	float xMax = _gridPosition[_INDEX(N + 1, 0)].x - 1.1f;
	float xMin = _gridPosition[_INDEX(0, 0)].x + 1.1f;

	for (int i = 0; i < _particlePosition.size(); i++)
	{
		XMFLOAT2 _picVel = _interp->gridToParticle(_particlePosition[i], _gridVelocity, _gridPosition);
		XMFLOAT2 _flipVel = _particleVelocity[i] + _interp->gridToParticle(_particlePosition[i], _oldVel, _gridPosition);

		_particleVelocity[i] = _picVel * (1 - _flipRatio) + _flipVel * _flipRatio;
		_particlePosition[i] += _particleVelocity[i] * dt;

		if (_particlePosition[i].x > xMax) _particlePosition[i].x = xMax;
		else if (_particlePosition[i].x < xMin) _particlePosition[i].x = xMin;

		if (_particlePosition[i].y > yMax) _particlePosition[i].y = yMax;
		else if (_particlePosition[i].y < yMin) _particlePosition[i].y = yMin;
	}
}
