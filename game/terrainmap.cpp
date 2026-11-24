#include "ht.h"
#include <vector>
#include <algorithm>

namespace wi {

/*
for (j = 0; j < 64; j += 8) {
        var str = "";
        for (i = 0; i < 8; i++)
                str += (j + i) * (j + i) + ", ";
        WScript.Echo(str);
}
*/

int ganSquared[256] = {
	0, 1, 4, 9, 16, 25, 36, 49,
	64, 81, 100, 121, 144, 169, 196, 225,
	256, 289, 324, 361, 400, 441, 484, 529,
	576, 625, 676, 729, 784, 841, 900, 961,
	1024, 1089, 1156, 1225, 1296, 1369, 1444, 1521,
	1600, 1681, 1764, 1849, 1936, 2025, 2116, 2209,
	2304, 2401, 2500, 2601, 2704, 2809, 2916, 3025,
	3136, 3249, 3364, 3481, 3600, 3721, 3844, 3969,
	4096, 4225, 4356, 4489, 4624, 4761, 4900, 5041,
	5184, 5329, 5476, 5625, 5776, 5929, 6084, 6241,
	6400, 6561, 6724, 6889, 7056, 7225, 7396, 7569,
	7744, 7921, 8100, 8281, 8464, 8649, 8836, 9025,
	9216, 9409, 9604, 9801, 10000, 10201, 10404, 10609,
	10816, 11025, 11236, 11449, 11664, 11881, 12100, 12321,
	12544, 12769, 12996, 13225, 13456, 13689, 13924, 14161,
	14400, 14641, 14884, 15129, 15376, 15625, 15876, 16129,
	16384, 16641, 16900, 17161, 17424, 17689, 17956, 18225,
	18496, 18769, 19044, 19321, 19600, 19881, 20164, 20449,
	20736, 21025, 21316, 21609, 21904, 22201, 22500, 22801,
	23104, 23409, 23716, 24025, 24336, 24649, 24964, 25281,
	25600, 25921, 26244, 26569, 26896, 27225, 27556, 27889,
	28224, 28561, 28900, 29241, 29584, 29929, 30276, 30625,
	30976, 31329, 31684, 32041, 32400, 32761, 33124, 33489,
	33856, 34225, 34596, 34969, 35344, 35721, 36100, 36481,
	36864, 37249, 37636, 38025, 38416, 38809, 39204, 39601,
	40000, 40401, 40804, 41209, 41616, 42025, 42436, 42849,
	43264, 43681, 44100, 44521, 44944, 45369, 45796, 46225,
	46656, 47089, 47524, 47961, 48400, 48841, 49284, 49729,
	50176, 50625, 51076, 51529, 51984, 52441, 52900, 53361,
	53824, 54289, 54756, 55225, 55696, 56169, 56644, 57121,
	57600, 58081, 58564, 59049, 59536, 60025, 60516, 61009,
	61504, 62001, 62500, 63001, 63504, 64009, 64516, 65025,
};

byte gmpdirdirOpposite[8] = { 4, 5, 6, 7, 0, 1, 2, 3 };

// Road
// Open
// Wall
// Blocked

// Extra cost of a tile with something the search avoids (a structure): it
// can still be pathed through, so blocked destinations can be approached
// (path following then stops at the blockage)

#define knScoreStructure 9000

// Through blocked terrain: only when a unit stands on it (no other way out)

#define knScoreBlocked 10000

// Cost of moving by one tile in a certain direction

word TerrainMap::s_anScoreVector[] = { 5, 7, 5, 7, 5, 7, 5, 7 };

// Longest path (in steps) the path arrays below can hold: every tile of the
// largest map

#define kcNodesMax (ktcMax * ktcMax)

// Rooms are kctRoom x kctRoom tiles

#define kctRoom 16
#define kshRoom 4

// Search work per update (tiles expanded), shared by all searches. A big
// search that doesn't fit carries on in the next update.

#define kcNodesPerUpdate 24000

// A* over the tile grid. Per tile state is valid when its stamp is the
// search's, so starting a search costs nothing however big the map.

struct PathHeapEntry {
	int nF;
	int nG;
	int off;
};

struct PathSearch {
	int *anG;
	dword *adwStamp;
	byte *abDir;		// direction it was reached by; kbDirStart; | kbClosed
	dword dwStamp;
	std::vector<PathHeapEntry> heap;

