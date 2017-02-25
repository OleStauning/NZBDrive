#include "INZBDrive.hpp"
#include "NZBDrive.hpp"
#include "Logger.hpp"
#include <memory>

extern "C" __declspec(dllexport) ByteFountain::INZBDrive* __cdecl CreateNZBDrive()
{
	return new ByteFountain::NZBDrive();
}

extern "C" __declspec(dllexport) void __cdecl ReleaseNZBDrive(ByteFountain::INZBDrive* drive)
{
	delete drive;
}
