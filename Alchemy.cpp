// Alchemy.cpp : Defines the entry point for the console application.
// 
/* TO DO:
- Re-add beacon forcing
   * Also allow preventing placement of some beacon for some time
   * Data structures of forced beacons and prevented beacons with beacon, min, max, current, and done
- Re-add reading inputs/runmode.txt
- Refactor beaconmap + typemap into single array of structs
   * Add "fixed" field for future use, possibly also fixed beacon + fixed type separate fields
- Add continuous run mode looking for best setups, check how Ahlyis did it
*/

#pragma warning(disable : 4996)

#include "stdafx.h"
#include <string.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <windows.h>
#include <vector>
#include "Accessories.h"

using namespace std;

int basemap[MAX_ROW][MAX_COL];
BEACON bestbeaconmap[MAX_ROW][MAX_COL];
BTYPE besttypemap[MAX_ROW][MAX_COL];
double bestscore;

int forcetype;
int forcecountmin;
int forcecountmax;
bool PrintLegend = true;
int mapbasevalue;
int mapfinalvalue;
int iTotalArea = MAX_ROW * MAX_COL;
int searchorder[MAX_ROW * MAX_COL];
int iTries;
int iBestVal;
bool bDebug = false;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
int beaconcolor = 15;

int main(int argc, char* argv[])
{
    int i;
    int maps = 1,
        options = 0,
        beacons = 0,
        beacontype = 0;

    forcetype = -1;
    forcecountmin = forcecountmax = 0;
    mapbasevalue = 0;
    mapfinalvalue = 0;
    iBestVal = 0;

    if (!LoadOperatingMode (&maps, &options, &beacons, &beacontype) )
    {
        iTries = 1;
        /* Initialize random seed. */
        srand ( time(NULL) );

        printf ("Which map? 1=Tutorial, 2=Flesh, 3=Tronne, 4=Candyland\n");
        cin >> maps;

        if ( (maps < 2) || (maps > MAX_MAPS) )
        {
            maps = 1;
        }

        printf ("Include currently blockaded? 1=yes\n");
        cin >> options;

        if (options != 1)
        {
            options = 0;
        }

        printf ("Up to which beacons? 0=box only, 1=knight, 2=arrow, 3=wall, 4=donut\n");
        cin >> beacons;

        if ( (beacons < 1) || (beacons > 4) )
        {
            beacons = 0;
        }

        printf ("Which beacon type? 0=speed, 1=production, 2=efficiency\n");
        cin >> beacontype;

        if ( (beacontype < 1) || (beacontype > 2) )
        {
            beacontype = 0;
        }

        printf ("Force X amount of which beacon type? 0=square, 1=knight, 2=arrow, 3=wall, 4=donut\n");
        cin >> forcetype;

        if ( (forcetype < 1) || (forcetype > 4) )
        {
            forcetype = 0;
        }

        printf ("Force how many of that type? (0 - 999)\n");
        cin >> forcecountmin;

        if ( (forcecountmin < 1) || (forcecountmin > 999) )
        {
            forcecountmin = 0;
        }
        forcecountmax = forcecountmin; /* variable force count range only via input file. */
    }
    SetWhichToCheck (maps, options);
    
    bestscore = 0;
    for (i = 0; i < iTries; i++)
    {
        if (bDebug)
        {
            cout << "Try #" << i + 1;
        }
        Shuffle();
        InitBeaconAndTypeMaps(options); // Reset maps
        RecalculateScoremaps();
        mapbasevalue = floor(TotalScore());

        OptimizeByReplacement();
        RecalculateScoremaps();
        mapfinalvalue = floor(TotalScore());

        if (mapfinalvalue > bestscore + 0.001) {
            bestscore = mapfinalvalue;
            CopyMap(beaconmap, bestbeaconmap, typemap, besttypemap);
        }
    }

    // Copy best map back into regular map
    CopyMap(bestbeaconmap, beaconmap, besttypemap, typemap);
    mapfinalvalue = bestscore;
    DisplayResults (maps, options, beacons, beacontype);

    return 0;
}

