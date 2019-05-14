//---------------------------------------------------------------------------
// Sokoban.cpp
//
// Copyright (C) 2001-2004 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------

#include "SDL.h"
#include "Sokoban.h"
//#include "djButton.h"
#include "djDesktop.h"
#include "djSettings.h"

#define SETTING_LEVELSCOMPLETED	"Sokoban1.00-LevelsCompleted"
#define SETTING_SHADOWS			"Sokoban1.00-Shadows"


// Time between auto-moves on mouse click (seconds)
// This stupid underscore is in there because Microsoft VC++ 6 compiler is getting
// all fscking stupid confused about it otherwise, for some unknown reason
const float AUTO_FOLLOWMOVETIME = 0.028f; // <-- I made this slightly faster in SYASokoban 2

const float PUSH_SPRITE_TIME = 0.25f;

// This value merely needs to be greater than the longest possible path length
// making it the number of squares in the array seems a good bet.
#define BADMOVE (128*128)

CSokoban::CSokoban()
{
	g_Settings.SetDefaultSetting(SETTING_LEVELSCOMPLETED, "");
	g_Settings.SetDefaultSettingBool(SETTING_SHADOWS, true);
	m_pSokobanHeader = NULL;
	m_pAbout = NULL;
	m_iNumMoves = 0;
	m_iNumPushes = 0;
	m_bShadows = g_Settings.GetSettingBool(SETTING_SHADOWS);
	m_nLevelW = -1;
	m_nLevelH = -1;
	m_nHeroX = -1;
	m_nHeroY = -1;
	m_nHeroDirection = 0;
	m_bWon = false;
	m_bGotMouseClick = false;
	m_iCurLev = -1;
	m_nMovesTaken = -1;
	m_bMayPickCrates = true;
	m_nCrateX = -1;
	m_nCrateY = -1;
	m_fPushedTimer = 0;
	m_pCurSet = NULL;
	m_bShowingAbout = false;
	m_pTileSet = NULL;
	m_nMouseX = 0;
	m_nMouseY = 0;
	m_pLevelManager = NULL;
	ClearAutoPath();
}

CSokoban::~CSokoban()
{
	djASSERT(!m_pLevelManager); // Must be freed in Destroy()
}

void CSokoban::ClearAutoPath()
{
	for ( int i=0; i<128; i++ )
	{
		for ( int j=0; j<128; j++ )
		{
			m_FindMap[j][i] = BADMOVE;
		}
	}
	m_aAutoPath.clear();
	m_bBusyFollowingAutoPath = false;
	m_fAutoPathFollowTimer = 0;
}

void CSokoban::FindTarget(int nX, int nY, int nPathLen)
{
	if (nX<0 || nY<0 || nX>=m_nLevelW || nY>=m_nLevelH)
		return;
	if (m_CurLevel[nX][nY]!=sokBLANK && m_CurLevel[nX][nY]!=sokCRATEDEST)
		return;
	if(m_FindMap[nX][nY] <= nPathLen)
		return;
	
	m_FindMap[nX][nY] = nPathLen++;
	
	if((nX == m_nHeroX) && (nY == m_nHeroY))
		return;
	
	FindTarget(nX - 1, nY, nPathLen);
	FindTarget(nX + 1, nY, nPathLen);
	FindTarget(nX, nY - 1, nPathLen);
	FindTarget(nX, nY + 1, nPathLen);
}

bool CSokoban::Initialize()
{
	SDL_EnableKeyRepeat(250, SDL_DEFAULT_REPEAT_INTERVAL - 5);

	// Load default sprites
	LoadTileSets("default");
	if (m_apTileSets.size()==0)
		return false;

	// Load "Sokoban" header bitmap
	SDL_Surface *pSurface = SDL_LoadBMP("data/sokoban/themes/default/sokoban.bmp");
	if (pSurface)
	{
		m_pSokobanHeader = SDL_DisplayFormat(pSurface);
		SDL_FreeSurface(pSurface);
	}
	
	// Initialize levelset manager
	m_pLevelManager = new CLevelManager;

	// Load default levelset
	LoadLevelset("data/sokoban/levels/default.txt");
	m_pLevelManager->AddLevelSet(
		"Default",
		"-",
		"-",
		"-",
		"-",
		"data/sokoban/levels/default.txt",
		"data/sokoban/levels/default.desc");

	// Create some stupid test buttons
	/*
	CdjButton *pButton = new CdjButton;
	pButton->SetName("Button1");
	pButton->Initialize();
	pButton->SetText("A1", "A2", "A3");
	pButton->SetExtents(m_nW - 200 - 1, 32, 200, 32);
	AddChild(pButton);
	
	CdjButton *pButton2 = new CdjButton;
	pButton2->SetName("Button2");
	pButton2->Initialize();
	pButton2->SetText("B1", "B2", "B3");
	pButton2->SetExtents(10, 4, 80, 32);
	pButton->AddChild(pButton2);
	//*/

	SetName("Sokoban");
	
	return true;
}

bool CSokoban::Destroy()
{
	djLog("CSokoban::Destroy()");
	djDEL(m_pCurSet);
	djDEL(m_pLevelManager);
	if (m_pSokobanHeader) SDL_FreeSurface(m_pSokobanHeader);
	m_pSokobanHeader = NULL;
	ClearTileSets();
	return true;
}

