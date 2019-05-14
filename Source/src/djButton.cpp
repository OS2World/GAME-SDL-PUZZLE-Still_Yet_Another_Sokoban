//---------------------------------------------------------------------------
// djButton.cpp
//
// Copyright (C) 2001-2002 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------

#include "djButton.h"
#include "djDesktop.h"

CdjButton::CdjButton()
{
	m_szNormal = NULL;
	m_szHighlighted = NULL;
	m_szSelected = NULL;
	m_nState = 0;
}

CdjButton::~CdjButton()
{
	djDELV(m_szNormal);
	djDELV(m_szHighlighted);
	djDELV(m_szSelected);
}

void CdjButton::SetText(const char *szNormal, const char *szHighlighted, const char *szSelected)
{
	djDELV(m_szNormal);
	djDELV(m_szHighlighted);
	djDELV(m_szSelected);
	m_szNormal = djStrDup(szNormal);
	m_szHighlighted = djStrDup(szHighlighted);
	m_szSelected = djStrDup(szSelected);
}

void CdjButton::Draw(SDL_Surface *pSurface, int nX, int nY)
{
	switch (m_nState)
	{
	case 0: djDrawRect(pSurface, nX, nY, m_nW, m_nH, SDL_MapRGB(pSurface->format,80,80,80)); break;
	case 1: djDrawRect(pSurface, nX, nY, m_nW, m_nH, SDL_MapRGB(pSurface->format,192,192,192)); break;
	case 2: djDrawRect(pSurface, nX, nY, m_nW, m_nH, SDL_MapRGB(pSurface->format,255,255,255)); break;
	}

	//*
	switch (m_nState)
	{
	case 0:
		if (m_szNormal) g_pScreen->m_pFont->DrawText(pSurface, nX + (m_nW - strlen(m_szNormal)*8)/2, nY + (m_nH - 8)/2, m_szNormal);
		break;
	case 1: 
		if (m_szHighlighted) g_pScreen->m_pFont->DrawText(pSurface, nX + (m_nW - strlen(m_szHighlighted)*8)/2, nY + (m_nH - 8)/2, m_szHighlighted);
		break;
	case 2: 
		if (m_szSelected) g_pScreen->m_pFont->DrawText(pSurface, nX + (m_nW - strlen(m_szSelected)*8)/2, nY + (m_nH - 8)/2, m_szSelected);
		break;
	}
	//*/
}