bool LoadOperatingMode (int *maps, int *options, int *beacons, int *beacontype)
{
    char sData[360] = {0},
         sFile[80];
    bool bRetVal = false;
    unsigned int iSeed = 0;

    sprintf (sFile, ".\\runmode.txt");

    ifstream Data(sFile);

    Data >> sData;
    *maps = MakeNumber (sData);

    if ( (*maps > 0) && (*maps <= MAX_MAPS) )
    {
        Data >> sData;

        *options = MakeNumber (sData);

        if (*options != 1)
            *options = 0;

        Data >> sData;

        *beacons = MakeNumber (sData);

        if ( (*beacons < 1) || (*beacons > 4) )
            *beacons = 0;

        Data >> sData;

        *beacontype = MakeNumber (sData);

        if ( (*beacontype != 1) && (*beacontype != 2) )
            *beacontype = 0;

        Data >> sData;

        forcetype = MakeNumber (sData);

        if ( (forcetype < 1) || (forcetype > 4) )
            forcetype = 0;

        Data >> sData;

        forcecountmin = MakeNumber (sData);

        if ( (forcecountmin < 1) || (forcecountmin > 999) )
            forcecountmin = 0;

        Data >> sData;

        forcecountmax = MakeNumber (sData);

        if ( (forcecountmax < forcecountmin) || (forcecountmax > 999) )
            forcecountmax = forcecountmin;

        Data >> sData;

        if (MakeNumber (sData) == 1)
            PrintLegend = false;
        else
            PrintLegend = true;

        Data >> sData;

        iSeed = MakeNumber (sData);

        if (iSeed > 0)
        {
            srand (iSeed);
        }
        else
        {
            /* Initialize random seed. */
            srand (time (NULL) );
        }

        Data >> sData;

        iTries = MakeNumber (sData);

        if (iTries < 1)
            iTries = 1;

        cout << "We will be running " << iTries << " attempts this run.\n";

       bRetVal = true;
    }

        Data >> sData;

        if (MakeNumber (sData) == 1)
            bDebug = true;

    return bRetVal;
}

int MakeNumber (char *sData)
{
    int x = 0, iRetVal =0;

    while (    (sData[x] >= '0')
            && (sData[x] <= '9')
          )
    {
        iRetVal += sData[x] - 48;
        iRetVal *= 10;
        x++;

        if (iRetVal > 1000000000)
        {
            sData[0] = 0;
            x = 0;
            iRetVal = 0;
        }
    }
    iRetVal /= 10;

    return iRetVal;
}

/* This originally was just to load the map. It now includes other setup functionality as well. */
void SetWhichToCheck (int maps, int options)
{
    char sData[360] = {0},
         sFile[80];
    bool bRetVal = false;
    int iVal = 0, x = 0, y = 0;

    sprintf (sFile, ".\\tutorial.txt");

    if (maps == 2)
    {
        sprintf (sFile, ".\\flesh.txt");
    }

    if (maps == 3)
    {
        sprintf (sFile, ".\\tronne.txt");
    }

    if (maps == 4)
    {
        sprintf (sFile, ".\\candyland.txt");
    }

    ifstream Data(sFile);

    Data >> sData;

    for (x = 0; x < 17; x++)
    {
        for (y = 0; y < 20; y++)
        {
            if (sData[y] == '1') // 1 = open to place
            {
                basemap[x][y] = 1;
            }
            else if(sData[y] == '2') // 2 = usable, but currently blocked
            {
                basemap[x][y] = 2;
            }
            else
            {
                basemap[x][y] = 0; // 0 = unusable square
            }
        }
        Data >> sData;
    }

    for (x = 0; x < (MAX_ROW * MAX_COL); x++)
    {
        searchorder[x] = x;
    }

    return;
}