bool CSokoban::Update(float dt)
{
	// Have we won?
	if (m_bWon)
	{
		ClearAutoPath();
		if (m_bGotMouseClick)
		{
			SetLevel((m_iCurLev + 1) % m_pCurSet->apLevels.size());
			//return true;
		}
	}

	// About box?
	if (m_bShowingAbout)
	{
		if (!m_pAbout)
			m_bShowingAbout = false;
		return true;
	}
	if (!m_pTileSet)
		return false;

	int nSpriteSize = m_pTileSet->nW;

	bool bLeftMouseDown = (g_pScreen->m_nMouseButtons & 1)!=0;
	if (!m_bBusyFollowingAutoPath)
	{
		m_bMayPickCrates = true;
	}
	if (!bLeftMouseDown)
	{
		m_nCrateX = -1;
		m_nCrateY = -1;
	}
	// If the left mouse button is down, and we have a possible auto-trail to follow, follow it.
	if (!m_bWon && m_bMouseInside && bLeftMouseDown)
	{
		if (m_nCrateX!=-1 && m_nCrateY!=-1) // Crate pick?
		{

		}
		else
		{
			if (!m_bBusyFollowingAutoPath)
			{
				if (m_aAutoPath.size()>0)
				{
					m_bMayPickCrates = false;
					m_bBusyFollowingAutoPath = true;
					m_fAutoPathFollowTimer = AUTO_FOLLOWMOVETIME;
				}
				else
					m_nHeroDirection = 0;
			}
		}
	}

	if (m_bBusyFollowingAutoPath)
	{
		m_fAutoPathFollowTimer -= dt;
		while (m_fAutoPathFollowTimer<0 && m_bBusyFollowingAutoPath)
		{
			m_fAutoPathFollowTimer += AUTO_FOLLOWMOVETIME;
			TryMove(m_aAutoPath[0].dx, m_aAutoPath[0].dy);
			if (m_aAutoPath.size()>0)
				m_aAutoPath.erase(m_aAutoPath.begin());
			if (m_aAutoPath.size()==0)
			{
				ClearAutoPath();
				m_nHeroDirection = 0;
			}
		}
	}

	if (m_pCurSet && m_iCurLev>=0)
	{
		int iOffsetX = ((m_nW - 140) / 2) - ((m_nLevelW*nSpriteSize) / 2);
		int iOffsetY = (m_nH / 2) - ((m_nLevelH*nSpriteSize) / 2);
		if (iOffsetX<0) iOffsetX = 0;
		if (iOffsetY<0) iOffsetY = 0;

		if (!m_bWon && m_bMouseInside
			&& m_nMouseX>=iOffsetX && m_nMouseY>=iOffsetY
			&& m_nMouseX<iOffsetX+m_nLevelW*nSpriteSize && m_nMouseY<iOffsetY+m_nLevelH*nSpriteSize)
		{
			int nBlockX = (m_nMouseX - iOffsetX) / nSpriteSize;
			int nBlockY = (m_nMouseY - iOffsetY) / nSpriteSize;
			bool bMouseMovedToNewBlockOrHeroMoved = (nBlockX!=m_nTargetX || nBlockY!=m_nTargetY);
			m_nTargetX = nBlockX;
			m_nTargetY = nBlockY;

			// If mouse is over a crate, and we're not using the mouse to move hero
			if (m_bMayPickCrates && IsCrate(m_CurLevel[nBlockX][nBlockY]))
			{
				ClearAutoPath();
				// Determine if crate is next to hero
				if ((m_nHeroX==nBlockX && djABS(nBlockY-m_nHeroY)==1) || (m_nHeroY==nBlockY && djABS(nBlockX-m_nHeroX)==1))
				{
					// If left button down and no crate currently selected, select this one.
					if (bLeftMouseDown && m_nCrateX==-1 && m_nCrateY==-1)
					{
						m_nCrateX = nBlockX;
						m_nCrateY = nBlockY;
					}
				}
			}
			else
			{
				// Mouse moved to new block; recalculate shortest path
				if (!m_bBusyFollowingAutoPath && bMouseMovedToNewBlockOrHeroMoved)
				{
					ClearAutoPath();
					// Build an auto-move path from whatever we've just found, if possible.
					FindTarget(m_nTargetX, m_nTargetY, 0);
					if (m_FindMap[m_nHeroX][m_nHeroY]!=BADMOVE)
					{
						// We made it back, so let's walk the path we just built up
						int cx = m_nHeroX;
						int cy = m_nHeroY;
						while (m_FindMap[cx][cy])
						{
							SAutoMove Move;
							if (m_FindMap[cx - 1][cy] == (m_FindMap[cx][cy] - 1))
							{
								Move.dx = -1; Move.dy = 0; m_aAutoPath.push_back(Move);
								cx--;
							} else if (m_FindMap[cx + 1][cy] == (m_FindMap[cx][cy] - 1))
							{
								Move.dx = 1; Move.dy = 0; m_aAutoPath.push_back(Move);
								cx++;
							} else if (m_FindMap[cx][cy - 1] == (m_FindMap[cx][cy] - 1))
							{
								Move.dx = 0; Move.dy = -1; m_aAutoPath.push_back(Move);
								cy--;
							} else if (m_FindMap[cx][cy + 1] == (m_FindMap[cx][cy] - 1))
							{
								Move.dx = 0; Move.dy = 1; m_aAutoPath.push_back(Move);
								cy++;
							} else
							{
								djASSERT(0); // Shouldn't happen
							}
						}
					}
				}
			}

			// If we have a selected crate ..
			if (m_nCrateX!=-1 && m_nCrateY!=-1)
			{
				// Check if user has tried to "drag" the crate
				if (nBlockX!=m_nCrateX || nBlockY!=m_nCrateY)
				{
					// Get total diff X and Y
					int nDiffX = nBlockX - m_nCrateX;
					int nDiffY = nBlockY - m_nCrateY;
					// Can't move diagonally, at least one axis diff must therefore be 0.
					if (nDiffX==0 || nDiffY==0)
					{
						// Get diff X and Y *direction*
						int nDX = djMIN(1, djMAX(-1, nDiffX));
						int nDY = djMIN(1, djMAX(-1, nDiffY));
						bool bCanMove = true;
						// Check if this "drag" movement is possible, if so, update hero position also.
						// We stop if we physically can't do the movement, or if the crate has reached the desired position.
						while (bCanMove && (m_nCrateX!=nBlockX || m_nCrateY!=nBlockY))
						{
							int nHeroX = m_nHeroX;
							int nHeroY = m_nHeroY;
							// See if hero is on opposite side of crate to mouse position
							if (m_nHeroX+nDX==m_nCrateX && m_nHeroY+nDY==m_nCrateY)
							{
								// If so, do a movement of hero. This will handle the crate-pushing for us.
								TryMove(nDX, nDY);
							}
							// Determine if we moved succesfully ..
							if (nHeroX==m_nHeroX && nHeroY==m_nHeroY)
							{
								bCanMove = false;
							}
							else
							{
								// Update crate selection to new pos
								m_nCrateX = m_nHeroX + nDX;
								m_nCrateY = m_nHeroY + nDY;
							}
						}
					}
				}
			}
		}
		else
		{
			m_nTargetX = -1;
			m_nTargetY = -1;
			// Mouse outside of map area, clear auto-path
			if (!m_bBusyFollowingAutoPath)
				ClearAutoPath();
		}
	}

	if (m_fPushedTimer>0)
		m_fPushedTimer -= dt;

	return true;
}

