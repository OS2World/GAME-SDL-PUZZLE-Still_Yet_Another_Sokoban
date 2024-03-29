//---------------------------------------------------------------------------
// djUtils.h
//
// Copyright (C) 2002 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------

#ifndef _DJUTILS_H_
#define _DJUTILS_H_

//! Make a deep copy of string szSource. Returns NULL if NULL is passed.
extern char *djStrDup(const char *szSource);

//! Scalar delete helper macro.
#define djDEL(p) if (p!=NULL) { delete p; p = NULL; }
//! Vector delete helper macro.
#define djDELV(p) if (p!=NULL) { delete []p; p = NULL; }

//! Min
#define djMIN(a,b) ((a)<(b)?(a):(b))
//! Max
#define djMAX(a,b) ((a)>(b)?(a):(b))
//! Absolute value
#define djABS(a) ((a)<0?-(a):(a))

//! Write a debug message
extern void djLog(const char *szFormat, ...);

#ifndef UINT
typedef unsigned int UINT;
#endif

//! If nExpression is zero, displays a message box which includes the
//! error szExpression, and stops the application. You should not call
//! this directly, you should call either djASSERT() or djASSERTD().
extern void djAssert(int nExpression, const char *szFile, int nLine, const char *szExpression);

#define djASSERT(Expression) djAssert((int)Expression, __FILE__, __LINE__, #Expression)

#ifdef _DEBUG
#define djASSERTD(Expression) djASSERT(Expression)
#else
#define djASSERTD(Expression)
#endif

#endif