void InitBeaconAndTypeMaps(int options) {

    for (int x = 0; x < MAX_ROW; x++)
    {
        for (int y = 0; y < MAX_COL; y++)
        {
            switch (basemap[x][y])
            {
            case 1:
            {
                beaconmap[x][y] = BEACON_EMPTY;
                typemap[x][y] = BTYPE_EMPTY;
                break;
            }
            case 2:
            {
                if (options == 1)
                {
                    beaconmap[x][y] = BEACON_EMPTY;
                    typemap[x][y] = BTYPE_EMPTY;
                }
                else
                {
                    beaconmap[x][y] = BEACON_BLOCKED;
                    typemap[x][y] = BTYPE_BLOCKED;
                }
                break;
            }
            default:
            {
                beaconmap[x][y] = BEACON_WATER;
                typemap[x][y] = BTYPE_WATER;
                break;
            }
            }
        }
    } // initialize base working map,
}


int  DisplayResults (int maps, int options, int beacons, int beacontype)
{
    int x, y;
    SetConsoleTextAttribute(hConsole, 15);

    cout << "Calculated beacon placement for map: ";

    switch (maps)
    {
        case 1:
        {
            SetConsoleTextAttribute(hConsole, 1);
            cout << "Tutorial\n";
            break;
        }
        case 2:
        {
            SetConsoleTextAttribute(hConsole, 12);
            cout << "Flesh\n";
            break;
        }
        case 3:
        {
            SetConsoleTextAttribute(hConsole, 3);
            cout << "Tronne\n";
            break;
        }
        case 4:
        {
            SetConsoleTextAttribute(hConsole, 13);
            cout << "Candyland\n";
            break;
        }
        default:
        {
            cout << "Unknown - " << maps << "\n";
            break;
        }
    }
    SetConsoleTextAttribute(hConsole, 15);

    if (options == 1)
    {
        cout << "With every square unlocked.\n";
    }
    else
    {
        cout << "Ignoring locked squares.\n";
    }

    cout << "Using box";

    if (beacons == 0)
    {
        cout << " beacons only.\n";
    }
    else if (beacons == 1)
    {
        cout << " and knight beacons only.\n";
    }
    else if (beacons == 2)
    {
        cout << ", knight and arrow beacons.\n";
    }
    else if (beacons == 3)
    {
        cout << " through wall beacons.\n";
    }
    else if (beacons == 4)
    {
        cout << " through donut beacons.\n";
    }
    else
    {
        cout << "... ERROR. Using beacon " << beacons << " is not supported!\n";
    }

    cout << "Using ";
    switch (beacontype)
    {
        case 0:
        {
            SetConsoleTextAttribute(hConsole, 9);
            cout << "speed";
            break;
        }
        case 1:
        {
            SetConsoleTextAttribute(hConsole, 5);
            cout << "production";
            break;
        }
        case 2:
        {
            SetConsoleTextAttribute(hConsole, 14);
            cout << "efficiency";
            break;
        }
        default:
        {
            cout << "Using ERROR - TYPE " << beacontype << " beacons.\n\n";
            break;
        }
    }
    SetConsoleTextAttribute(hConsole, 15);
    cout << " beacons.\n";

    if (PrintLegend)
    {
        cout << "Legend:\n~ = unusable square\n. = open square\n# = box beacon\nK = knight beacon\n";
        cout << "> = right facing arrow beacon\n";
        cout << "< = left facing arrow beacon\n";
        cout << "v = down facing arrow beacon\n";
        cout << "^ = up facing arrow beacon\n";
        cout << "- = horizontal wall beacon\n| = vertical wall beacon\nO = donut beacon\n";
    }
    cout << "\nThe map should look like this:\n";

    for (x = 0; x < MAX_ROW; x++)
    {
        for (y = 0; y < MAX_COL; y++)
        {
            switch (typemap[x][y])
            {
                case BTYPE_SPEED:
                {
                    beaconcolor = 9;
                    break;
                }
                case BTYPE_PROD:
                {
                    beaconcolor = 5;
                    break;
                }
                case BTYPE_COST:
                {
                    beaconcolor = 14;
                    break;
                }
            }

            switch (beaconmap[x][y])
            {
                case BEACON_WATER:
                {
                    if (maps == 1)
                        SetConsoleTextAttribute(hConsole, 24);
                    else if (maps == 2)
                        SetConsoleTextAttribute(hConsole, 197);
                    else if (maps == 3)
                        SetConsoleTextAttribute(hConsole, 48);
                    else if (maps == 4)
                        SetConsoleTextAttribute(hConsole, 222);

                    cout << "~";
                    break;
                }
                case BEACON_BLOCKED:
                {
                    if (maps == 1)
                        SetConsoleTextAttribute(hConsole, 24);
                    else if (maps == 2)
                        SetConsoleTextAttribute(hConsole, 197);
                    else if (maps == 3)
                        SetConsoleTextAttribute(hConsole, 48);
                    else if (maps == 4)
                        SetConsoleTextAttribute(hConsole, 222);

                    cout << " ";
                    break;
                }
                case BEACON_EMPTY:
                {
                    SetConsoleTextAttribute(hConsole, 15);
                    cout << ".";
                    break;
                }
                case BEACON_BOX:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "#";
                    break;
                }
                case BEACON_KNIGHT:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "K";
                    break;
                }
                case BEACON_ARROW_RIGHT:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << ">";
                    break;
                }
                case BEACON_ARROW_LEFT:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "<";
                    break;
                }
                case BEACON_ARROW_DOWN:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "v";
                    break;
                }
                case BEACON_ARROW_UP:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "^";
                    break;
                }
                case BEACON_WALL_HORIZONTAL:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "-";
                    break;
                }
                case BEACON_WALL_VERTICAL:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "|";
                    break;
                }
                case BEACON_DONUT:
                {
                    SetConsoleTextAttribute(hConsole, beaconcolor);
                    cout << "O";
                    break;
                }
                default:
                {
                    cout << "ERROR";
                    break;
                }
            }
        }
        SetConsoleTextAttribute(hConsole, 15);
        cout << " \n";
    }
    SetConsoleTextAttribute(hConsole, 15);
    cout << "Empty map value: " << mapbasevalue << "  Populated map value: " << mapfinalvalue << "\n";

    return 0;
}