void CSokoban::Draw(SDL_Surface *pSurface, int nX, int nY)
{
	CdjFont *pFont = g_pScreen->m_pFont8x13;

	// Clear background (NOW DONE BY DESKTOP)
	//djDrawFilledRect(pSurface, nX, nY, m_nW, m_nH, SDL_MapRGB(pSurface->format,0,0,0));

	// About box?
	if (m_bShowingAbout)
	{
		if (m_pAbout)
			djBlit(pSurface, m_pAbout, m_nW / 2 - 320/2, m_nH / 2 - 240/2);
		return;
	}

	// Tileset error?
	if (!m_pTileSet)
	{
		g_pScreen->m_pFont8x13->DrawText(pSurface, nX, nY, "ERROR: NO TILESET");
		return;
	}

	int nSpriteSize = m_pTileSet->nW;
	SDL_Surface *pSprites = m_pTileSet->pSprites;

	// Draw level
	if (m_pCurSet && m_iCurLev>=0)
	{
		int iOffsetX, iOffsetY, i, j;
		// Center level to screen
		iOffsetX = ((m_nW - 140) / 2) - ((m_nLevelW*nSpriteSize) / 2);
		iOffsetY = (m_nH / 2) - ((m_nLevelH*nSpriteSize) / 2);
		if (iOffsetX<0) iOffsetX = 0;
		if (iOffsetY<0) iOffsetY = 0;
		// Draw level blocks
		int nBorderWidth = nSpriteSize/2;

		// Level border
		djDrawSprite(0, pSurface, m_pTileSet->pBorders, nX+iOffsetX-nBorderWidth, nY+iOffsetY-nBorderWidth, nBorderWidth, nBorderWidth);
		djDrawSprite(1, pSurface, m_pTileSet->pBorders, nX+iOffsetX+m_nLevelW*nSpriteSize, nY+iOffsetY-nBorderWidth, nBorderWidth, nBorderWidth);
		djDrawSprite(5, pSurface, m_pTileSet->pBorders, nX+iOffsetX+m_nLevelW*nSpriteSize, nY+iOffsetY+m_nLevelH*nSpriteSize, nBorderWidth, nBorderWidth);
		djDrawSprite(4, pSurface, m_pTileSet->pBorders, nX+iOffsetX-nBorderWidth, nY+iOffsetY+m_nLevelH*nSpriteSize, nBorderWidth, nBorderWidth);
		for ( i=0; i<m_nLevelW; i++ )
		{
			djDrawSprite(1, pSurface, m_pTileSet->pBorders, nX+iOffsetX+i*nSpriteSize, nY+iOffsetY-nBorderWidth, nSpriteSize, nBorderWidth);
			djDrawSprite(3, pSurface, m_pTileSet->pBorders, nX+iOffsetX+i*nSpriteSize, nY+iOffsetY+m_nLevelH*nSpriteSize, nSpriteSize, nBorderWidth);
		}
		for ( i=0; i<m_nLevelH; i++ )
		{
			djDrawSprite(4, pSurface, m_pTileSet->pBorders, nX+iOffsetX-nBorderWidth, nY+iOffsetY+i*nSpriteSize, nBorderWidth, nSpriteSize);
			djDrawSprite(5, pSurface, m_pTileSet->pBorders, nX+iOffsetX+m_nLevelW*nSpriteSize, nY+iOffsetY+i*nSpriteSize, nBorderWidth, nSpriteSize);
		}
		// Level
		int nShadowBits = 0;
		char cLevChar;
		int nOffsetX;
		int nOffsetY = iOffsetY + nY; // Output X,Y on screen
		for ( i=0; i<m_nLevelH; i++ )
		{
			nOffsetX = iOffsetX + nX;
			for ( j=0; j<m_nLevelW; j++ )
			{
				cLevChar = m_CurLevel[j][i];

				nShadowBits = 0;
				if (m_bShadows)
				{
					// Bit 1: Something above casting shadow on us
					// Bit 2: Something above and to left of us casting shadow on us
					// Bit 4: Something left of us casting shadow on us
					// Thats 0-7 possibilities, but we exclude 7 because the shadow is the same as for 5
					if (i>0 && CastsShadow(m_CurLevel[j][i-1])) nShadowBits |= 1;
					if (j>0 && CastsShadow(m_CurLevel[j-1][i])) nShadowBits |= 4;
					if (nShadowBits!=5 && i>0 && j>0 && CastsShadow(m_CurLevel[j-1][i-1]))
						nShadowBits |= 2;
				}
				switch (cLevChar)
				{
				case sokBLANK:
					djDrawSprite(sokBLANK + nShadowBits, pSurface, pSprites, nOffsetX, nOffsetY, nSpriteSize, nSpriteSize);
					break;
				case sokCRATEDEST:
					djDrawSprite(sokCRATEDEST + nShadowBits, pSurface, pSprites, nOffsetX, nOffsetY, nSpriteSize, nSpriteSize);
					break;
				default:
					djDrawSprite(cLevChar, pSurface, pSprites, nOffsetX, nOffsetY, nSpriteSize, nSpriteSize);
				}
				nOffsetX += nSpriteSize;
			}
			nOffsetY += nSpriteSize;
		}

		// DEBUG: djDrawRect(pSurface, 2+nX+iOffsetX + m_nTargetX*nSpriteSize, 2+nY+iOffsetY + m_nTargetY*nSpriteSize, nSpriteSize-4, nSpriteSize-4, SDL_MapRGB(pSurface->format, 255,120,255));

		// Draw mouse select box thingy
		if (!m_bWon && m_bMouseInside
			&& m_nMouseX>=iOffsetX && m_nMouseY>=iOffsetY
			&& m_nMouseX<iOffsetX+m_nLevelW*nSpriteSize && m_nMouseY<iOffsetY+m_nLevelH*nSpriteSize)
		{
			// If we have a selected crate ..
			if (m_nCrateX!=-1 && m_nCrateY!=-1)
			{
				// Show *selected* crate always if we have one selected
				djDrawRect(pSurface, nX+iOffsetX + m_nCrateX*nSpriteSize, nY+iOffsetY + m_nCrateY*nSpriteSize, nSpriteSize, nSpriteSize, SDL_MapRGB(pSurface->format, 120,120,255));
			}
			else
			{
				int nBlockX = (m_nMouseX - iOffsetX) / nSpriteSize;
				int nBlockY = (m_nMouseY - iOffsetY) / nSpriteSize;

				// If mouse is over a crate, and we're not using the mouse to move hero
				if (m_bMayPickCrates && IsCrate(m_CurLevel[nBlockX][nBlockY]))
				{
					// Determine if crate is next to hero
					if ((m_nHeroX==nBlockX && djABS(nBlockY-m_nHeroY)==1) || (m_nHeroY==nBlockY && djABS(nBlockX-m_nHeroX)==1))
					{
						// Show *highlighted* crate only when mouse over it and it's next to hero
						if (m_nCrateX==-1 && m_nCrateY==-1)
							djDrawRect(pSurface, nX+iOffsetX + nBlockX*nSpriteSize, nY+iOffsetY + nBlockY*nSpriteSize, nSpriteSize, nSpriteSize, SDL_MapRGB(pSurface->format, 70,70,200));
					}
				}
			}
		}

		// Draw auto-path stuff ..
		if (m_aAutoPath.size()>0)
		{
			if (m_bBusyFollowingAutoPath)
			{
				if (m_aAutoPath.size()>3) // <-- Don't show very short paths
				{
					int nPathX = m_nHeroX;
					int nPathY = m_nHeroY;
					for ( UINT i=0; i<m_aAutoPath.size(); i++ )
					{
						int x1 = nX+iOffsetX + nPathX*nSpriteSize + nSpriteSize/2;
						int y1 = nY+iOffsetY + nPathY*nSpriteSize + nSpriteSize/2;
						nPathX += m_aAutoPath[i].dx;
						nPathY += m_aAutoPath[i].dy;
						int x2 = nX+iOffsetX + nPathX*nSpriteSize + nSpriteSize/2;
						int y2 = nY+iOffsetY + nPathY*nSpriteSize + nSpriteSize/2;
						djDrawRect(pSurface,
							djMIN(x1, x2),
							djMIN(y1, y2),
							m_aAutoPath[i].dx!=0 ? nSpriteSize+1 : 2,
							m_aAutoPath[i].dy!=0 ? nSpriteSize+1 : 2,
							m_bBusyFollowingAutoPath ?
							SDL_MapRGB(pSurface->format, djMIN(255, 100+i*25), 255, djMIN(255, 60+i*10))
							:
						SDL_MapRGB(pSurface->format, 0, 20, 0));
					}
				}
			}
			else
			{
				// Show green rectangle at mouse, indicating user can click to move hero there.
				djDrawRect(pSurface, nX+iOffsetX + m_nTargetX*nSpriteSize, nY+iOffsetY + m_nTargetY*nSpriteSize, nSpriteSize, nSpriteSize, SDL_MapRGB(pSurface->format, 80, 255,80));
			}
		}
		
		// Draw hero
		djDrawSprite(m_bWon ? 40 : (m_fPushedTimer>0 ? 40 : 32) + m_nHeroDirection, pSurface, pSprites, nX+iOffsetX+m_nHeroX*nSpriteSize, nY+iOffsetY+m_nHeroY*nSpriteSize, nSpriteSize, nSpriteSize);
	}

	// Sokoban header in top right
	if (m_pSokobanHeader)
		djBlit(pSurface, m_pSokobanHeader, nX + m_nW - 140, nY);

	// Gray 'divider' separating game area and stuff on right
	djDrawFilledRect(pSurface, nX + m_nW - 140, nY, 1, m_nH, SDL_MapRGB(pSurface->format, 60,60,60));

	// Help
	int nDY = pFont->GetH();
	//int nDX = pFont->GetW();
	int nHelpY = nY + m_nH - 10*nDY*2;
	int nHelpX = nX + m_nW - 140 + 8;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "Alt+Enter:");		nHelpY += nDY;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "Fullscreen Mode");	nHelpY += nDY*2;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "M: Next Level");		nHelpY += nDY*2;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "N: Prev Level");		nHelpY += nDY*2;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "R: Reset");			nHelpY += nDY*2;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "U: Undo");			nHelpY += nDY*2;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "S: Shadows");		nHelpY += nDY*2;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "A: About");			nHelpY += nDY*2;
	pFont->DrawText(pSurface, nHelpX, nHelpY, "ESC: Exit");			nHelpY += nDY*2;

	// Draw level/score info
	int nStatusBarHeight = nDY + 10;
	djDrawFilledRect(pSurface, nX, nY+m_nH-nStatusBarHeight  , m_nW, 1, SDL_MapRGB(pSurface->format, 182,182,182));
	djDrawFilledRect(pSurface, nX, nY+m_nH-nStatusBarHeight+1, m_nW, 1, SDL_MapRGB(pSurface->format, 162,162,162));
	djDrawFilledRect(pSurface, nX, nY+m_nH-nStatusBarHeight+2, m_nW, nStatusBarHeight-2, SDL_MapRGB(pSurface->format, 110,110,110));
	char sbuf[1024];
	if (m_pCurSet!=NULL)
	{
		sprintf(sbuf, "Level: %d/%d  Moves: %d  Pushes: %d", m_iCurLev+1, m_pCurSet->apLevels.size(), m_iNumMoves, m_iNumPushes);
		pFont->DrawText(
			pSurface,
			nX + m_nW - pFont->GetStringWidth(sbuf) - 8,
			nY + m_nH - nDY - 4,
			sbuf);
		if (m_nMovesTaken!=-1)
		{
			sprintf(sbuf, "Completed (%d move%s)", m_nMovesTaken, m_nMovesTaken==1 ? "" : "s");
			pFont->DrawText(
				pSurface,
				nX + 8,
				nY + m_nH - nDY - 4,
				sbuf);
		}
	}

	// Just won?
	if (m_bWon && !m_bGotMouseClick)
	{
		strcpy(sbuf, "SPACE or mouse click to continue");
		pFont->DrawText(
			pSurface,
			nX + (m_nW-140)/2 - pFont->GetStringWidth(sbuf)/2,
			nY + m_nH - 24 - pFont->GetH(),
			sbuf);
	}

	// Search for *.desc in data\sokoban\levels


	/*
	for ( UINT i=0; i<m_pLevelManager->GetNumLevelSetDescriptors(); i++ )
	{
		int nFooY = nY;
		CLevelManager::SLevelSet LS = m_pLevelManager->GetLevelSetDescriptor(i);
		sprintf(sbuf, "%d %s %s %s %s %s %s %s",
			i+1,
			LS.sName.c_str(),
			LS.sAuthor.c_str(),
			LS.sURL.c_str(),
			LS.sEmail.c_str(),
			LS.sDifficulty.c_str(),
			LS.sLevelFile.c_str(),
			LS.sDescFile.c_str());
		pFont->DrawText(
			pSurface,
			nX + 10,
			nFooY,
			sbuf);

		nFooY += 13;
	}
	//*/
}