	int txFrom, tyFrom, txTo, tyTo;
	byte bfAvoid;
	bool fCorridor;
	bool fThroughBlocked;
	bool fActive;
	int cupdLast;
	int offBest;
	int nHBest;
};

#define kbDirStart 0x08
#define kbClosed 0x80

struct RoomNode {
	TCoord tx, ty;		// a tile of this part near its middle
	int iedgeFirst;
	int cedges;
};

static inline bool HeapLess(const PathHeapEntry &a, const PathHeapEntry &b)
{
	// Min-heap on f; ties prefer the deeper node (larger g)

	if (a.nF != b.nF)
		return a.nF > b.nF;
	return a.nG < b.nG;
}

static PathSearch *NewPathSearch(int cTiles)
{
	PathSearch *psrch = new PathSearch;
	psrch->anG = new int[cTiles];
	psrch->adwStamp = new dword[cTiles];
	psrch->abDir = new byte[cTiles];
	memset(psrch->adwStamp, 0, cTiles * sizeof(dword));
	psrch->dwStamp = 0;
	psrch->fActive = false;
	psrch->cupdLast = 0;
	return psrch;
}

static void DeletePathSearch(PathSearch *psrch)
{
	if (psrch == NULL)
		return;
	delete[] psrch->anG;
	delete[] psrch->adwStamp;
	delete[] psrch->abDir;
	delete psrch;
}

// Octile distance in path cost units: the straight-line estimate on a grid
// with diagonal moves

static inline int Octile(int dx, int dy)
{
	dx = dx < 0 ? -dx : dx;
	dy = dy < 0 ? -dy : dy;
	return dx > dy ? 5 * dx + 2 * dy : 5 * dy + 2 * dx;
}

// The tile search's estimate: the straight-line distance, weighted so the
// search heads for the goal instead of fanning out over every equally good
// tile. Paths come out about 10% longer than the very shortest (still well
// shorter than the original search's), for about a quarter of the work.

#define HEURISTIC_NUM 2
#define HEURISTIC_DEN 1

static inline int Estimate(int dx, int dy)
{
	return Octile(dx, dy) * HEURISTIC_NUM / HEURISTIC_DEN;
}

TerrainMap::TerrainMap()
{
	m_ptrmaph = NULL;
	m_pbTerrain = NULL;
	m_atptWall = NULL;
	m_cWalls = 0;
	m_abBuffer = NULL;
	m_awArea = NULL;
	m_abRoomPart = NULL;
	m_aiRoomNodeFirst = NULL;
	m_cRoomNodes = 0;
	m_arnod = NULL;
	m_aiRoomEdge = NULL;
	m_adwRoomMark = NULL;
	m_dwRoomMark = 0;
	m_psrchNow = NULL;
	m_psrchBig = NULL;
	m_cupdBudget = -1;
	m_cNodesBudget = kcNodesPerUpdate;
	m_fSearchPending = false;
}

TerrainMap::~TerrainMap()
{
	if (m_ptrmaph != NULL)
		gpakr.UnmapFile(&m_fmap);
	delete[] m_pbTerrain;
	delete[] m_atptWall;
	delete[] m_abBuffer;
	delete[] m_awArea;
	delete[] m_abRoomPart;
	delete[] m_aiRoomNodeFirst;
	delete[] m_arnod;
	delete[] m_aiRoomEdge;
	delete[] m_adwRoomMark;
	DeletePathSearch(m_psrchNow);
	DeletePathSearch(m_psrchBig);
}

bool TerrainMap::Init(char *pszFn)
{
	// Load the terrain map

	m_ptrmaph = (TerrainMapHeader *)gpakr.MapFile(pszFn, &m_fmap);
	if (m_ptrmaph == NULL)
		return false;

	// Alloc the path buffer

	m_ctx = BigWord(m_ptrmaph->ctx);
	m_cty = BigWord(m_ptrmaph->cty);
	m_cbBuffer = m_ctx * m_cty;
	m_abBuffer = new byte[m_cbBuffer];
	Assert(m_abBuffer != NULL, "out of memory!");
	if (m_abBuffer == NULL)
		return false;
	memset(m_abBuffer, 0, m_cbBuffer);

	// The terrain types, in a copy of our own. Walls are objects (WallGob):
	// their tiles are open ground here, blocked by the wall standing on them
	// like any structure, so they open when it is destroyed. The tiles the
	// level put walls on are kept for the level to place them.

	m_pbTerrain = new byte[m_cbBuffer];
	memcpy(m_pbTerrain, (byte *)(m_ptrmaph + 1), m_cbBuffer);
	m_cWalls = 0;
	for (int off = 0; off < m_cbBuffer; off++) {
		if (m_pbTerrain[off] == kttWall)
			m_cWalls++;
	}
	m_atptWall = new TPoint[m_cWalls > 0 ? m_cWalls : 1];
	int iWall = 0;
	for (int off = 0; off < m_cbBuffer; off++) {
		if (m_pbTerrain[off] == kttWall) {
			m_atptWall[iWall].tx = off % m_ctx;
			m_atptWall[iWall].ty = off / m_ctx;
			iWall++;
			m_pbTerrain[off] = kttOpen;
		}
	}

	// Init

	for (int n = 0; n < 8; n++)
		m_mpDirDelta[n] = g_mpDirToDy[n] * m_ctx + g_mpDirToDx[n];

	m_psrchNow = NewPathSearch(m_cbBuffer);
	m_psrchBig = NewPathSearch(m_cbBuffer);
	InitAreas();
	InitRooms();
	return true;
}

// Areas: flood fill of open terrain, 8-connected (units move diagonally
// between any two tiles)

void TerrainMap::InitAreas()
{
	m_awArea = new word[m_cbBuffer];
	memset(m_awArea, 0, m_cbBuffer * sizeof(word));
	std::vector<int> stack;
	int nArea = 0;
	for (int off = 0; off < m_cbBuffer; off++) {
		if (m_awArea[off] != 0 || !IsTerrainOpen(off))
			continue;
		if (nArea < 0xffff)
			nArea++;
		m_awArea[off] = (word)nArea;
		stack.push_back(off);
		while (!stack.empty()) {
			int offT = stack.back();
			stack.pop_back();
			int tx = offT % m_ctx, ty = offT / m_ctx;
			for (int n = 0; n < 8; n++) {
				int txN = tx + g_mpDirToDx[n], tyN = ty + g_mpDirToDy[n];
				if (txN < 0 || tyN < 0 || txN >= m_ctx || tyN >= m_cty)
					continue;
				int offN = tyN * m_ctx + txN;
				if (m_awArea[offN] != 0 || !IsTerrainOpen(offN))
					continue;
				m_awArea[offN] = (word)nArea;
				stack.push_back(offN);
			}
		}
	}
}

int TerrainMap::GetArea(TCoord tx, TCoord ty)
{
	if (tx < 0 || ty < 0 || tx >= m_ctx || ty >= m_cty)
		return 0;
	return m_awArea[ty * m_ctx + tx];
}

// Area of a tile, or for a blocked tile (a unit can stand at the edge of
// blocked terrain) the area of an open neighbour

int TerrainMap::AreaAround(int tx, int ty)
{
	int nArea = GetArea(tx, ty);
	if (nArea != 0)
		return nArea;
	for (int n = 0; n < 8; n++) {
		nArea = GetArea(tx + g_mpDirToDx[n], ty + g_mpDirToDy[n]);
		if (nArea != 0)
			return nArea;
	}
	return 0;
}

// The tile of an area nearest to a point, searching outward ring by ring

bool TerrainMap::FindNearestInArea(int txTo, int tyTo, int nArea, int *ptx, int *pty)
{
	int nMax = m_ctx > m_cty ? m_ctx : m_cty;
	for (int r = 1; r < nMax; r++) {
		int nBest = 0x7fffffff;
		for (int ty = tyTo - r; ty <= tyTo + r; ty++) {
			if (ty < 0 || ty >= m_cty)
				continue;
			bool fEdgeRow = (ty == tyTo - r || ty == tyTo + r);
			for (int tx = txTo - r; tx <= txTo + r; tx += fEdgeRow ? 1 : 2 * r) {
				if (tx < 0 || tx >= m_ctx)
					continue;
				if (m_awArea[ty * m_ctx + tx] != nArea)
					continue;
				int n = (tx - txTo) * (tx - txTo) + (ty - tyTo) * (ty - tyTo);
				if (n < nBest) {
					nBest = n;
					*ptx = tx;
					*pty = ty;
				}
			}
		}
		if (nBest != 0x7fffffff)
			return true;
	}
	return false;
}

// Rooms: split each room's open tiles into connected parts (within the
// room), then link parts that touch across room edges

void TerrainMap::InitRooms()
{
	m_ctxRoom = (m_ctx + kctRoom - 1) / kctRoom;
	m_ctyRoom = (m_cty + kctRoom - 1) / kctRoom;
	int cRooms = m_ctxRoom * m_ctyRoom;
	m_abRoomPart = new byte[m_cbBuffer];
	memset(m_abRoomPart, 0, m_cbBuffer);
	m_aiRoomNodeFirst = new int[cRooms + 1];
	m_adwRoomMark = new dword[cRooms];
	memset(m_adwRoomMark, 0, cRooms * sizeof(dword));

	std::vector<RoomNode> vnod;
	std::vector<int> stack;
	std::vector<long> vsum;
	for (int ry = 0; ry < m_ctyRoom; ry++) {
		for (int rx = 0; rx < m_ctxRoom; rx++) {
			int iroom = ry * m_ctxRoom + rx;
			m_aiRoomNodeFirst[iroom] = (int)vnod.size();
			int txL = rx * kctRoom, tyT = ry * kctRoom;
			int txR = _min(txL + kctRoom, m_ctx), tyB = _min(tyT + kctRoom, m_cty);
			int cParts = 0;
			for (int ty = tyT; ty < tyB; ty++) {
				for (int tx = txL; tx < txR; tx++) {
					int off = ty * m_ctx + tx;
					if (m_abRoomPart[off] != 0 || !IsTerrainOpen(off) || cParts >= 255)
						continue;
					cParts++;
					m_abRoomPart[off] = (byte)cParts;
					long sx = 0, sy = 0, c = 0;
					std::vector<int> vtiles;
					stack.push_back(off);
					while (!stack.empty()) {
						int offT = stack.back();
						stack.pop_back();
						vtiles.push_back(offT);
						int txT = offT % m_ctx, tyT2 = offT / m_ctx;
						sx += txT;
						sy += tyT2;
						c++;
						for (int n = 0; n < 8; n++) {
							int txN = txT + g_mpDirToDx[n], tyN = tyT2 + g_mpDirToDy[n];
							if (txN < txL || tyN < tyT || txN >= txR || tyN >= tyB)
								continue;
							int offN = tyN * m_ctx + txN;
							if (m_abRoomPart[offN] != 0 || !IsTerrainOpen(offN))
								continue;
							m_abRoomPart[offN] = (byte)cParts;
							stack.push_back(offN);
						}
					}

					// Representative tile: the part's tile nearest its middle

					int txM = (int)(sx / c), tyM = (int)(sy / c);
					int nBest = 0x7fffffff;
					RoomNode rnod;
					rnod.tx = (TCoord)txM;
					rnod.ty = (TCoord)tyM;
					for (size_t i = 0; i < vtiles.size(); i++) {
						int txT = vtiles[i] % m_ctx, tyT2 = vtiles[i] / m_ctx;
						int n = (txT - txM) * (txT - txM) + (tyT2 - tyM) * (tyT2 - tyM);
						if (n < nBest) {
							nBest = n;
							rnod.tx = (TCoord)txT;
							rnod.ty = (TCoord)tyT2;
						}
					}
					rnod.iedgeFirst = 0;
					rnod.cedges = 0;
					vnod.push_back(rnod);
				}
			}
		}
	}
	m_aiRoomNodeFirst[cRooms] = (int)vnod.size();
	m_cRoomNodes = (int)vnod.size();

	// Links between parts of neighbouring rooms

	std::vector<std::vector<int> > vadj(m_cRoomNodes);
	for (int ty = 0; ty < m_cty; ty++) {
		for (int tx = 0; tx < m_ctx; tx++) {
			int off = ty * m_ctx + tx;
			if (m_abRoomPart[off] == 0)
				continue;
			int nodeA = RoomNodeOf(tx, ty);
			for (int n = 0; n < 8; n++) {
				int txN = tx + g_mpDirToDx[n], tyN = ty + g_mpDirToDy[n];
				if (txN < 0 || tyN < 0 || txN >= m_ctx || tyN >= m_cty)
					continue;
				if (txN / kctRoom == tx / kctRoom && tyN / kctRoom == ty / kctRoom)
					continue;
				if (m_abRoomPart[tyN * m_ctx + txN] == 0)
					continue;
				int nodeB = RoomNodeOf(txN, tyN);
				std::vector<int> &v = vadj[nodeA];
				if (std::find(v.begin(), v.end(), nodeB) == v.end())
					v.push_back(nodeB);
			}
		}
	}
	int cedges = 0;
	for (int i = 0; i < m_cRoomNodes; i++)
		cedges += (int)vadj[i].size();
	m_arnod = new RoomNode[m_cRoomNodes > 0 ? m_cRoomNodes : 1];
	m_aiRoomEdge = new int[cedges > 0 ? cedges : 1];
	int iedge = 0;
	for (int i = 0; i < m_cRoomNodes; i++) {
		m_arnod[i] = vnod[i];
		m_arnod[i].iedgeFirst = iedge;
		m_arnod[i].cedges = (int)vadj[i].size();
		for (size_t j = 0; j < vadj[i].size(); j++)
			m_aiRoomEdge[iedge++] = vadj[i][j];
	}
}

int TerrainMap::RoomNodeOf(int tx, int ty)
{
	int nPart = m_abRoomPart[ty * m_ctx + tx];
	if (nPart == 0)
		return -1;
	return m_aiRoomNodeFirst[(ty / kctRoom) * m_ctxRoom + tx / kctRoom] + nPart - 1;
}

// Plans a rough route between room parts and marks the rooms along it, and
// the rooms around those, as the corridor the detailed search may use

bool TerrainMap::MarkCorridor(int txFrom, int tyFrom, int txTo, int tyTo)
{
	m_dwRoomMark++;
	if (m_dwRoomMark == 0) {
		memset(m_adwRoomMark, 0, m_ctxRoom * m_ctyRoom * sizeof(dword));
		m_dwRoomMark = 1;
	}

	std::vector<int> vroute;
	int nodeFrom = RoomNodeOf(txFrom, tyFrom);
	int nodeTo = RoomNodeOf(txTo, tyTo);
	if (nodeFrom < 0 || nodeTo < 0)
		return false;
	if (nodeFrom == nodeTo) {
		vroute.push_back(nodeFrom);
	} else {
		// A* over room parts

		static std::vector<int> s_anG, s_aiParent;
		static std::vector<dword> s_adwStamp;
		static dword s_dwStamp;
		if ((int)s_anG.size() < m_cRoomNodes) {
			s_anG.assign(m_cRoomNodes, 0);
			s_aiParent.assign(m_cRoomNodes, -1);
			s_adwStamp.assign(m_cRoomNodes, 0);
		}
		s_dwStamp++;
		std::vector<PathHeapEntry> heap;
		RoomNode *prnodTo = &m_arnod[nodeTo];
		s_anG[nodeFrom] = 0;
		s_aiParent[nodeFrom] = -1;
		s_adwStamp[nodeFrom] = s_dwStamp;
		PathHeapEntry e;
		e.nG = 0;
		e.nF = Octile(m_arnod[nodeFrom].tx - prnodTo->tx, m_arnod[nodeFrom].ty - prnodTo->ty);
		e.off = nodeFrom;
		heap.push_back(e);
		bool fFound = false;
		while (!heap.empty()) {
			std::pop_heap(heap.begin(), heap.end(), HeapLess);
			e = heap.back();
			heap.pop_back();
			if (e.nG > s_anG[e.off])
				continue;
			if (e.off == nodeTo) {
				fFound = true;
				break;
			}
			RoomNode *prnod = &m_arnod[e.off];
			for (int i = 0; i < prnod->cedges; i++) {
				int nodeN = m_aiRoomEdge[prnod->iedgeFirst + i];
				RoomNode *prnodN = &m_arnod[nodeN];
				int nG = e.nG + Octile(prnodN->tx - prnod->tx, prnodN->ty - prnod->ty);
				if (s_adwStamp[nodeN] == s_dwStamp && nG >= s_anG[nodeN])
					continue;
				s_adwStamp[nodeN] = s_dwStamp;
				s_anG[nodeN] = nG;
				s_aiParent[nodeN] = e.off;
				PathHeapEntry eN;
				eN.nG = nG;
				eN.nF = nG + Octile(prnodN->tx - prnodTo->tx, prnodN->ty - prnodTo->ty);
				eN.off = nodeN;
				heap.push_back(eN);
				std::push_heap(heap.begin(), heap.end(), HeapLess);
			}
		}
		if (!fFound)
			return false;
		for (int node = nodeTo; node >= 0; node = s_aiParent[node])
			vroute.push_back(node);
	}

	// Mark the route's rooms and their neighbours

	for (size_t i = 0; i < vroute.size(); i++) {
		RoomNode *prnod = &m_arnod[vroute[i]];
		int rx = prnod->tx / kctRoom, ry = prnod->ty / kctRoom;
		for (int dy = -1; dy <= 1; dy++) {
			for (int dx = -1; dx <= 1; dx++) {
				int rxT = rx + dx, ryT = ry + dy;
				if (rxT < 0 || ryT < 0 || rxT >= m_ctxRoom || ryT >= m_ctyRoom)
					continue;
				m_adwRoomMark[ryT * m_ctxRoom + rxT] = m_dwRoomMark;
			}
		}
	}

	// The rooms of the ends themselves (a blocked start isn't in a part)

	m_adwRoomMark[(tyFrom / kctRoom) * m_ctxRoom + txFrom / kctRoom] = m_dwRoomMark;
	m_adwRoomMark[(tyTo / kctRoom) * m_ctxRoom + txTo / kctRoom] = m_dwRoomMark;
	return true;
}


void TerrainMap::RefreshBudget()
{
	int cupd = gsim.GetUpdateCount();
	if (cupd != m_cupdBudget) {
		m_cupdBudget = cupd;
		m_cNodesBudget = kcNodesPerUpdate;
	}
}

#define knVerTerrainMapState 1
bool TerrainMap::LoadState(Stream *pstm)
{
	byte nVer = pstm->ReadByte();
	if (nVer != knVerTerrainMapState)
		return false;
	pstm->ReadBytesRLE(m_abBuffer, m_cbBuffer);
	return pstm->IsSuccess();
}

bool TerrainMap::SaveState(Stream *pstm)
{
	pstm->WriteByte(knVerTerrainMapState);
	for (int n = 0; n < m_cbBuffer; n++)
		m_abBuffer[n] &= kbfMobileUnit | kbfStructure | kbfReserved;
	pstm->WriteBytesRLE(m_abBuffer, m_cbBuffer);
	return pstm->IsSuccess();
}

void TerrainMap::SetFlags(int tx, int ty, int ctx, int cty, byte bf)
{
	Assert(tx >= 0 && ty >= 0);
	Assert(tx + ctx <= m_ctx && ty + cty <= m_cty);

	for (int txT = tx; txT < tx + ctx; txT++) {
		for (int tyT = ty; tyT < ty + cty; tyT++) {
			m_abBuffer[tyT * m_ctx + txT] |= bf;
		}
	}

}

bool TerrainMap::TestFlags(int tx, int ty, int ctx, int cty, byte bf)
{
	// We consider all tiles off the edge of the map to be fully occupied

	if (tx < 0 || ty < 0 || tx + ctx > m_ctx || ty + cty > m_cty)
		return true;

	for (int txT = tx; txT < tx + ctx; txT++) {
		for (int tyT = ty; tyT < ty + cty; tyT++) {
			int off = tyT * m_ctx + txT;
			if (m_abBuffer[off] & bf)
				return true;
		}
	}
	return false;
}

bool TerrainMap::IsOccupied(int tx, int ty, int ctx, int cty, byte bf)
{
	// We consider all tiles off the edge of the map to be fully occupied

	if (tx < 0 || ty < 0 || tx + ctx > m_ctx || ty + cty > m_cty)
		return true;

	for (int txT = tx; txT < tx + ctx; txT++) {
		for (int tyT = ty; tyT < ty + cty; tyT++) {
			int off = tyT * m_ctx + txT;
			if (m_abBuffer[off] & bf)
				return true;
			if (m_pbTerrain[off] > kttOpen)
				return true;
		}
	}
	return false;
}

void TerrainMap::ClearFlags(int tx, int ty, int ctx, int cty, byte bf)
{
	Assert(tx >= 0 && ty >= 0);
	Assert(tx + ctx <= m_ctx && ty + cty <= m_cty);

	for (int txT = tx; txT < tx + ctx; txT++) {
		for (int tyT = ty; tyT < ty + cty; tyT++) {
			m_abBuffer[tyT * m_ctx + txT] &= ~bf;
		}
	}

}

bool TerrainMap::IsLineOccupied(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, byte bfTerrainAvoid)
{
	// There already?

	if (txFrom == txTo && tyFrom == tyTo)
		return false;

	// Number of x's and y's to travel

	int dx = abs(txTo - txFrom);
	int dy = abs(tyTo - tyFrom);

	// Direction on each axis

	int sx, sy;
	if (txFrom < txTo) {
		sx = 1;
	} else {
		sx = -1;
	}
	if (tyFrom < tyTo) {
		sy = 1;
	} else {
		sy = -1;
	}

	TCoord txT = txFrom;
	TCoord tyT = tyFrom;
	int dc = 0;
	while (true) {
		if (dx > dy) {
			txT += sx;
			dc += dy;
			if (dc >= dx) {
				dc -= dx;
				tyT += sy;
			}
		} else {
			tyT += sy;
			dc += dx;
			if (dc >= dy) {
				dc -= dy;
				txT += sx;
			}
		}

		// If this is blocking, bail

		if (IsBlocked(txT, tyT, bfTerrainAvoid))
			return true;

		// If we're there, break

		if (txT == txTo && tyT == tyTo)
			break;
	}

	return false;
}

bool TerrainMap::FindFirstUnoccupied(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, TCoord *ptxFree, TCoord *ptyFree)
{
	// Number of x's and y's to travel

	int dx = abs(txTo - txFrom);
	int dy = abs(tyTo - tyFrom);

	// Direction on each axis

	int sx, sy;
	if (txFrom < txTo) {
		sx = 1;
	} else {
		sx = -1;
	}
	if (tyFrom < tyTo) {
		sy = 1;
	} else {
		sy = -1;
	}

	// If this is blocking, bail

	TCoord txT = txFrom;
	TCoord tyT = tyFrom;
	if (!IsBlocked(txT, tyT, 0)) {
		*ptxFree = txT;
		*ptyFree = tyT;
		return true;
	}
	if (txFrom == txTo && tyFrom == tyTo)
		return false;

	int dc = 0;
	while (true) {
		if (dx > dy) {
			txT += sx;
			dc += dy;
			if (dc >= dx) {
				dc -= dx;
				tyT += sy;
			}
		} else {
			tyT += sy;
			dc += dx;
			if (dc >= dy) {
				dc -= dy;
				txT += sx;
			}
		}

		// If this is blocking, bail

		if (!IsBlocked(txT, tyT, 0)) {
			*ptxFree = txT;
			*ptyFree = tyT;
			return true;
		}

		// If we're there, break

		if (txT == txTo && tyT == tyTo)
			break;
	}

	return false;
}

bool TerrainMap::FindLastUnoccupied(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, TCoord *ptxFree, TCoord *ptyFree)
{
	// Number of x's and y's to travel

	int dx = abs(txTo - txFrom);
	int dy = abs(tyTo - tyFrom);

	// Direction on each axis

	int sx, sy;
	if (txFrom < txTo) {
		sx = 1;
	} else {
		sx = -1;
	}
	if (tyFrom < tyTo) {
		sy = 1;
	} else {
		sy = -1;
	}

	// If this is not blocking, remember

	TCoord txT = txFrom;
	TCoord tyT = tyFrom;
	if (IsBlocked(txT, tyT, 0))
		return false;
	*ptxFree = txT;
	*ptyFree = tyT;
	if (txFrom == txTo && tyFrom == tyTo)
		return true;

	int dc = 0;
	while (true) {
		if (dx > dy) {
			txT += sx;
			dc += dy;
			if (dc >= dx) {
				dc -= dx;
				tyT += sy;
			}
		} else {
			tyT += sy;
			dc += dx;
			if (dc >= dy) {
				dc -= dy;
				txT += sx;
			}
		}

		// If this is not blocking, remember it

		if (IsBlocked(txT, tyT, 0))
			return true;
		*ptxFree = txT;
		*ptyFree = tyT;

		// If we're there, break

		if (txT == txTo && tyT == tyTo)
			break;
	}

	return true;
}

Path *TerrainMap::FindLinePath(TCoord txFrom, TCoord tyFrom, TCoord txTo, TCoord tyTo, byte bfTerrainAvoid)
{
	if (txFrom == txTo && tyFrom == tyTo)
		return NULL;

	TPoint atpt[ktcMax + ktcMax / 2];

	// Number of x's and y's to travel

	int dx = abs(txTo - txFrom);
	int dy = abs(tyTo - tyFrom);

	// Direction on each axis

	int sx, sy;
	if (txFrom < txTo) {
		sx = 1;
	} else {
		sx = -1;
	}
	if (tyFrom < tyTo) {
		sy = 1;
	} else {
		sy = -1;
	}

	TCoord txT = txFrom;
	TCoord tyT = tyFrom;
	int nStep = 0;
	int dc = 0;
	while (true) {
		if (dx > dy) {
			txT += sx;
			dc += dy;
			if (dc >= dx) {
				dc -= dx;
				tyT += sy;
			}
		} else {
			tyT += sy;
			dc += dx;
			if (dc >= dy) {
				dc -= dy;
				txT += sx;
			}
		}

		// If this is blocking, bail

		if (IsBlocked(txT, tyT, bfTerrainAvoid))
			return NULL;

		// Location looks good, save it

		atpt[nStep].tx = (byte)txT;
		atpt[nStep].ty = (byte)tyT;
		nStep++;
		Assert(nStep < ARRAYSIZE(atpt));

		// If we're there, break

		if (txT == txTo && tyT == tyTo)
			break;
	}

	// Create a path from this

	return CreatePath(this, txFrom, tyFrom, atpt, nStep);
}

void TerrainMap::StartSearch(PathSearch *psrch, int txFrom, int tyFrom, int txTo, int tyTo, byte bfAvoid, bool fCorridor, bool fThroughBlocked)
{
	psrch->dwStamp++;
	if (psrch->dwStamp == 0) {
		memset(psrch->adwStamp, 0, m_cbBuffer * sizeof(dword));
		psrch->dwStamp = 1;
	}
	psrch->heap.clear();
	psrch->txFrom = txFrom;
	psrch->tyFrom = tyFrom;
	psrch->txTo = txTo;
	psrch->tyTo = tyTo;
	psrch->bfAvoid = bfAvoid;
	psrch->fCorridor = fCorridor;
	psrch->fThroughBlocked = fThroughBlocked;
	psrch->fActive = true;
	psrch->cupdLast = gsim.GetUpdateCount();

	int off = tyFrom * m_ctx + txFrom;
	psrch->adwStamp[off] = psrch->dwStamp;
	psrch->anG[off] = 0;
	psrch->abDir[off] = kbDirStart;
	PathHeapEntry e;
	e.nG = 0;
	e.nF = Estimate(txTo - txFrom, tyTo - tyFrom);
	e.off = off;
	psrch->heap.push_back(e);
	psrch->offBest = off;
	psrch->nHBest = e.nF;
}

// Runs a search for at most cNodesMax tiles. Returns 1 found (the
// destination is closed), 0 not found (nothing left to try), -1 out of
// budget (call again to carry on).

#define knSearchFound 1
#define knSearchNotFound 0
#define knSearchBudget -1

int TerrainMap::RunSearch(PathSearch *psrch, int cNodesMax)
{
	byte *pbTerrain = m_pbTerrain;
	int offTo = psrch->tyTo * m_ctx + psrch->txTo;
	int cNodes = 0;
	std::vector<PathHeapEntry> &heap = psrch->heap;
	while (!heap.empty()) {
		if (cNodes >= cNodesMax) {
			m_cNodesBudget -= cNodes;
			return knSearchBudget;
		}
		std::pop_heap(heap.begin(), heap.end(), HeapLess);
		PathHeapEntry e = heap.back();
		heap.pop_back();
		int off = e.off;
		if (psrch->abDir[off] & kbClosed)
			continue;
		if (e.nG > psrch->anG[off])
			continue;
		psrch->abDir[off] |= kbClosed;
		cNodes++;
		if (off == offTo) {
			m_cNodesBudget -= cNodes;
			return knSearchFound;
		}

		int tx = off % m_ctx;
		int ty = off / m_ctx;
		int nH = e.nF - e.nG;
		if (nH < psrch->nHBest) {
			psrch->nHBest = nH;
			psrch->offBest = off;
		}

		for (int n = 0; n < 8; n++) {
			int txN = tx + g_mpDirToDx[n];
			int tyN = ty + g_mpDirToDy[n];
			if (txN < 0 || txN >= m_ctx || tyN < 0 || tyN >= m_cty)
				continue;
			int offN = off + m_mpDirDelta[n];

			int nCost = s_anScoreVector[n];
			if (pbTerrain[offN] > kttOpen && offN != offTo) {
				if (!psrch->fThroughBlocked)
					continue;
				nCost += knScoreBlocked;
			}
			if (psrch->fCorridor && m_adwRoomMark[(tyN >> kshRoom) * m_ctxRoom + (txN >> kshRoom)] != m_dwRoomMark)
				continue;
			if (m_abBuffer[offN] & psrch->bfAvoid)
				nCost += knScoreStructure;

			int nG = e.nG + nCost;
			if (psrch->adwStamp[offN] == psrch->dwStamp) {
				// Reached before: an open tile takes the shorter route. A
				// closed one is done (with the weighted estimate it may not
				// be the very shortest, but reopening tiles costs more than
				// it gains).

				if (psrch->abDir[offN] & kbClosed)
					continue;
				if (nG >= psrch->anG[offN])
					continue;
			} else {
				psrch->adwStamp[offN] = psrch->dwStamp;
			}
			psrch->anG[offN] = nG;
			psrch->abDir[offN] = (byte)n;
			PathHeapEntry eN;
			eN.nG = nG;
			eN.nF = nG + Estimate(psrch->txTo - txN, psrch->tyTo - tyN);
			eN.off = offN;
			heap.push_back(eN);
			std::push_heap(heap.begin(), heap.end(), HeapLess);
		}
	}
	m_cNodesBudget -= cNodes;
	return knSearchNotFound;
}

Path *TerrainMap::FindPath(int txFrom, int tyFrom, int txTo, int tyTo, byte bfTerrainAvoid)
{
	m_fSearchPending = false;

	// Arbitrary pathing

	if (txFrom == txTo && tyFrom == tyTo)
		return NULL;

	// Quick out. Miners do this when they want to get to the dump point but it's set because
	// the processor is occupied.

	if (abs(txFrom - txTo) <= 1 && abs(tyFrom - tyTo) <= 1) {
		TPoint tptT;
		tptT.tx = txTo;
		tptT.ty = tyTo;
		return CreatePath(this, txFrom, tyFrom, &tptT, 1);
	}

	RefreshBudget();

	// Can the destination be reached at all? If it's in another area (an
	// island, an enclosed spot, blocked terrain), head for the nearest tile
	// that can be reached instead of searching the whole map to find out.

	int nArea = AreaAround(txFrom, tyFrom);
	if (nArea != 0 && GetArea(txTo, tyTo) != nArea) {
		int txT, tyT;
		if (!FindNearestInArea(txTo, tyTo, nArea, &txT, &tyT))
			return NULL;
		txTo = txT;
		tyTo = tyT;
		if (txFrom == txTo && tyFrom == tyTo)
			return NULL;
		if (abs(txFrom - txTo) <= 1 && abs(tyFrom - tyTo) <= 1) {
			TPoint tptT;
			tptT.tx = txTo;
			tptT.ty = tyTo;
			return CreatePath(this, txFrom, tyFrom, &tptT, 1);
		}
	}

	// A big search for this already under way? Carry on with it.

	PathSearch *psrch = m_psrchBig;
	if (psrch->fActive && psrch->txFrom == txFrom && psrch->tyFrom == tyFrom &&
			psrch->txTo == txTo && psrch->tyTo == tyTo && psrch->bfAvoid == bfTerrainAvoid) {
		if (m_cNodesBudget <= 0) {
			m_fSearchPending = true;
			return NULL;
		}
		psrch->cupdLast = gsim.GetUpdateCount();
		int nResult = RunSearch(psrch, m_cNodesBudget);
		if (nResult == knSearchBudget) {
			m_fSearchPending = true;
			return NULL;
		}
		psrch->fActive = false;
		return MakePath(psrch, nResult == knSearchFound ? psrch->tyTo * m_ctx + psrch->txTo : psrch->offBest);
	}

	// This update's search work used up? Ask again next update.

	if (m_cNodesBudget <= 0) {
		m_fSearchPending = true;
		return NULL;
	}

	// Detailed search within a corridor of rooms along a rough route

	if (nArea != 0 && MarkCorridor(txFrom, tyFrom, txTo, tyTo)) {
		StartSearch(m_psrchNow, txFrom, tyFrom, txTo, tyTo, bfTerrainAvoid, true, false);
		int nResult = RunSearch(m_psrchNow, 0x7fffffff);
		m_psrchNow->fActive = false;
		if (nResult == knSearchFound)
			return MakePath(m_psrchNow, tyTo * m_ctx + txTo);
	}

	// The whole map, over as many updates as it takes. One at a time: if
	// another is under way (and its unit still asks for it), wait.

	if (psrch->fActive && gsim.GetUpdateCount() - psrch->cupdLast <= 2) {
		m_fSearchPending = true;
		return NULL;
	}
	StartSearch(psrch, txFrom, tyFrom, txTo, tyTo, bfTerrainAvoid, false, nArea == 0);
	int nResult = RunSearch(psrch, m_cNodesBudget > 0 ? m_cNodesBudget : 1);
	if (nResult == knSearchBudget) {
		m_fSearchPending = true;
		return NULL;
	}
	psrch->fActive = false;
	return MakePath(psrch, nResult == knSearchFound ? tyTo * m_ctx + txTo : psrch->offBest);
}

bool TerrainMap::IsBlocked(TCoord tx, TCoord ty, byte bf)
{
	int offMap = ty * m_ctx + tx;

	// We path through structures and blocked terrain (at very high cost) so that structures or
	// terrain that are "blocking" can at least be approached (albeit at high pathing cost)
	// as opposed to be totally blocked and never considered. Then at path following we determine
	// if there is blockage with this method.

	if (m_abBuffer[offMap] & bf)
		return true;
	if (m_pbTerrain[offMap] > kttOpen)
		return true;

	return false;
}

bool TerrainMap::GetFlags(int tx, int ty, byte *pbf)
{
	Assert(tx >= 0 && ty >= 0);
	Assert(tx < m_ctx && ty < m_cty);

	int offMap = ty * m_ctx + tx;
	if (m_pbTerrain[offMap] > kttOpen)
		return false;
	*pbf = m_abBuffer[offMap];
	return true;
}

Path *TerrainMap::MakePath(PathSearch *psrch, int off)
{
	static Direction adir[kcNodesMax];

	int offStart = psrch->tyFrom * m_ctx + psrch->txFrom;
	if (off == offStart)
		return NULL;
	Direction *pdirT = &adir[ARRAYSIZE(adir)];
	while (off != offStart) {
		Assert(psrch->adwStamp[off] == psrch->dwStamp);
		Direction dir = psrch->abDir[off] & kbfDirMask;
		pdirT--;
		*pdirT = dir;
		off -= m_mpDirDelta[dir];
	}

	return CreatePath(this, psrch->txFrom, psrch->tyFrom, pdirT, (int)(&adir[ARRAYSIZE(adir)] - pdirT));
}

//
// Path class
//

Path *CreatePath(TerrainMap *ptrmap, TCoord txStart, TCoord tyStart, TPoint *atpt, int ctpt)
{
	static Direction adir[kcNodesMax];

	TCoord txLast = txStart;
	TCoord tyLast = tyStart;
	for (int idir = 0; idir < ctpt; idir++) {
		adir[idir] = DirectionFromLocations(txLast, tyLast, atpt[idir].tx, atpt[idir].ty);
		txLast = atpt[idir].tx;
		tyLast = atpt[idir].ty;
	}

	return CreatePath(ptrmap, txStart, tyStart, adir, ctpt);
}

Path *CreatePath(TerrainMap *ptrmap, TCoord txStart, TCoord tyStart, Direction *adir, int cdir)
{
	Path *ppath = new Path;
	if (ppath == NULL)
		return NULL;
	if (!ppath->Init(ptrmap, txStart, tyStart, adir, cdir)) {
		delete ppath;
		return NULL;
	}
	return ppath;
}

Path::Path()
{
	m_cdirs = 0;
	m_adir = NULL;
}

Path::~Path()
{
	delete[] m_adir;
}

bool Path::Init(TerrainMap *ptrmap, TCoord txStart, TCoord tyStart, Direction *adir, int cdir)
{
	m_adir = new Direction[cdir];
	if (m_adir == NULL)
		return false;
	memcpy(m_adir, adir, cdir * sizeof(Direction));

	m_cdirs = cdir;
	m_txStart = txStart;
	m_tyStart = tyStart;

	// Init cache

	m_idirCache = -1;
	m_txCache = txStart;
	m_tyCache = tyStart;
	m_ptrmap = ptrmap;

	return true;
}

int Path::GetCount()
{
	return m_cdirs;
}

bool Path::GetPoint(int itpt, TPoint *ptpt, byte bf)
{
	if (itpt < 0 || itpt >= m_cdirs)
		return false;

	TCoord txT, tyT;
	CalcTo(itpt, &txT, &tyT);

	if (m_ptrmap->IsBlocked(txT, tyT, bf))
		return false;

	ptpt->tx = txT;
	ptpt->ty = tyT;
	return true;
}

bool Path::GetPointRaw(int itpt, TPoint *ptpt)
{
	if (itpt < 0 || itpt >= m_cdirs)
		return false;
	TCoord txT, tyT;
	CalcTo(itpt, &txT, &tyT);
	ptpt->tx = txT;
	ptpt->ty = tyT;
	return true;
}

void Path::SetCacheIndex(int idir)
{
	if (idir < 0 || idir >= m_cdirs)
		return;

	TCoord txT, tyT;
	CalcTo(idir, &txT, &tyT);
	m_idirCache = idir;
	m_txCache = txT;
	m_tyCache = tyT;
}

void Path::CalcTo(int idir, TCoord *ptx, TCoord *pty)
{
	if (idir < m_idirCache) {
		m_idirCache = -1;
		m_txCache = m_txStart;
		m_tyCache = m_tyStart;
		Assert(false);
	}

	int idirT = m_idirCache;
	TCoord txT = m_txCache;
	TCoord tyT = m_tyCache;
	while (true) {
		idirT++;
		if (idirT > idir)
			break;
		txT += g_mpDirToDx[m_adir[idirT]];
		tyT += g_mpDirToDy[m_adir[idirT]];
	}

	*ptx = txT;
	*pty = tyT;
}

int Path::FindClosestPoint(TCoord txFrom, TCoord tyFrom, int itptStart, int ctptFurtherStop, TPoint *ptptClosest)
{
	int ctptRemaining = m_cdirs - itptStart;

	int n2Last = 0x7fffffff;
	int nSmallest = 0x7fffffff;
	int jSmallest = -1;
	int ctptFurtherStopT = ctptFurtherStop;
	for (int j = 0; j < ctptRemaining; j++) {
		// Get location. False would mean it's blocked

		TPoint tpt;
		if (!GetPointRaw(j + itptStart, &tpt))
			break;

		// If we're on the path, return that

		if (tpt.tx == txFrom && tpt.ty == tyFrom) {
			*ptptClosest = tpt;
			return j + itptStart;
		}

		// Calc smallest distance, always advance if we get smaller

		int n2 = ganSquared[abs(tpt.tx - txFrom)] + ganSquared[abs(tpt.ty - tyFrom)];
		if (n2 <= nSmallest) {
			nSmallest = n2;
			jSmallest = j;
			*ptptClosest = tpt;
		}

		// If we've increased distance the last cFurtherAwayStop times, stop

		if (n2 > n2Last) {
			ctptFurtherStopT--;
			if (ctptFurtherStopT == 0)
				break;
		} else {
			ctptFurtherStopT = ctptFurtherStop;
		}
		n2Last = n2;
	}

	if (jSmallest == -1)
		return -1;
	return jSmallest + itptStart;
}

#define knVerPathState 4
bool Path::LoadState(TerrainMap *ptrmap, Stream *pstm)
{
	byte nVer = pstm->ReadByte();
	if (nVer != knVerPathState)
		return false;

	m_ptrmap = ptrmap;
	m_cdirs = pstm->ReadWord();
	m_txStart = pstm->ReadCoord();
	m_tyStart = pstm->ReadCoord();
	m_adir = new Direction[m_cdirs];
	if (m_adir == NULL)
		return false;
	m_idirCache = (int)(short)pstm->ReadWord();
	m_txCache = pstm->ReadCoord();
	m_tyCache = pstm->ReadCoord();

	byte bT;
	for (int idir = 0; idir < m_cdirs; idir++) {
		Direction dir;
		if ((idir & 1) == 0) {
			bT = pstm->ReadByte();
			dir = (bT & 0xf0) >> 4;
		} else {
			dir = bT & 0x0f;
		}
		m_adir[idir] = dir;
	}

	// Done

	return pstm->IsSuccess();
}

bool Path::SaveState(Stream *pstm)
{
	pstm->WriteByte(knVerPathState);
	pstm->WriteWord(m_cdirs);
	pstm->WriteCoord(m_txStart);
	pstm->WriteCoord(m_tyStart);
	pstm->WriteWord(m_idirCache);
	pstm->WriteCoord(m_txCache);
	pstm->WriteCoord(m_tyCache);

	// 2 directions per byte, smaller encoding

	byte bT = 0;
	for (int idir = 0; idir < m_cdirs; idir++) {
		Direction dir = m_adir[idir];
		if ((idir & 1) == 0) {
			bT = dir << 4;
			if (idir == m_cdirs - 1)
				pstm->WriteByte(bT);
		} else {
			bT |= dir;
			pstm->WriteByte(bT);
		}
	}

	// Done

	return pstm->IsSuccess();
}

#ifdef DRAW_PATHS
void Path::Draw(DibBitmap *pbm, int xView, int yView, Side side)
{
	TCoord txOld = m_txStart;
	TCoord tyOld = m_tyStart;

	for (int idir = 0; idir < m_cdirs; idir++) {
		Direction dir = m_adir[idir];
		TCoord txNew = txOld + g_mpDirToDx[dir];
		TCoord tyNew = tyOld + g_mpDirToDy[dir];
		DrawArrow(pbm, PcFromTc(txOld) - xView, PcFromTc(tyOld) - yView, dir, side);
		txOld = txNew;
		tyOld = tyNew;
	}

	// Draw 'X' at destination

	DrawArrow(pbm, PcFromTc(txOld) - xView, PcFromTc(tyOld) - yView, 8, side);
}
#endif

Direction DirectionFromLocations(TCoord txOld, TCoord tyOld, TCoord txNew, TCoord tyNew)
{
	Assert(abs(txOld - txNew) <= 1 && abs(tyOld - tyNew) <= 1);

	Direction dir;
	if (txNew < txOld) {
		if (tyNew < tyOld) {
			dir = kdirNW;
		} else if (tyNew > tyOld) {
			dir = kdirSW;
		} else {
			dir = kdirW;
		}
	} else if (txNew > txOld) {
		if (tyNew < tyOld) {
			dir = kdirNE;
		} else if (tyNew > tyOld) {
			dir = kdirSE;
		} else {
			dir = kdirE;
		}
	} else {
		if (tyNew < tyOld) {
			dir = kdirN;
		} else if (tyNew > tyOld) {
			dir = kdirS;
		} else {
			Assert(false);
			dir = kdirInvalid;
		}
	}

	return dir;
}

//
// TrackPoint class
//

bool TrackPoint::Init(Path *ppath, TCoord txFrom, TCoord tyFrom, int itptStart, int ctptFurtherStop)
{
	// Create a tracking point from txFrom, tyFrom to the path

	m_itptClosest = ppath->FindClosestPoint(txFrom, tyFrom, itptStart, ctptFurtherStop, &m_tptClosest);
	if (m_itptClosest == -1)
		return false;
	
	// Get the previous and next locations

	if (!ppath->GetPoint(m_itptClosest - 1, &m_tptBefore, 0))
		m_tptBefore = m_tptClosest;
	if (!ppath->GetPoint(m_itptClosest + 1, &m_tptAfter, 0))
		m_tptAfter = m_tptClosest;

	// Measure current distances to these locations. We'll measure
	// progress against these

	m_n2Before = ganSquared[abs(txFrom - m_tptBefore.tx)] + ganSquared[abs(tyFrom - m_tptBefore.ty)];
	m_n2After = ganSquared[abs(txFrom - m_tptAfter.tx)] + ganSquared[abs(tyFrom - m_tptAfter.ty)];
	m_tptInitial.tx = txFrom;
	m_tptInitial.ty = tyFrom;
	return true;
}

void TrackPoint::InitFrom(TrackPoint *ptrkp)
{
	m_tptInitial = ptrkp->m_tptInitial;
	m_tptClosest = ptrkp->m_tptClosest;
	m_tptBefore = ptrkp->m_tptBefore;
	m_tptAfter = ptrkp->m_tptAfter;
	m_itptClosest = ptrkp->m_itptClosest;
	m_n2Before = ptrkp->m_n2Before;
	m_n2After = ptrkp->m_n2After;
}

bool TrackPoint::IsProgress(TrackPoint *ptrkpA)
{
	// Returns true if this is closer than ptrkp
	// First compare closest path point. Longer is better "progress"

	TrackPoint *ptrkpB = this;
	if (ptrkpA->m_itptClosest < ptrkpB->m_itptClosest)
		return false;
	if (ptrkpA->m_itptClosest > ptrkpB->m_itptClosest)
		return true;		

	// Either same distance to before and closer to after, or further from before and same or greater to
	// after or closer to both before and after are recognized as "progress".

	if ((ptrkpA->m_n2Before == ptrkpB->m_n2Before && ptrkpA->m_n2After < ptrkpB->m_n2After) ||
			(ptrkpA->m_n2Before > ptrkpB->m_n2Before && ptrkpA->m_n2After <= ptrkpB->m_n2After) ||
			(ptrkpA->m_n2Before < ptrkpB->m_n2Before && ptrkpA->m_n2After < ptrkpB->m_n2After)) {
		return true;
	}
	return false;
}

bool TrackPoint::IsBetterSort(TrackPoint *ptrkpA)
{
	return IsProgress(ptrkpA);
}

} // namespace wi