// Currently unused
void Shuffle ()
{
    int x, i, temp;

    for (x = 0; x < iTotalArea; x++)
    {
        i = rand() % (iTotalArea);
        temp = searchorder[x];
        searchorder[x] = searchorder[i];
        searchorder[i] = temp;
    }

    return;
}

// Currently unused, needs update
void CopyMap (BEACON sourcemap[MAX_ROW][MAX_COL], BEACON destmap[MAX_ROW][MAX_COL], BTYPE sourcetmap[MAX_ROW][MAX_COL], BTYPE desttmap[MAX_ROW][MAX_COL])
{
    int x, y;

    for (x = 0; x < MAX_ROW; x++)
    {
        for (y = 0; y < MAX_COL; y++)
        {
            destmap[x][y] = sourcemap[x][y];
            desttmap[x][y] = sourcetmap[x][y];
        }
    }
}



// Returns the score for one tile. Generalize to multiple scoring functions here.
double CalculateScoreForTile(double speed, double prod, double cost, BEACON tile_content) {
    if (tile_content != BEACON_EMPTY) return 0; // No score for filled or blocked tiles
    return 100 * ((1 + speed) < SpeedCap ? (1 + speed) : SpeedCap) * (1 + prod) * (1 + CostFactor * (1 - cost));
}