bool CSokoban::LoadLevelset(const char *szFilename)
{
	FILE *fin = fopen(szFilename, "r");
	if (fin==NULL)
		return false;
		
	char buf[1024];

	CLevel *pLev;
	CLevelSet *pSet = new CLevelSet;

	int state = 0; // reading level num
	int ysize = 0;

	int iNumCrates;
	int iNumCrateDests;

	fgets(buf, sizeof(buf), fin);
	buf[strlen(buf)-1] = 0; // strip newline char
	while (!feof(fin))
	{
		int iLevNum;

		switch (state)
		{
		case 0:
			sscanf(buf, "%d", &iLevNum);
			state = 1;
			ysize = 0;
			pLev = new CLevel;
			pLev->iNum = iLevNum;
			pLev->x = -1;
			pLev->y = -1;
			iNumCrates = 0;
			iNumCrateDests = 0;
			break;
		case 1:
			if (buf[0]=='~')
			{
				// Test if bogus level
				if (iNumCrates == iNumCrateDests)
				{
					pSet->apLevels.push_back(pLev);
				}
				state = 0;
			}
			else
			{
				char *szLine = new char[strlen(buf) + 1];
				strcpy(szLine, buf);
				int i;
				// interpret
				for ( i=0; i<(int)strlen(szLine); i++ )
				{
					switch (szLine[i])
					{
					case ' ': szLine[i] = sokBLANK;     break; // blank
					case '#': szLine[i] = sokWALL;      break; // wall
					case '$': szLine[i] = sokCRATE; iNumCrates++; break; // crate
					case '.': szLine[i] = sokCRATEDEST; iNumCrateDests++; break; // crate destination
					case '*': szLine[i] = sokCRATEDONE; iNumCrates++; iNumCrateDests++; break; // crate on crate destination
					case '@': szLine[i] = sokBLANK; pLev->x = i; pLev->y = ysize; break; // startpos
					case  13: szLine[i] = sokBLANK; break;// newline (DOS text file format shit)
					default:
						printf("Warn: unkown char %d\n", (int)szLine[i]);
					}
				}
				pLev->apLines.push_back(szLine);
				ysize++;
			}
			break;
		}

		fgets(buf, sizeof(buf), fin);
		buf[strlen(buf)-1] = 0; // strip newline char
	}

	m_pCurSet = pSet;

	// Skip forward until we reach the first level this user hasn't completed
	int n = 0;
	while (IsLevelCompleted(n) && n<m_pCurSet->apLevels.size()-1)
	{
		n++;
	}
	SetLevel(n);

	fclose(fin);

	return true;
}

