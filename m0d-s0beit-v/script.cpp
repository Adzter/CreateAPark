#include "stdafx.h"

int freezePlayer[MAX_PLAYERS+1];
Player moneyPlayer = -1;

BlipList* pBlipList;
GtaThread_VTable gGtaThreadOriginal;
GtaThread_VTable gGtaThreadNew;

HANDLE mainFiber;
DWORD wakeAt;

void WAIT(DWORD ms)
{
#ifndef __DEBUG
	wakeAt = timeGetTime() + ms;
	SwitchToFiber(mainFiber);
#endif
}

void RequestControl(Entity e)
{
	NETWORK::NETWORK_REQUEST_CONTROL_OF_ENTITY(e);
	if (!NETWORK::NETWORK_HAS_CONTROL_OF_ENTITY(e))
		WAIT(0);
	NETWORK::NETWORK_REQUEST_CONTROL_OF_ENTITY(e);
}

eThreadState Trampoline(GtaThread* This)
{
	rage::scrThread* runningThread = GetActiveThread();
	SetActiveThread(This);
	#ifdef  __DEBUG
	Run(); //We don't want to also call RunUnlireable, since it's expecting WAIT() to work, which it doesn't in debug mode. #depechemode
	#else
	Tick();
	#endif
	SetActiveThread(runningThread);
	return gGtaThreadOriginal.Run(This);
}

void __stdcall ReliableScriptFunction(LPVOID lpParameter)
{
	try
	{
		while (1)
		{
			Run();
			SwitchToFiber(mainFiber);
		}
	}
	catch (...)
	{
		Log::Fatal("Failed scriptFiber");
	}
}

void __stdcall HeavyWaitFunction(LPVOID lpParameter)
{
	try
	{
		while (1)
		{
			RunUnreliable();
			SwitchToFiber(mainFiber);
		}
	}
	catch (...)
	{
		Log::Fatal("Failed scriptFiber");
	}
}

void Tick()
{
	if (mainFiber == nullptr)
		mainFiber = ConvertThreadToFiber(nullptr);

	static HANDLE reliableFiber;
	if (reliableFiber)
		SwitchToFiber(reliableFiber);
	else
		reliableFiber = CreateFiber(NULL, ReliableScriptFunction, nullptr);

	if (timeGetTime() < wakeAt)
		return;

	static HANDLE scriptFiber;
	if (scriptFiber)
		SwitchToFiber(scriptFiber);
	else
		scriptFiber = CreateFiber(NULL, HeavyWaitFunction, nullptr);
}

void RunUnreliable() //Put functions that don't really need to be run every frame that can cause heavy wait times for the function here.
{
	
}

/*
Create-A-Park

Created by: Adzter
http://steamcommunity.com/id/adzter790

Heavily based off Native Trainer's menu code by:
http://dev-c.com
Alexander Blade

Also makes heavy use of m0d-s0beit-v by
https://bitbucket.org/gir489/m0d-s0beit1-v-redux
gir489: Project lead/Lead developer
s0biet: Original project designer and developer.

*/

/*
F8					activate
NUM2/8/4/6			navigate thru the menus and lists (numlock must be on)
NUM5 				select
NUM0/BACKSPACE/F9 	back
*/

#include "script.h"

#include <string>
#include <ctime>
#include <vector>

#pragma warning(disable : 4244 4305) // double <-> float conversions


bool trainer_switch_pressed()
{
	return IsKeyPressed(VK_F8);
}

void get_button_state(bool *a, bool *b, bool *up, bool *down, bool *l, bool *r)
{
	if (a) *a = IsKeyPressed(VK_NUMPAD5);
	if (b) *b = IsKeyPressed(VK_NUMPAD0) || trainer_switch_pressed() || IsKeyPressed(VK_BACK);
	if (up) *up = IsKeyPressed(VK_NUMPAD8);
	if (down) *down = IsKeyPressed(VK_NUMPAD2);
	if (r) *r = IsKeyPressed(VK_NUMPAD6);
	if (l) *l = IsKeyPressed(VK_NUMPAD4);
}

void menu_beep()
{
	AUDIO::PLAY_SOUND_FRONTEND(-1, "NAV_UP_DOWN", "HUD_FRONTEND_DEFAULT_SOUNDSET", 0);
}

std::string statusText;
DWORD statusTextDrawTicksMax;
bool statusTextGxtEntry;