// Calculate score difference based on replacing a beacon (or empty space) in a spot with another beacon (or empty space)
double CalculateScoreDiff(BEACON beacon, BTYPE beacontype, BEACON oldbeacon, BTYPE oldbeacontype, int x, int y, bool update_scoremaps) {
    // No change? No difference.
    if (beacon == oldbeacon && beacontype == oldbeacontype) return 0;

    // Build a list of changed tiles to recalculate the score for by going through the tiles affected by the old and new beacon
    vector<CHANGE_HOLDER> changes = {};
    CHANGE_HOLDER c;
    for (POINT p : SHAPES[oldbeacon]) { // Go through old beacon points
        if (x + p.x >= 0 && x + p.x < MAX_ROW && y + p.y >= 0 && y + p.y < MAX_COL && beaconmap[x + p.x][y + p.y] >= BEACON_EMPTY) { // within bounds, open tile
            c.boost_old[0] = c.boost_old[1] = c.boost_new[0] = c.boost_new[1] = 0;
            c.boost_old[2] = c.boost_new[2] = 1;
            c.x = x + p.x;
            c.y = y + p.y;
            c.boost_old[oldbeacontype - BTYPE_OFFSET] = EFFECTS[oldbeacon][oldbeacontype];
            changes.push_back(c);
        }
    }
    bool updated;
    for (POINT p : SHAPES[beacon]) { // Go through new beacon points
        if (x + p.x >= 0 && x + p.x < MAX_ROW && y + p.y >= 0 && y + p.y < MAX_COL && beaconmap[x + p.x][y + p.y] >= BEACON_EMPTY) { // within bounds, open tile
            // Update existing change?
            updated = false;
            for (CHANGE_HOLDER q : changes) {
                if (x + p.x == q.x && y + p.y == q.y) {
                    // Update
                    q.boost_new[beacontype - BTYPE_OFFSET] = EFFECTS[beacon][beacontype];
                    updated = true;
                    break;
                }
            }

            // Add new change
            if (!updated) {
                c.boost_old[0] = c.boost_old[1] = c.boost_new[0] = c.boost_new[1] = 0;
                c.boost_old[2] = c.boost_new[2] = 1;
                c.x = x + p.x;
                c.y = y + p.y;
                c.boost_new[beacontype - BTYPE_OFFSET] = EFFECTS[beacon][beacontype];
                changes.push_back(c);
            }
        }
    }

    // Go through changes and calculate the score difference
    double scorediff = 0, scorenew;
    for (CHANGE_HOLDER q : changes) {
        scorenew = CalculateScoreForTile(effectmap[q.x][q.y][0] + q.boost_new[0] - q.boost_old[0],
            effectmap[q.x][q.y][1] + q.boost_new[1] - q.boost_old[1],
            effectmap[q.x][q.y][2] * q.boost_new[2] / q.boost_old[2],
            beaconmap[q.x][q.y]
        );
        scorediff = scorediff + scorenew - scoremap[q.x][q.y];

        if (update_scoremaps) {
            effectmap[q.x][q.y][0] += q.boost_new[0] - q.boost_old[0];
            effectmap[q.x][q.y][1] += q.boost_new[1] - q.boost_old[1];
            effectmap[q.x][q.y][2] *= q.boost_new[2] / q.boost_old[2];
            scoremap[q.x][q.y] = scorenew;
        }
    }

    // If going from empty to beacon or beacon to empty, also take into account score change from that
    if (beacon == BEACON_EMPTY || oldbeacon == BEACON_EMPTY) {
        scorenew = CalculateScoreForTile(effectmap[x][y][0],
            effectmap[x][y][1],
            effectmap[x][y][2],
            beacon
        );
        scorediff = scorediff + scorenew - scoremap[x][y];

        if (update_scoremaps) {
            scoremap[x][y] = scorenew;
        }
    }

    return scorediff;
}


void ResetScoreMaps() {
    for (int x = 0; x < MAX_ROW; x++) {
        for (int y = 0; y < MAX_COL; y++) {
            effectmap[x][y][0] = effectmap[x][y][1] = 0.0;
            effectmap[x][y][2] = 1.0;
            scoremap[x][y] = beaconmap[x][y] >= BEACON_EMPTY ? 100.0 : 0.0;
        }
    }
}