int CSokoban::SetLevel(int iLev)
{
	m_nMovesTaken = GetNumMovesTaken(iLev);
	
	m_aUndo.clear();
	m_iNumMoves = 0;
	m_iNumPushes = 0;
	m_iCurLev = iLev;
	CLevel *pLev = m_pCurSet->apLevels[iLev];
	// Clear level to blanks
	int i, j;
	for ( i=0; i<128; i++ )
	{
		for ( j=0; j<128; j++ )
		{
			m_CurLevel[j][i] = sokBLANK;
		}
	}
	m_nLevelH = pLev->apLines.size();
	m_nLevelW = 0;
	for ( i=0; i<(int)pLev->apLines.size(); i++ )
	{
		char *szLine = pLev->apLines[i];
		if ((int)strlen(szLine) > m_nLevelW)
			m_nLevelW = strlen(szLine);
		for ( j=0; j<(int)strlen(szLine); j++ )
		{
			m_CurLevel[j][i] = szLine[j];
		}
	}

	// Resolve correct wall directions
	for ( i=0; i<128; i++ )
	{
		for ( j=0; j<120; j++ )
		{
			if (IsWall(m_CurLevel[j][i]))
			{
				int nBits = 0;
				if (i>=1  && IsWall(m_CurLevel[j][i-1])) nBits |= 1;
				if (j<127 && IsWall(m_CurLevel[j+1][i])) nBits |= 2;
				if (i<127 && IsWall(m_CurLevel[j][i+1])) nBits |= 4;
				if (j>=1  && IsWall(m_CurLevel[j-1][i])) nBits |= 8;
				m_CurLevel[j][i] += nBits;
				// Cracked walls ..
				if (m_CurLevel[j][i]==sokWALL+5 && (rand()%35)==0)
					m_CurLevel[j][i] = sokWALL_CRACKED;
			}
		}
	}

	ClearAutoPath();
	m_bMayPickCrates = true;
	m_nCrateX = -1;
	m_nCrateY = -1;

	m_nHeroX = pLev->x;
	m_nHeroY = pLev->y;
	m_nHeroDirection = 0;
	
	m_fPushedTimer = 0;

	m_bWon = false;
	m_bGotMouseClick = false;

	ChooseTileSet();

	return 0;
}