void update_status_text()
{
	if (GetTickCount() < statusTextDrawTicksMax)
	{
		UI::SET_TEXT_FONT(0);
		UI::SET_TEXT_SCALE(0.55, 0.55);
		UI::SET_TEXT_COLOUR(255, 255, 255, 255);
		UI::SET_TEXT_WRAP(0.0, 1.0);
		UI::SET_TEXT_CENTRE(1);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
		if (statusTextGxtEntry)
		{
			UI::_SET_TEXT_ENTRY((char *)statusText.c_str());
		}
		else
		{
			UI::_SET_TEXT_ENTRY("STRING");
			UI::_ADD_TEXT_COMPONENT_STRING((char *)statusText.c_str());
		}
		UI::_DRAW_TEXT(0.5, 0.5);
	}
}

void set_status_text(std::string str, DWORD time = 2500, bool isGxtEntry = false)
{
	statusText = str;
	statusTextDrawTicksMax = GetTickCount() + time;
	statusTextGxtEntry = isGxtEntry;
}

// Variables
Object lastSpawnedObject;
float moveMultiplier = 1.0;
float rotMultiplier = 1.0;
bool moveIsActive;
std::vector<Object> objectList;
DWORD instaRampLastPressed = GetTickCount();

Object simpleSpawnObject(char *mdl, float x, float y, float z)
{
	// Get the model's hash value
	DWORD model = GAMEPLAY::GET_HASH_KEY(mdl);

	// Get the model working in multiplayer, courtesy of:
	// https://www.reddit.com/r/Gta5Modding/comments/372yka/simple_coding_snippets_c/crk76n0
	if (STREAMING::IS_MODEL_VALID(model)) {
		STREAMING::REQUEST_MODEL(model);

		// Make sure the model has loaded
		while (!STREAMING::HAS_MODEL_LOADED(model)) {
			WAIT(0);
		}

		// Return the object
		return OBJECT::CREATE_OBJECT(model, x, y, z, 1, 1, 1);
	}

	// Because C++ complains about all routes not having a return
	return NULL;
}

void deleteLastObject()
{
	// Make sure we actually have something to delete and not an empty vector
	if (lastSpawnedObject && (!objectList.empty()))
	{
		// Delete the object
		OBJECT::DELETE_OBJECT(&objectList.back());

		if (objectList.size() == 1)
		{
			// Remove it from the vector and since this is the last element, empty the vector
			objectList.clear();

			// Clear the lastSpawnedObject variable
			lastSpawnedObject = NULL;
		}
		else
		{
			// Remove it from the vector
			objectList.pop_back();

			// Set the last spawned object to the previous one in the vector
			lastSpawnedObject = objectList.back();
		}
	}
}

void deleteAllObjects()
{
	// Loop through the vector
	for (int i = 0; i < objectList.size(); i++)
	{
		// Delete each object
		OBJECT::DELETE_OBJECT(&objectList[i]);
	}

	// Reset the vector/lastSpawnedObject
	objectList.clear();
	lastSpawnedObject = NULL;
}