// Calculate the effectmap and scoremap based on beaconmap and typemap
void RecalculateScoremaps() {
    ResetScoreMaps();

    // Update scores from empty for all tiles with their current beacons, set score and effect maps
    for (int x = 0; x < MAX_ROW; x++) {
        for (int y = 0; y < MAX_COL; y++) {
            CalculateScoreDiff(beaconmap[x][y], typemap[x][y], BEACON_EMPTY, BTYPE_EMPTY, x, y, true);
        }
    }
}

double TotalScore() {
    double score = 0;
    for (int x = 0; x < MAX_ROW; x++) {
        for (int y = 0; y < MAX_COL; y++) {
            score += scoremap[x][y];
        }
    }
    return score;
}

// Finds the best placement for one specific beacon
PLACEMENT FindBestBeaconPosition(BEACON beacon, BTYPE type) {
    PLACEMENT placement;
    placement.scorediff = 0;
    double scorediff;

    int x, y;
    for (int i = 0; i < iTotalArea; i++) {
        x = searchorder[i] / MAX_COL;
        y = searchorder[i] % MAX_COL;
        if (beaconmap[x][y] >= BEACON_EMPTY && (doReplace || beaconmap[x][y] == BEACON_EMPTY)) {
            scorediff = CalculateScoreDiff(beacon, type, beaconmap[x][y], typemap[x][y], x, y, false);
            if (scorediff > placement.scorediff + 0.001) {
                placement.scorediff = scorediff;
                placement.place.x = x;
                placement.place.y = y;
                placement.beacon = beacon;
                placement.type = type;

            }
        }
    }
    return placement;
}

// Finds the best beacon to place
PLACEMENT FindBestBeacon() {
    PLACEMENT placement, ptemp;
    placement.scorediff = 0;

    for (BEACON b : BeaconsToUse) {
        if (b >= FIRST_BEACON) {
            for (BTYPE t : TypesToUse) {
                ptemp = FindBestBeaconPosition(b, t);
                if (ptemp.scorediff > placement.scorediff + 0.001) {
                    placement = ptemp;
                }
            }
        }
        else if (b == BEACON_EMPTY) {
            ptemp = FindBestBeaconPosition(b, BTYPE_EMPTY);
            if (ptemp.scorediff > placement.scorediff + 0.001) {
                placement = ptemp;
            }
        }
    }
    return placement;
}

// Places the best found beacon on the beaconmap and typemap and updates effectmap and scoremap
bool PlaceBestBeacon() {
    const PLACEMENT p = FindBestBeacon();

    // If best beacon improves score, use it
    if (p.scorediff > 0) {
        if (bDebug && beaconmap[p.place.x][p.place.y] != BEACON_EMPTY) {
            cout << "Replaced " << beaconmap[p.place.x][p.place.y] << " type " << typemap[p.place.x][p.place.y] << " with " << p.beacon << " type " << p.type;
            cout << " in location " << p.place.x << " " << p.place.y << "\n";
        }
        // Update effect map and scoremap
        CalculateScoreDiff(p.beacon, p.type, beaconmap[p.place.x][p.place.y], typemap[p.place.x][p.place.y], p.place.x, p.place.y, true);
        // Update beacon map
        beaconmap[p.place.x][p.place.y] = p.beacon;
        typemap[p.place.x][p.place.y] = p.type;
        return true;
    }
    return false;
}


// Optimizes a map by finding the best beacon to place until no improvement can be found by replacing any beacon
// 
// This algorithm for example will not usually move a beacon for profid. To move a beacon it has to be profitably removed, and then a new beacon has to be profitably placed somewhere else
// This is often not possible, and these (possibly easy to see) improvements will be missed
int OptimizeByReplacement() {
    bool done = false;
    int stepcounter = 0;
    while (!done) {
        done = !PlaceBestBeacon(); // Will return false when it couldn't place anything

        // This is to get rid of built up rounding errors from floating poitn calculations
        // Not sure it is actually necessary
        stepcounter++;
        if (stepcounter > 1000) {
            RecalculateScoremaps();
            stepcounter = 0;
        }
    }

    return 0;
}

