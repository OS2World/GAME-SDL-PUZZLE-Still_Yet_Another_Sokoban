//---------------------------------------------------------------------------
// djButton.h
//
// Copyright (C) 2001-2002 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------
#ifndef _DJBUTTON_H_
#define _DJBUTTON_H_

#include "djItem.h"

/*!\brief Button widget.
\author   David Joffe

Related: \ref CdjItem
*/
class CdjButton : public CdjItem
{
public:
	//! Constructor
	CdjButton();
	//! Destructor
	virtual ~CdjButton();

	//! Set button text
	void SetText(const char *szNormal, const char *szHighlighted=NULL, const char *szSelected=NULL);

	virtual void OnMouseEnter(int nX, int nY) { m_nState = 1; }
	virtual void OnMouseLeave() { m_nState = 0; }
	virtual bool OnMouseButtonDown(int nButton) { m_nState = 2; return true; }
	virtual bool OnMouseButtonUp(int nButton) { m_nState = 1; return true; }
	virtual void Draw(SDL_Surface *pSurface, int nX, int nY);

protected:
	char *m_szNormal;
	char *m_szHighlighted;
	char *m_szSelected;
	int m_nState;
};

#endif