// Updates features frame by frame
void update_features()
{
	update_status_text();

	// common variables
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	// Check for the insta ramp key (0x54 = letter T)
	if (IsKeyPressed(0x54))
	{
		// We need the direction, position and rotation
		Vector3 dir = ENTITY::GET_ENTITY_FORWARD_VECTOR(playerPed);
		Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), false);
		float rot = (ENTITY::GET_ENTITY_ROTATION(playerPed, 0)).z;

		// Spawn a ramp infront of the player then delete it afterwards
		Object o = simpleSpawnObject("prop_mp_ramp_01", playerPos.x + (dir.x * 10.0f), playerPos.y + (dir.y * 10.0f), playerPos.z - 1.0f);
		
		// Set the rotation of the object just created relative to the player
		ENTITY::SET_ENTITY_ROTATION(o, 0, 0, rot, 1, 1);
	}

	// Check if we have an object that was last spawned
	if (lastSpawnedObject && moveIsActive)
	{
		// Store this inside a variable here rather than repeat it down below
		Vector3 rampRotation = ENTITY::GET_ENTITY_ROTATION(lastSpawnedObject, 2);

		///////////////////////////////////
		//	 CHANGING THE RAMP ANGLES	 //
		///////////////////////////////////

		// Rotate clockwise
		if (IsKeyPressed(VK_DIVIDE))
		{
			ENTITY::SET_ENTITY_ROTATION(lastSpawnedObject, rampRotation.x, rampRotation.y, rampRotation.z + 1, 1, 1);
		}

		// Rotate counter clockwise
		if (IsKeyPressed(VK_MULTIPLY))
		{
			ENTITY::SET_ENTITY_ROTATION(lastSpawnedObject, rampRotation.x, rampRotation.y, rampRotation.z - 1, 1, 1);
		}

		// Forwards
		if (IsKeyPressed(VK_ADD))
		{
			ENTITY::SET_ENTITY_ROTATION(lastSpawnedObject, rampRotation.x + 1, rampRotation.y, rampRotation.z, 1, 1);
		}

		// Backwards
		if (IsKeyPressed(VK_SUBTRACT))
		{
			ENTITY::SET_ENTITY_ROTATION(lastSpawnedObject, rampRotation.x - 1, rampRotation.y, rampRotation.z, 1, 1);
		}

		// Store this inside a variable here rather than repeat it down below
		Vector3 rampPosition = ENTITY::GET_ENTITY_COORDS(lastSpawnedObject, 1);

		///////////////////////////////////
		//	CHANGING THE RAMP POSITION	 //
		///////////////////////////////////

		// Up
		if (IsKeyPressed(VK_NUMPAD9))
		{
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(lastSpawnedObject, rampPosition.x, rampPosition.y, rampPosition.z + 0.1, 0, 0, 0);
		}

		// Down
		if (IsKeyPressed(VK_NUMPAD3))
		{
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(lastSpawnedObject, rampPosition.x, rampPosition.y, rampPosition.z - 0.1, 0, 0, 0);
		}

		// Forwards
		if (IsKeyPressed(VK_NUMPAD8))
		{
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(lastSpawnedObject, rampPosition.x, rampPosition.y + 0.1, rampPosition.z, 0, 0, 0);
		}

		// Backwards
		if (IsKeyPressed(VK_NUMPAD2))
		{
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(lastSpawnedObject, rampPosition.x, rampPosition.y - 0.1, rampPosition.z, 0, 0, 0);
		}

		// Left
		if (IsKeyPressed(VK_NUMPAD4))
		{
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(lastSpawnedObject, rampPosition.x - 0.1, rampPosition.y, rampPosition.z, 0, 0, 0);
		}
		// Right
		if (IsKeyPressed(VK_NUMPAD6))
		{
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(lastSpawnedObject, rampPosition.x + 0.1, rampPosition.y, rampPosition.z, 0, 0, 0);
		}
	}
}

std::string line_as_str(std::string text, bool *pState)
{
	while (text.size() < 18) text += " ";
	return text + (pState ? (*pState ? " [ON]" : " [OFF]") : "");
}


int activeLineIndexSpawn = 0;
void process_spawn_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 7;

	std::string caption = "Spawn objects";

	static struct {
		LPCSTR		text;
		char		*rampName;
	} lines[lineCount] = {
		{ "Ramp 1", "prop_mp_ramp_01" },
		{ "Ramp 2", "prop_mp_ramp_02" },
		{ "Ramp 3", "prop_mp_ramp_03" },
		{ "Water ramp 1", "prop_water_ramp_02" },
		{ "Water ramp 2", "prop_water_ramp_03" },
		{ "Shipping container 1", "prop_container_01mb" },
		{ "Shipping container 2", "prop_container_03mb" },
	};


	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		do

		{
			// draw menu
			draw_menu_line(caption, lineWidth, 15.0, 18.0, 0.0, 5.0, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexSpawn)
					draw_menu_line(lines[i].text, lineWidth, 9.0, 60.0 + i * 36.0, 0.0, 9.0, false, false);
					draw_menu_line(lines[activeLineIndexSpawn].text, lineWidth + 1.0, 11.0, 56.0 + activeLineIndexSpawn * 36.0, 0.0, 7.0, true, false);

			update_features();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		// process buttons
		bool bSelect, bBack, bUp, bDown;
		get_button_state(&bSelect, &bBack, &bUp, &bDown, NULL, NULL);
		if (bSelect)
		{
			menu_beep();

			// Calculate the player's co-ordinates
			Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), false);

			// Spawn the object and store it inside a variable, remember to offset the x/y/z so it doesn't spawn inside the player
			Object o = simpleSpawnObject(lines[activeLineIndexSpawn].rampName, playerPos.x + 5, playerPos.y, playerPos.z - 1);

			// Rather than it jumping up when the player touches it, do it now instead
			OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(o);

			// Set this object as the last spawned object
			lastSpawnedObject = o;

			// Add the object to the end of the vector
			objectList.push_back(o);

			waitTime = 200;
		}
		else
			if (bBack || trainer_switch_pressed())
			{
				menu_beep();
				break;
			}
			else
				if (bUp)
				{
					menu_beep();
					if (activeLineIndexSpawn == 0)
						activeLineIndexSpawn = lineCount;
					activeLineIndexSpawn--;
					waitTime = 150;
				}
				else
					if (bDown)
					{
						menu_beep();
						activeLineIndexSpawn++;
						if (activeLineIndexSpawn == lineCount)
							activeLineIndexSpawn = 0;
						waitTime = 150;
					}
	}
}

