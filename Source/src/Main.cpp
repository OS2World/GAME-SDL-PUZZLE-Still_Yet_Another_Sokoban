//---------------------------------------------------------------------------
// Main.cpp
//
// Copyright (C) 2001-2004 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------


#define INCL_DOS
#include <os2.h>


#include "djDesktop.h"
#include "djSettings.h"
#include "GameRegistry.h"
#include "Games.h"
#include <stdlib.h>

void MorphToPM();
int HeartBeat();

#define CONFIG_FILE ".syasokoban1.00"

void MorphToPM()
 {
   PPIB pib;
   PTIB tib;
 
   DosGetInfoBlocks(&tib, &pib);
 
   // Change flag from VIO to PM:
   if (pib->pib_ultype==2) pib->pib_ultype = 3;
 }


int main(int, char**)
{

        MorphToPM(); // Morph the VIO application to a PM one to be able to use Win* functions
	RegisterGames();

	// Check if no games are registered
	if (g_GameRegistry.GetNumGameDescriptors()==0)
	{
		printf("No games registered\n");
		return -2;
	}

	/*
	SDL_Surface *pIcon = SDL_LoadBMP("data/sokoban/sokoban_icon.bmp");
	if (pIcon)
	{
		// FIXME: Crashes
		SDL_WM_SetIcon(pIcon, NULL);
	}
	*/

	char szConfigFile[1024] = {0};
	// Isn't there a better way to do this? e.g. system call. I think this messes up with su'd users.
	char *szHome = getenv("HOME");
	if (szHome)
	{
		strcpy(szConfigFile, szHome);
		strcat(szConfigFile, "/");
	}
	strcat(szConfigFile, CONFIG_FILE);
	
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		char *szRet = SDL_GetError();
		printf("SDL Initialization failed (%s)\n", szRet);
		return -1;
	}

	// Load settings
	g_Settings.LoadSettings(szConfigFile);

	// Initialize desktop
	g_pScreen = new CdjDesktop;
	if (!g_pScreen->Initialize())
		return 0;

	// If only one game registered, skip to that immediately; otherwise,
	// show a menu allowing the user to select a game.
	if (g_GameRegistry.GetNumGameDescriptors()==1)
	{
		// Get the first (and only) game descriptor
		SGameDescriptor GD = g_GameRegistry.GetGameDescriptor(0);
		// Create the default app
		CdjItem *pGame = g_GameRegistry.AllocateGame(GD.szClassName);
		if (!pGame)
			return -1;
		SDL_WM_SetCaption(GD.szWindowTitle, NULL);

		// Expand to fill "desktop" area entirely
		pGame->SetExtents(0, 0, g_pScreen->m_nW, g_pScreen->m_nH);

		/*
		//pGame->SetExtents(20, 40, 800, 600);
		pGame->SetExtents(
			(g_pScreen->m_nW / 2) - (800 / 2),
			(g_pScreen->m_nH / 2) - (600 / 2),
			800,
			600);
		*/

		if (!pGame->Initialize())
			return -1;

		/*
		CdjWindow *pWindow = new CdjWindow;
		pWindow->Initialize();
		pWindow->SetName(pGame->GetName());
		pWindow->AddChild(pGame);
		g_pScreen->AddChild(pWindow);
		*/
		g_pScreen->AddChild(pGame);
	}
	else
	{
		// FIXME: TODO
	}

	// Enter main loop
	HeartBeat();

	// Clean up
	g_pScreen->DestroyAll();
	djDEL(g_pScreen);

	// Save setings
	g_Settings.SaveSettings(szConfigFile);

	//if (pIcon) SDL_FreeSurface(pIcon);

	// Shut down SDL
	SDL_Quit();

	UnregisterGames();
	
	return 0;
}

int HeartBeat()
{
	float fTime = (float)SDL_GetTicks() / 1000.0f;
	float fTimeOld = fTime;
	
	while (g_pScreen->m_bRunning)
	{
		fTime = (float)SDL_GetTicks() / 1000.0f;
		
		// Get mouse pos
		//SDL_GetMouseState(&g_pScreen->m_nMouseX, &g_pScreen->m_nMouseY);
		// Update events
		SDL_Event Event;
		SDLMod ModState;
		ModState = SDL_GetModState();
		while ( SDL_PollEvent(&Event) )
		{
			g_pScreen->HandleEvent(Event, ModState);
		}
		
		// Update
		g_pScreen->UpdateAll(fTime - fTimeOld);

		g_pScreen->DrawAll();

		// draw mouse cursor
		if (g_pScreen->m_bShowCursor)
			djBlit(g_pScreen->GetSurface(), g_pScreen->m_pCursor, g_pScreen->m_nMouseX, g_pScreen->m_nMouseY);
		
		SDL_Flip(g_pScreen->GetSurface());

		//SDL_Delay(1);
		
		fTimeOld = fTime;
	}
	
	return 0;
}
