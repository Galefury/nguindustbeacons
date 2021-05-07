#include <vector>
#include <windows.h>
using namespace std;

#define MAX_ROW 17
#define MAX_COL 20
#define MIN_ROW -1
#define MIN_COL -1
#define MAX_MAPS 4


enum BEACON {
	BEACON_BLOCKED,
	BEACON_WATER,
	BEACON_EMPTY,
	BEACON_BOX,
	BEACON_KNIGHT,
	BEACON_ARROW_UP,
	BEACON_ARROW_RIGHT,
	BEACON_ARROW_DOWN,
	BEACON_ARROW_LEFT,
	BEACON_WALL_HORIZONTAL,
	BEACON_WALL_VERTICAL,
	BEACON_DONUT
};

#define FIRST_BEACON BEACON_BOX
#define LAST_BEACON BEACON_DONUT

enum BTYPE {
	BTYPE_BLOCKED,
	BTYPE_WATER,
	BTYPE_EMPTY,
	BTYPE_SPEED,
	BTYPE_PROD,
	BTYPE_COST
};

#define BTYPE_OFFSET BTYPE_SPEED

BEACON beaconmap[MAX_ROW][MAX_COL];
BTYPE typemap[MAX_ROW][MAX_COL];
double effectmap[MAX_ROW][MAX_COL][3];
double scoremap[MAX_ROW][MAX_COL];

/*
// Already exists in windows.h
struct POINT {
	int x, y;
};
*/

// Holds the beacon shapes in the form of a list of points relative to their position they affect
const vector<vector<POINT>> SHAPES = { //[BEACON][Points]
	{}, // BEACON_BLOCKED
	{}, // BEACON_WATER
	{}, // BEACON_EMPTY
	{{-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}}, // BEACON_BOX
	{{-2, 1}, {-2, -1}, {-1, 2}, {1, 2}, {2, -1}, {2, 1}, {1, -2}, {-1, -2}}, // BEACON_KNIGHT
	{{-1, 0}, {-2, 0}, {-3, -2}, {-3, -1}, {-3, 0}, {-3, 1}, {-3, 2}, {-4, -1}, {-4, 0}, {-4, 1}, {-5, 0}}, // BEACON_ARROW_UP
	{{0, 1}, {0, 2}, {-2, 3}, {-1, 3}, {0, 3}, {1, 3}, {2, 3}, {-1, 4}, {0, 4}, {1, 4}, {0, 5}}, // BEACON_ARROW_RIGHT
	{{1, 0}, {2, 0}, {3, -2}, {3, -1}, {3, 0}, {3, 1}, {3, 2}, {4, -1}, {4, 0}, {4, 1}, {5, 0}}, // BEACON_ARROW_DOWN
	{{0, -1}, {0, -2}, {-2, -3}, {-1, -3}, {0, -3}, {1, -3}, {2, -3}, {-1, -4}, {0, -4}, {1, -4}, {0, -5}}, // BEACON_ARROW_LEFT
	{{0, -1}, {0, -2}, {0, -3}, {0, -4}, {0, -5}, {0, -6}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}}, // BEACON_WALL_HORIZONTAL
	{{-1, 0}, {-2, 0}, {-3, 0}, {-4, 0}, {-5, 0}, {-6, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}}, // BEACON_WALL_VERTICAL
	{{-2, 2}, {-2, 1}, {-2, 0}, {-2, -1}, {-2, -2}, {-1, -2}, {-1, 2}, {0, -2}, {0, 2}, {1, -2}, {1, 2}, {2, 2}, {2, 1}, {2, 0}, {2, -1}, {2, -2}} // BEACON_DONUT
};

const double EFFECTS[LAST_BEACON + 1][BTYPE_COST + 1] = { //[BEACON][BTYPE]
	{0, 0, 0, 0, 0, 0}, // BEACON_BLOCKED
	{0, 0, 0, 0, 0, 0}, // BEACON_WATER
	{0, 0, 0, 0, 0, 0}, // BEACON_EMPTY
	{0, 0, 0, 0.4, 0.3, 0.85}, // BEACON_BOX
	{0, 0, 0, 0.35, 0.35, 0.87}, // BEACON_KNIGHT
	{0, 0, 0, 0.26, 0.22, 0.93}, // BEACON_ARROW_UP
	{0, 0, 0, 0.26, 0.22, 0.93}, // BEACON_ARROW_RIGHT
	{0, 0, 0, 0.26, 0.22, 0.93}, // BEACON_ARROW_DOWN
	{0, 0, 0, 0.26, 0.22, 0.93}, // BEACON_ARROW_LEFT
	{0, 0, 0, 0.27, 0.27, 0.91}, // BEACON_WALL_HORIZONTAL
	{0, 0, 0, 0.27, 0.27, 0.91}, // BEACON_WALL_VERTICAL
	{0, 0, 0, 0.23, 0.26, 0.92}  // BEACON_DONUT
};