void CSokoban::MoveHero(int dx, int dy)
{
	m_nHeroX += dx;
	m_nHeroY += dy;
	m_iNumMoves++;

	// If this move was from a keypress, invalidate the auto-path; the shortest
	// path to mouse cursor position will be immediately recalculated.
	if (!m_bBusyFollowingAutoPath)
	{
		m_nTargetX = -1;
		m_nTargetY = -1;
		ClearAutoPath();
	}
}

void CSokoban::SetHeroDirection(int dx, int dy)
{
	if (dx<0)
		m_nHeroDirection = 4;
	else if (dx>0)
		m_nHeroDirection = 2;
	else if (dy<0)
		m_nHeroDirection = 1;
	else if (dy>0)
		m_nHeroDirection = 3;
	else
		m_nHeroDirection = 0;
}

void CSokoban::SetShowingAbout(bool b)
{
	if (!b)
	{
		m_bShowingAbout = false;
		djDEL(m_pAbout);
	}
	else
	{
		SDL_Surface *pSurface = SDL_LoadBMP("data/sokoban/about.bmp");
		if (pSurface)
		{
			m_pAbout = SDL_DisplayFormat(pSurface);
			SDL_FreeSurface(pSurface);
			m_bShowingAbout = true;
		}
	}
}

bool CSokoban::OnMouseButtonDown(int nButton)
{
	if (nButton==SDL_BUTTON_LEFT)
	{
		if (m_bShowingAbout)
		{
			SetShowingAbout(false);
		}
		else
		{
			if (m_bWon && !m_bGotMouseClick)
				m_bGotMouseClick = true;
		}
	}
	return true;
}

bool CSokoban::OnMouseMove(int nX, int nY, int nDX, int nDY)
{
	m_nMouseX = nX;
	m_nMouseY = nY;
	return true;
}

void CSokoban::TryMove(int dx, int dy)
{
	SetHeroDirection(dx, dy);
	if ((m_nHeroX + dx < 0) || (m_nHeroY + dy < 0) || (m_nHeroX + dx > 127) || (m_nHeroY + dy > 128))
		return;
	int iDest = m_CurLevel[m_nHeroX + dx][m_nHeroY + dy];

	switch (iDest)
	{
	case sokBLANK:
	case sokCRATEDEST:
		{
			SUndoMove Undo;
			Undo.bCrate = false;
			Undo.x = m_nHeroX;
			Undo.y = m_nHeroY;
			Undo.nHeroDirection = m_nHeroDirection;
			m_aUndo.push_back(Undo);

			MoveHero(dx, dy);
		}
		break;

	case sokCRATEDONE:
	case sokCRATE:
		{
			int iNext = m_CurLevel[m_nHeroX + dx + dx][m_nHeroY + dy + dy];
			switch (iNext)
			{
			case sokBLANK:
			case sokCRATEDEST:
				if (!m_bWon) // If already won, not allowed to move
				{
					SUndoMove Undo;
					Undo.x = m_nHeroX;
					Undo.y = m_nHeroY;
					Undo.nHeroDirection = m_nHeroDirection;
					Undo.bCrate = true;
					Undo.cxB = m_nHeroX + dx;
					Undo.cyB = m_nHeroY + dy;
					Undo.cxA = m_nHeroX + dx + dx;
					Undo.cyA = m_nHeroY + dy + dy;
					m_aUndo.push_back(Undo);
					
					m_CurLevel[m_nHeroX + dx + dx][m_nHeroY + dy + dy] = (iNext==sokCRATEDEST ? sokCRATEDONE : sokCRATE);
					m_CurLevel[m_nHeroX + dx][m_nHeroY + dy] = (iDest==sokCRATEDONE ? sokCRATEDEST : sokBLANK);
					MoveHero(dx, dy);
					m_iNumPushes++;
					m_fPushedTimer = PUSH_SPRITE_TIME;
				}
				break;
			}
		}

	default:
		// Couldn't move, must be up against a wall or whatever, so show "pushing" sprite.
		m_fPushedTimer = PUSH_SPRITE_TIME;
		break;
	}

	// Test if we've won ..
	for ( int i=0; i<m_nLevelH; i++ )
	{
		for ( int j=0; j<m_nLevelW; j++ )
		{
			if (m_CurLevel[j][i] == sokCRATE) // oops .. still a loose crate lying around
				return;
		}
	}
	// no loose crates lying around, so we must've won, right?
	m_bWon = true;
	m_bGotMouseClick = false;
	AddCompletedLevel(m_iCurLev);
	return;
}

