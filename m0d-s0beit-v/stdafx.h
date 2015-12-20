// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN
//#define __DEBUG

// Windows Header Files:
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <string>
#include <vector>
#include <sstream>
#include <Psapi.h>
#include <iostream>
#include <fstream>
#include <timeapi.h>
#include <time.h>

#pragma comment(lib, "winmm.lib")

extern MODULEINFO g_MainModuleInfo;

// Mine
#include "Log.h"
#include "Pattern.h"

// Rockstar
#include "Types.h"
#include "pgCollection.h"
#include "scrThread.h"
#include "Hashes.h"
#include "VehicleValues.h"
#include "natives.h"
#include "RAGEHelper.h"
#include "DrawHelper.h"
#include "KeyHelper.h"
#include "NetworkHelper.h"
#include "VehicleHelper.h"
#include "StatScripts.h"
#include "RadioScripts.h"
#include "WeaponScripts.h"

// Main
#include "script.h"

void Tick();
void Run();
void RunUnreliable();