// Holds the changes to the beacon effects on a tile upon changing some beacon
struct CHANGE_HOLDER {
	double boost_old[3];
	double boost_new[3];
	int x, y;
};

// Holds information on where to place a beacon
struct PLACEMENT {
	double scorediff;
	POINT place;
	BEACON beacon;
	BTYPE type;
};

// Holds information on when to run which refinement
struct REFINEMENT_SCHEDULE {
	double RunWhenThisCloseToBest; // Minimum score to run this = BestScore - RunWhenThisCloseToBest * (BestScore - WorstScore)
	int BeaconsToRemove;
	int LoopsUntilGivingUp;
	bool RollbackBadChanges;
	double WorstScore;
	double BestScore;
};

// Settings:
bool doReplace = true;
//vector<BEACON> BeaconsToUse = { BEACON_EMPTY, BEACON_BOX, BEACON_KNIGHT, BEACON_ARROW_UP, BEACON_ARROW_DOWN, BEACON_ARROW_RIGHT, BEACON_ARROW_LEFT};
vector<BEACON> BeaconsToUse = { BEACON_EMPTY, BEACON_BOX, BEACON_KNIGHT, BEACON_ARROW_UP, BEACON_ARROW_DOWN, BEACON_ARROW_RIGHT, BEACON_ARROW_LEFT, BEACON_WALL_HORIZONTAL, BEACON_WALL_VERTICAL, BEACON_DONUT };
vector<BTYPE> TypesToUse = { BTYPE_SPEED, BTYPE_PROD, BTYPE_COST };
//vector<BTYPE> TypesToUse = { BTYPE_COST};
double CostFactor = 10.; // How many tiles are needed to produce ingredients for what you're placing (without speed or prod beacons)
double SpeedCap = 1000.; // How much speed increase to cap, more will not help (capped speed divided by unbeaconed speed)
bool doRemovals = false; // Do another optimization pass with remove and replace
bool doRefinements = true; // Do another optimization pass with refinement according to the refinement schedule
vector<REFINEMENT_SCHEDULE> RefinementSchedule = {
	{0.03, 1, 1, false, INFINITY, 0}, // Remove a beacon and place a new one, score should never get worse by doing this
	{0.05, 2, 10, true, INFINITY, 0}, // Remove two beacons
	{0.1, 3, 50, true, INFINITY, 0}, // Remove three beacons
	{0.2, 10, 100, true, INFINITY, 0 } // Remove 10 beacons
};


void SetWhichToCheck (int maps, int options);
int  MakeNumber (char *sData);
bool LoadOperatingMode (int *maps, int *options, int *beacons, int *beacontype);

int  DisplayResults (int maps, int options, int beacons, int beacontype);

void Shuffle ();
void CopyMap(BEACON sourcemap[MAX_ROW][MAX_COL], BEACON destmap[MAX_ROW][MAX_COL], BTYPE sourcetmap[MAX_ROW][MAX_COL], BTYPE desttmap[MAX_ROW][MAX_COL]);

// Setup
void InitBeaconAndTypeMaps(int options);

// Score Calculation
double CalculateScoreForTile(double speed, double prod, double cost, BEACON tile_content);
double CalculateScoreDiff(BEACON beacon, BTYPE beacontype, BEACON oldbeacon, BTYPE oldbeacontype, int x, int y, bool update_scoremaps);
void ResetScoreMaps();
void RecalculateScoremaps();
double TotalScore();

// Beacon Placement
PLACEMENT FindBestBeaconPosition(BEACON beacon, BTYPE type);
PLACEMENT FindBestBeacon();
bool PlaceBestBeacon();
bool RemoveAndReplaceOnce();


// Optimizers
int OptimizeByReplacement();
int OptimizeByRemoveAndReplace();
int OptimizeByRefinement(int BeaconsToRemove, int LoopsUntilGivingUp, bool RollbackBadChanges);