bool CSokoban::OnEvent(SDL_Event &Event, SDLMod ModState)
{
	if (Event.type == SDL_KEYDOWN)
	{
		if (Event.key.type == SDL_KEYDOWN)
		{
			if (m_bShowingAbout)
			{
				switch (Event.key.keysym.sym)
				{
				case SDLK_a:
					SetShowingAbout(false);
					return true;
				case SDLK_SPACE:
					if (m_bShowingAbout)
						SetShowingAbout(false);
					return true;

				default: // gcc, stop whining about unhandled enum values.
					return false;
				}
			}
			else
			{
				switch (Event.key.keysym.sym)
				{
				case SDLK_UP: if (!m_bBusyFollowingAutoPath) TryMove(0, -1); return true;
				case SDLK_DOWN: if (!m_bBusyFollowingAutoPath) TryMove(0, 1); return true;
				case SDLK_LEFT: if (!m_bBusyFollowingAutoPath) TryMove(-1, 0); return true;
				case SDLK_RIGHT: if (!m_bBusyFollowingAutoPath) TryMove(1, 0); return true;
					
					// cheat key
				case SDLK_m: m_bWon = true; m_bGotMouseClick = true; return true;
				case SDLK_n: if (m_iCurLev>0) SetLevel(m_iCurLev-1); return true;
					
					// reset level
				case SDLK_r: SetLevel(m_iCurLev); return true;
					// undo
				case SDLK_u: UndoMove(); return true;
				case SDLK_s:
					m_bShadows = !m_bShadows;
					g_Settings.SetSettingBool(SETTING_SHADOWS, m_bShadows);
					return true;
					
				case SDLK_a:
					SetShowingAbout(true);
					return true;
					
				case SDLK_SPACE:
					if (m_bWon && !m_bGotMouseClick)
						m_bGotMouseClick = true;
					if (m_bShowingAbout)
						SetShowingAbout(false);
					return true;
				
				default: // gcc, stop whining about unhandled enum values.
					return false;
				}
			}
		}
	}
	return false; // not handled
}

void CSokoban::UndoMove()
{
	if (m_aUndo.size() <= 0)
		return;
	SUndoMove Undo = m_aUndo.back();
	m_aUndo.pop_back();

	if (Undo.bCrate)
	{
		int iAfter = m_CurLevel[Undo.cxA][Undo.cyA];
		int iBefore = m_CurLevel[Undo.cxB][Undo.cyB];
		m_CurLevel[Undo.cxA][Undo.cyA] = (iAfter == sokCRATE ? sokBLANK : sokCRATEDEST);
		m_CurLevel[Undo.cxB][Undo.cyB] = (iBefore == sokBLANK ? sokCRATE : sokCRATEDONE);
		
		m_iNumPushes--;
		m_fPushedTimer = 0;
	}
	m_nHeroX = Undo.x;
	m_nHeroY = Undo.y;
	m_nHeroDirection = Undo.nHeroDirection;
	m_iNumMoves--;
}

bool CSokoban::IsLevelCompleted(int nLevelNum)
{
	char buf[16];
	char *szStr = g_Settings.FindSetting(SETTING_LEVELSCOMPLETED);
	if (strlen(szStr)==0) // No levels done
		return false;
	char *pc = szStr;
	int n = 0;
	bool bLevelNum = true; // Every second entry is the number of moves taken
	while (*pc!=0)
	{
		if (*pc=='.')
		{
			buf[n] = 0;
			if (bLevelNum && nLevelNum==atoi(buf))
				return true;
			bLevelNum = !bLevelNum;
			n = 0;
		}
		else
		{
			buf[n++] = *pc;
		}
		pc++;
	}
	return false;
}

int CSokoban::GetNumMovesTaken(int nLevelNum)
{
	char buf[16];
	char *szStr = g_Settings.FindSetting(SETTING_LEVELSCOMPLETED);
	if (strlen(szStr)==0) // No levels done
		return -1;
	char *pc = szStr;
	int n = 0;
	bool bLevelNum = true; // Every second entry is the number of moves taken
	while (*pc!=0)
	{
		if (*pc=='.')
		{
			buf[n] = 0;
			if (bLevelNum && nLevelNum==atoi(buf))
			{
				pc++;
				n = 0;
				while (*pc!='.')
				{
					buf[n++] = *pc;
					pc++;
				}
				buf[n] = 0;
				return atoi(buf);
			}
			bLevelNum = !bLevelNum;
			n = 0;
		}
		else
		{
			buf[n++] = *pc;
		}
		pc++;
	}
	return -1;
}

void CSokoban::AddCompletedLevel(int nLevelNum)
{
	if (IsLevelCompleted(nLevelNum))
	{
		// Took fewer moves this time?
		if (m_nMovesTaken > m_iNumMoves)
		{
			// FIXME: THIS IS REALLY JUNKY
			vector<int> anIndex;
			vector<int> anNumMoves;
			UINT i;
			for ( i=0; i<90; i++ )
			{
				if (IsLevelCompleted(i))
				{
					anIndex.push_back(i);
					anNumMoves.push_back(i==nLevelNum ? m_iNumMoves : GetNumMovesTaken(i));
				}
			}
			char szNewSettingString[2048] = { 0 };
			for ( i=0; i<anIndex.size(); i++ )
			{
				char szBuf[64];
				sprintf(szBuf, "%d.%d.", anIndex[i], anNumMoves[i]);
				strcat(szNewSettingString, szBuf);
			}
			g_Settings.SetSetting(SETTING_LEVELSCOMPLETED, szNewSettingString);
			m_nMovesTaken = m_iNumMoves;
		}
		return;
	}
	char *szStr = g_Settings.FindSetting(SETTING_LEVELSCOMPLETED);
	char buf[2048];
	char buf2[16];
	strcpy(buf, szStr);
	sprintf(buf2, "%d.", nLevelNum);
	strcat(buf, buf2);
	sprintf(buf2, "%d.", m_iNumMoves);
	strcat(buf, buf2);
	// FIXME: Make this save settings immediately, in case crash etc
	g_Settings.SetSetting(SETTING_LEVELSCOMPLETED, buf);
}