int activeLineIndexMove = 0;
void process_move_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 1;

	std::string caption = "Move options";

	static struct {
		LPCSTR		text;
		bool		*pState;
		bool		*pUpdated;
	} lines[lineCount] = {
		{ "Toggle move/rotate", &moveIsActive, NULL },
	};


	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, 15.0, 18.0, 0.0, 5.0, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexMove)
					draw_menu_line(line_as_str(lines[i].text, lines[i].pState),
					lineWidth, 9.0, 60.0 + i * 36.0, 0.0, 9.0, false, false);
			draw_menu_line(line_as_str(lines[activeLineIndexMove].text, lines[activeLineIndexMove].pState),
				lineWidth + 1.0, 11.0, 56.0 + activeLineIndexMove * 36.0, 0.0, 7.0, true, false);

			update_features();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		// process buttons
		bool bSelect, bBack, bUp, bDown;
		get_button_state(&bSelect, &bBack, &bUp, &bDown, NULL, NULL);
		if (bSelect)
		{
			menu_beep();
			if (lines[activeLineIndexMove].pState)
				*lines[activeLineIndexMove].pState = !(*lines[activeLineIndexMove].pState);
			if (lines[activeLineIndexMove].pUpdated)
				*lines[activeLineIndexMove].pUpdated = true;
			waitTime = 200;
		}
		else
			if (bBack || trainer_switch_pressed())
			{
				menu_beep();
				break;
			}
			else
				if (bUp && !moveIsActive)
				{
					menu_beep();
					if (activeLineIndexMove == 0)
						activeLineIndexMove = lineCount;
					activeLineIndexMove--;
					waitTime = 150;
				}
				else
					if (bDown && !moveIsActive)
					{
						menu_beep();
						activeLineIndexMove++;
						if (activeLineIndexMove == lineCount)
							activeLineIndexMove = 0;
						waitTime = 150;
					}
	}
}

int activeLineIndexDelete = 0;
void process_delete_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 2;

	std::string caption = "Delete";

	static struct {
		LPCSTR		text;
		bool		*pState;
		bool		*pUpdated;
	} lines[lineCount] = {
		{ "Delete last", NULL, NULL },
		{ "Delete all", NULL, NULL }
	};


	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, 15.0, 18.0, 0.0, 5.0, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexDelete)
					draw_menu_line(line_as_str(lines[i].text, lines[i].pState),
					lineWidth, 9.0, 60.0 + i * 36.0, 0.0, 9.0, false, false);
			draw_menu_line(line_as_str(lines[activeLineIndexDelete].text, lines[activeLineIndexDelete].pState),
				lineWidth + 1.0, 11.0, 56.0 + activeLineIndexDelete * 36.0, 0.0, 7.0, true, false);

			update_features();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		// process buttons
		bool bSelect, bBack, bUp, bDown;
		get_button_state(&bSelect, &bBack, &bUp, &bDown, NULL, NULL);
		if (bSelect)
		{
			menu_beep();
			switch (activeLineIndexDelete)
			{
				// next radio track
			case 0:
				deleteLastObject();
				break;
			case 1:
				deleteAllObjects();
				break;
			}
			waitTime = 200;
		}
		else
			if (bBack || trainer_switch_pressed())
			{
				menu_beep();
				break;
			}
			else
				if (bUp)
				{
					menu_beep();
					if (activeLineIndexDelete == 0)
						activeLineIndexDelete = lineCount;
					activeLineIndexDelete--;
					waitTime = 150;
				}
				else
					if (bDown)
					{
						menu_beep();
						activeLineIndexDelete++;
						if (activeLineIndexDelete == lineCount)
							activeLineIndexDelete = 0;
						waitTime = 150;
					}
	}
}

void process_save_menu()
{
}

void process_load_menu()
{

}

