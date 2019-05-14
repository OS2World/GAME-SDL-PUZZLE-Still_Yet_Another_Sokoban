//---------------------------------------------------------------------------
// Sokoban.h
//
// Copyright (C) 2001-2004 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------

#include "djItem.h"
#include <string>

/*! Sokoban app widget */
class CSokoban : public CdjItem
{
public:
	//! Constructor
	CSokoban();
	//! Destructor
	virtual ~CSokoban();

	//
	//
	// CdjItem overridables
	//
	//

	virtual bool Initialize();
	virtual bool Destroy();
	virtual bool OnEvent(SDL_Event &Event, SDLMod ModState);
	virtual bool Update(float dt);
	virtual void Draw(SDL_Surface *pSurface, int nX, int nY);
	virtual bool OnMouseButtonDown(int nButton);
	virtual bool OnMouseMove(int nX, int nY, int nDX, int nDY);
	
protected:
	bool IsLevelCompleted(int nLevelNum);
	int  GetNumMovesTaken(int nLevelNum);
	void AddCompletedLevel(int nLevelNum);

	void UndoMove();
	void TryMove(int dx, int dy); //!< Try to move hero
	void MoveHero(int dx, int dy); //!< Actually move hero. Called from within TryMove() if a move is possible.
	void SetHeroDirection(int dx, int dy); //!< Set m_nHeroDirection based on desired movement
	bool LoadLevelset(const char *szFilename);
	int SetLevel(int iLev);


	int m_nLevelW;	//!< Width of current level [blocks]
	int m_nLevelH;	//!< Height of current level [blocks]
	int m_nHeroX;	//!< Current X position of hero
	int m_nHeroY;	//!< Current Y position of hero
	bool m_bWon;	//!< Current level completed?
	bool m_bGotMouseClick;
	int m_iCurLev;
	int m_iNumMoves;
	int m_iNumPushes;
	char m_CurLevel[128][128];	//!< Current level

	int m_nMouseX;
	int m_nMouseY;

	//! Show/hide shadows (I think that some people may find them visually distracting when trying to solve puzzles
	//! and will want to turn them off)
	bool m_bShadows;
	
	int m_nHeroDirection;		//!< Clockwise: 0,1,2,3,4: Neutral,up,right,down,left

	int m_nMovesTaken;			//!< If completed current level, moves taken, else -1
	
	///////////////////////////////////////////////////////////////
	// Graphics

	//! A single set of sprites
	struct STileSet
	{
		STileSet() { pSprites = NULL; pBorders = NULL; nW = 0; nH = 0; }
		~STileSet() { Destroy(); }
		void Destroy();
		bool Load(const char *szDirectory, int nSize);

		SDL_Surface *pSprites;	//!< Sprites (walls and hero)
		SDL_Surface *pBorders;	//!< Border bitmaps drawn around level
		int nW;	//!< Width [pixels]
		int nH;	//!< Height [pixels]
	};

	vector<STileSet *> m_apTileSets; //!< Array of tilesets, in increasing order of size. This is a "theme"
	STileSet *m_pTileSet; //!< Current tileset

	void LoadTileSets(const char *szDirectory);
	void ClearTileSets();
	void ChooseTileSet(); //!< Choose the largest tileset that will fit the current level on the screen

	SDL_Surface *m_pSokobanHeader;	//!< "Sokoban" header on top right of screen
	SDL_Surface *m_pAbout;			//!< About screen
	//! Set after you push a crate, in order to show a different ("pushing") sprite.
	//! Also set if you "push" against a wall.
	float m_fPushedTimer;

	bool m_bShowingAbout;		//!< Showing about screen?

	void SetShowingAbout(bool b);


	//! Level character constants
	enum
	{
		sokBLANK     = 1,	//!< Nothing
		sokCRATE     = 39,	//!< Crate
		sokCRATEDEST = 9,	//!< Crate destination area
		sokCRATEDONE = 47,	//!< Crate destination area with crate on it
		sokWALL      = 16,	//!< Solid wall. sokWALL+0 to sokWALL+15 are used for the different direction walls
		sokWALL_LAST = sokWALL+15,	//!< Solid wall last possible value
		sokWALL_CRACKED = sokWALL + 5 + 16 //!< Special case: "cracked" visual effect on vertical piece of wall
	};

	//! Helper: tests if level block is a wall
	inline bool IsWall(char c) const { return (c>=sokWALL && c<=sokWALL_LAST) || c==sokWALL_CRACKED; }
	//! Helper: tests if level block is a crate
	inline bool IsCrate(char c) const { return (c==sokCRATE || c==sokCRATEDONE); }
	//! Helper: tests if level block is of a type that casts a shadow
	inline bool CastsShadow(char c) const { return (IsWall(c) || IsCrate(c)); }


	///////////////////////////////////////////////////////////////
	// Mouse-click move helpers. Shortest path algorithm ripped from xsokoban ..

	int m_nTargetX;
	int m_nTargetY;
	int m_FindMap[128][128];
	// find the shortest path to the target via a fill search algorithm
	void FindTarget(int nX, int nY, int nPathLen);
	struct SAutoMove
	{
		int dx;
		int dy;
	};
	vector<SAutoMove> m_aAutoPath;
	void ClearAutoPath();
	bool m_bBusyFollowingAutoPath;
	float m_fAutoPathFollowTimer;

	///////////////////////////////////////////////////////////////
	// Mouse-click crate move helpers.
	bool m_bMayPickCrates;
	int m_nCrateX;
	int m_nCrateY;

	///////////////////////////////////////////////////////////////
	// Level storage structures
	
	class CLevel
	{
	public:
		CLevel();
		~CLevel();

		int iNum;
		int x, y; // startpos
		vector<char *> apLines;
	};

	class CLevelSet
	{
	public:
		CLevelSet();
		~CLevelSet();

		vector<CLevel *> apLevels;
		char *m_szName;
	};
	CLevelSet *m_pCurSet;	//! Current level set

	class CLevelManager
	{
	public:
		CLevelManager();
		~CLevelManager();

		//! Level set descriptor
		struct SLevelSet
		{
			std::string sName;
			std::string sAuthor;
			std::string sURL;
			std::string sEmail;
			std::string sDifficulty;
			std::string sLevelFile;
			std::string sDescFile;
		};

		UINT GetNumLevelSetDescriptors() const { return m_aLevelSets.size(); }

		const SLevelSet &GetLevelSetDescriptor(UINT uIndex) const { return m_aLevelSets[uIndex]; }

		// Load a descriptor given the .desc file
		//void LoadLevelSetDescriptor(const char *szFilename);

		// Add a descriptor
		void AddLevelSet(
			const char *szName,
			const char *szAuthor,
			const char *szURL,
			const char *szEmail,
			const char *szDifficulty,
			const char *szLevelFile,
			const char *szDescFile);

	protected:
		vector<SLevelSet> m_aLevelSets;
	};
	CLevelManager *m_pLevelManager;

	///////////////////////////////////////////////////////////////
	// Undo
	struct SUndoMove
	{
		bool bCrate;
		int x, y;
		int cxB, cyB, cxA, cyA; // crate coords before, after
		int nHeroDirection;
	};
	vector<SUndoMove> m_aUndo;
};
