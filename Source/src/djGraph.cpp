//---------------------------------------------------------------------------
// djGraph.cpp
//
// Copyright (C) 2001-2003 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------

#include "djGraph.h"
#include "djUtils.h"
#include <string.h>

#define SDL_SETRECT(SDLRect, nX, nY, nW, nH) SDLRect.x = (nX); SDLRect.y = (nY); SDLRect.w = (nW); SDLRect.h = (nH);

int djBlit(SDL_Surface *pDest, SDL_Surface *pSrc, int iX, int iY)
{
	if (pSrc==NULL || pDest==NULL)
		return -1;
	SDL_Rect rc;
	rc.x=iX;
	rc.y=iY;
	rc.w=pSrc->w;
	rc.h=pSrc->h;
	return SDL_BlitSurface(pSrc, NULL, pDest, &rc);
}

int djBlit(SDL_Surface *pDest, SDL_Surface *pSrc, int iX, int iY, int iSrcX, int iSrcY, int iW, int iH)
{
	if (pSrc==NULL || pDest==NULL)
		return -1;
	SDL_Rect rcSrc, rcDest;
	rcSrc.x = iSrcX;
	rcSrc.y = iSrcY;
	rcSrc.w = iW;
	rcSrc.h = iH;
	rcDest.x = iX;
	rcDest.y = iY;
	rcDest.w = iW;
	rcDest.h = iH;
	return SDL_BlitSurface(pSrc, &rcSrc, pDest, &rcDest);
}

int djDrawSprite(int iSprite, SDL_Surface *pDest, SDL_Surface *pSrc, int iX, int iY, int iW, int iH)
{
	if (pSrc==NULL || pDest==NULL)
		return -1;
	SDL_Rect rcSrc, rcDest;
	rcSrc.x = (iSprite % (pSrc->w / iW)) * iW;
	rcSrc.y = (iSprite / (pSrc->w / iW)) * iH;
	rcSrc.w = iW;
	rcSrc.h = iH;
	rcDest.x = iX;
	rcDest.y = iY;
	rcDest.w = iW;
	rcDest.h = iH;
	if (rcSrc.y>=pSrc->h || rcDest.y>=pDest->h || rcDest.x>=pDest->w || rcSrc.x>=pSrc->w)
		return 0;
	return SDL_BlitSurface(pSrc, &rcSrc, pDest, &rcDest);
}

/*
int djDrawText(SDL_Surface *pDest, SDL_Surface *pSrc, int iX, int iY, char *szStr)
{
	if (pSrc==NULL || pDest==NULL)
		return -1;
	for ( UINT i=0; i<strlen(szStr); i++ )
	{
		djDrawSprite((int)(unsigned char)szStr[i], pDest, pSrc, iX+i*8, iY, 8, 8);
	}
	return 0;
}
*/

void djDrawRect(SDL_Surface *pDest, int x, int y, int w, int h, UINT uColour)
{
	SDL_Rect Rect;
	SDL_SETRECT(Rect, x, y, w, 1);
	SDL_FillRect(pDest, &Rect, uColour);
	SDL_SETRECT(Rect, x, y, 1, h);
	SDL_FillRect(pDest, &Rect, uColour);
	SDL_SETRECT(Rect, x+w-1, y, 1, h);
	SDL_FillRect(pDest, &Rect, uColour);
	SDL_SETRECT(Rect, x, y+h-1, w, 1);
	SDL_FillRect(pDest, &Rect, uColour);
}

void djDrawFilledRect(SDL_Surface *pDest, int x, int y, int w, int h, UINT uColour)
{
	SDL_Rect Rect;
	SDL_SETRECT(Rect, x, y, w, h);
	SDL_FillRect(pDest, &Rect, uColour);
}
