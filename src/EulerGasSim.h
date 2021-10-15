#pragma once
#include "GridFluidSim.h"

class EulerGasSim : public GridFluidSim
{
public:
	EulerGasSim(GridIndex& index);
	~EulerGasSim() override;

private:
	void _update() override;
	void _force();
	void _advect();
	void _project();

	void _updateParticlePos() override;
};

