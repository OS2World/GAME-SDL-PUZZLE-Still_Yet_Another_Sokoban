//---------------------------------------------------------------------------
// Games.cpp
//
// Copyright (C) 2002-2003 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------

// Register games in the game registry here.

// To exclude games from the build, just comment out their
// registration entry here.

// See GameRegistry.h for more information.

#include "GameRegistry.h"
#include "Sokoban.h"
#include "Games.h"

CdjItem *AllocateSokoban() { return new CSokoban; }

void RegisterGames()
{
	g_GameRegistry.RegisterGame("CSokoban", "Sokoban", "1.00", "Sokoban", AllocateSokoban);
}

void UnregisterGames()
{
	g_GameRegistry.UnregisterGame("CSokoban");
}