int activeLineIndexMain = 0;
void process_main_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 5;

	std::string caption = "Create A Park (Adzter)";

	static LPCSTR lineCaption[lineCount] = {
		"Spawn",
		"Move",
		"Delete",
		"Save",
		"Load",
	};

	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, 15.0, 18.0, 0.0, 5.0, false, true);
			for (int i = 0; i < (lineCount-1); i++)
				if (i != activeLineIndexMain)
					draw_menu_line(lineCaption[i], lineWidth, 9.0, 60.0 + i*36.0, 0.0, 9.0, false, false);
					draw_menu_line(lineCaption[activeLineIndexMain], lineWidth + 1.0, 11.0, 56.0 + activeLineIndexMain * 36.0, 0.0, 7.0, true, false);

			update_features();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		// process buttons
		bool bSelect, bBack, bUp, bDown;
		get_button_state(&bSelect, &bBack, &bUp, &bDown, NULL, NULL);
		if (bSelect)
		{
			menu_beep();
			switch (activeLineIndexMain)
			{
			case 0:
				process_spawn_menu();
				break;
			case 1:
				process_move_menu();
				break;
			case 2:
				process_delete_menu();
				break;
			case 3:
				process_save_menu();
				break;
			case 4:
				process_load_menu();
				break;
			}
			waitTime = 200;
		}
		else
			if (bBack || trainer_switch_pressed())
			{
				menu_beep();
				break;
			}
			else
				if (bUp)
				{
					menu_beep();
					if (activeLineIndexMain == 0)
						activeLineIndexMain = lineCount;
					activeLineIndexMain--;
					waitTime = 150;
				}
				else
					if (bDown)
					{
						menu_beep();
						activeLineIndexMain++;
						if (activeLineIndexMain == lineCount)
							activeLineIndexMain = 0;
						waitTime = 150;
					}
	}
}

void Run() //Only call WAIT(0) here. The Tick() function will ignore wakeAt and call this again regardless of the specified wakeAt time.
{
	#ifdef __DEBUG
	static bool bQuit, F12 = false;
	if (isKeyPressedOnce(F12, VK_F12)){ bQuit = true; }
	if (bQuit) { return; }
	#endif

	while (true)
	{
		if (trainer_switch_pressed())
		{
			menu_beep();
			process_main_menu();
		}

		update_features();
		WAIT(0);
	}

	return;
}

bool AttemptScriptHook()
{
	rage::pgPtrCollection<GtaThread>* threadCollection = GetGtaThreadCollection(pBlipList);

	if (!threadCollection) {
		return false;
	}

	for (UINT16 i = 0; i < threadCollection->count(); i++) {
		GtaThread* pThread = threadCollection->at(i);

		if (!pThread)
			continue;

		//s0biet originally had some junk thread that was called for like 2 seconds then died. This thread is better.
		if (pThread->GetContext()->ScriptHash != MAIN_PERSISTENT) {
			continue;
		}

		// Now what? We need to find a target thread and hook its "Tick" function
		if (gGtaThreadOriginal.Deconstructor == NULL) {
			memcpy(&gGtaThreadOriginal, (DWORD64*)((DWORD64*)pThread)[0], sizeof(gGtaThreadOriginal)); //Create a backup of the original table so we can call the original functions from our hook.
			memcpy(&gGtaThreadNew, &gGtaThreadOriginal, sizeof(GtaThread_VTable)); //Construct our VMT replacement table.

			gGtaThreadNew.Run = Trampoline; //Replace the .Run method in the new table with our method.
		}

		if (((DWORD64*)pThread)[0] != (DWORD64)&gGtaThreadNew) { //If the table is not VMT Hooked.
			DEBUGOUT("Hooking thread [%i] (0x%X)", pThread->GetContext()->ThreadId, pThread->GetContext()->ScriptHash);
			((DWORD64*)pThread)[0] = (DWORD64)&gGtaThreadNew; //Replace the VMT pointer with a pointer to our new VMT.
			DEBUGOUT("Hooked thread [%i] (0x%X)", pThread->GetContext()->ThreadId, pThread->GetContext()->ScriptHash);
			return true;
		}
	}
	return false;
}

DWORD WINAPI lpHookScript(LPVOID lpParam) {
	while (!AttemptScriptHook()) {
		Sleep(100);
	}

	return 0; //We no longer need the lpHookScript thread because our Trampoline function will now be the hip and or hop hang out spot for the KewlKidzKlub®.
}

void SpawnScriptHook() {
	CreateThread(0, 0, lpHookScript, 0, 0, 0);
}