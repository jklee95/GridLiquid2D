#pragma once

// Console window is displayed in debug mode.
#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

#include "EulerianSimulation.h" // This includes Win32App.h


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
    int width = 800;
    int height = 800;

    Win32App winApp(width, height);
    winApp.initialize(hInstance);

    EulerianSimulation* eulersim = new EulerianSimulation();
    eulersim->setGridDomain(11, 20);
    eulersim->initialize();

    DX12App* dxapp = new DX12App();
    dxapp->setProjectionType(PROJ::ORTHOGRAPHIC);
    dxapp->setSimulation(eulersim, 0.1);

    winApp.initDirectX(dxapp);

    return winApp.run();
}