/////////////////////////////////////////////////////////////////////////////
//
// "Theme" support
//

void CSokoban::LoadTileSets(const char *szDirectory)
{
	ClearTileSets();
	STileSet *pTileSet;
	// Try to load all the sizes up to a certain max ..
	// Not very flexible, a bit memory inefficient etc, but hey. We're
	// going for "working product", not brilliant design.
	for ( int i=1; i<128; i++ )
	{
		pTileSet = new STileSet;
		if (pTileSet->Load(szDirectory, i))
		{
			m_apTileSets.push_back(pTileSet);
		}
		else
			djDEL(pTileSet);
	}
}

void CSokoban::ClearTileSets()
{
	for ( unsigned int i=0; i<m_apTileSets.size(); i++ )
	{
		djDEL(m_apTileSets[i]);
	}
	m_apTileSets.clear();
	m_pTileSet = NULL;
}

void CSokoban::STileSet::Destroy()
{
	if (pSprites) SDL_FreeSurface(pSprites);
	if (pBorders) SDL_FreeSurface(pBorders);
	pSprites = NULL;
	pBorders = NULL;
	nW = 0;
	nH = 0;
}

void CSokoban::ChooseTileSet()
{
	m_pTileSet = NULL;
	if (m_apTileSets.size()==0)
		return;
	// Choose the largest tileset that will fit this level on the screen
	// We leave some room on the right side and bottom for other UI elements (FIXME:
	// eventually the main CSokoban widget should be "shrunk" and the other bits should
	// be widgets themselves).
	// We also leave a bit of room for the border.
	// If no tileset fits the level, choose the smallest tileset. One day in the
	// future, for version 700, I will maybe implement scrolling .. (FIXME)
	int nMaxTileW = ((m_nW - 140)-16) / m_nLevelW; // Max possible sprite width [pixels]
	int nMaxTileH = ((m_nH - 16)-16) / m_nLevelH; // Max possible sprite width [pixels]
	// NOTE: This relies on the spritesets being in increasing order of size.
	for ( unsigned int i=0; i<m_apTileSets.size(); i++ )
	{
		if (m_apTileSets[i]->nW<=nMaxTileW && m_apTileSets[i]->nH<=nMaxTileH)
			m_pTileSet = m_apTileSets[i];
	}
	if (m_pTileSet==NULL)
		m_pTileSet = m_apTileSets[0];
}

bool CSokoban::STileSet::Load(const char *szDirectory, int nSize)
{
	char strbase[2048];
	sprintf(strbase, "data/sokoban/themes/%s", szDirectory);
	if (strbase[strlen(strbase)-1]!='/' && strbase[strlen(strbase)-1]!='\\')
		strcat(strbase, "/");

	nW = nSize;
	nH = nSize;

	char str[2048];
	SDL_Surface *pSurface = NULL;

	sprintf(str, "%ssprites%d.bmp", strbase, nSize);
	pSurface = SDL_LoadBMP(str);
	if (!pSurface)
	{
		Destroy();
		return false;
	}
	// Set transparency for sprites
	SDL_SetColorKey(pSurface, SDL_SRCCOLORKEY|SDL_RLEACCEL, SDL_MapRGB(pSurface->format, 0, 255, 0));
	pSprites = SDL_DisplayFormat(pSurface);
	SDL_FreeSurface(pSurface);
	
	// Load borders
	sprintf(str, "%sborders%d.bmp", strbase, nSize);
	pSurface = SDL_LoadBMP(str);
	if (!pSurface)
	{
		Destroy();
		return false;
	}
	pBorders = SDL_DisplayFormat(pSurface);
	SDL_FreeSurface(pSurface);

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// CLevel
//

CSokoban::CLevel::CLevel()
{
	iNum = 0;
	x = -1;
	y = -1;
}

CSokoban::CLevel::~CLevel()
{
	for ( UINT j=0; j<apLines.size(); j++ )
	{
		delete[] apLines[j];
	}
	apLines.clear();
}

/////////////////////////////////////////////////////////////////////////////
//
// CLevelSet
//

CSokoban::CLevelSet::CLevelSet()
{
	m_szName = NULL;
}

CSokoban::CLevelSet::~CLevelSet()
{
	for ( UINT i=0; i<apLevels.size(); i++ )
	{
		djDEL(apLevels[i]);
	}
	djDELV(m_szName);
}

/////////////////////////////////////////////////////////////////////////////
//
// CLevelManager
//

CSokoban::CLevelManager::CLevelManager()
{
}

CSokoban::CLevelManager::~CLevelManager()
{
}

void CSokoban::CLevelManager::AddLevelSet(
	const char *szName,
	const char *szAuthor,
	const char *szURL,
	const char *szEmail,
	const char *szDifficulty,
	const char *szLevelFile,
	const char *szDescFile)
{
	SLevelSet LS;
	LS.sName = szName;
	LS.sAuthor = szAuthor;
	LS.sURL = szURL;
	LS.sEmail = szEmail;
	LS.sDifficulty = szDifficulty;
	LS.sLevelFile = szLevelFile;
	LS.sDescFile = szDescFile;
	m_aLevelSets.push_back(LS);
}
