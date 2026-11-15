#ifndef __MEMPDBREADER_H__
#define __MEMPDBREADER_H__

#include "game/ht.h"
#include "mpshared/pdbreader.h"

namespace wi {

//
// Reads pdbs from memory
//

class MemPdbReader : public PdbReader
{
public:
	MemPdbReader();
	~MemPdbReader();
	bool Open(const char *pszFn);

	virtual void Close();
	virtual bool GetRecordSize(word nRec, word *pcb);
	virtual bool ReadRecord(word nRec, word n, word cb, void *pv);
	virtual byte *MapRecord(word nRec, void **ppvCookie, word *pcb = NULL);
	virtual void UnmapRecord(word nRec, void *pvCookie);

private:
	bool GetRecordEntry(word nRec, int cRec, RecordEntryType *prece, dword *pcb);
	dword GetCompressionHeader(word nRec, CompressionHeader *pcoh);

	dword m_cb;
	byte *m_pb;
	word m_cRecs;
	word m_cMapped;
	CacheHandle *m_aphcRecordData;
};

} // namespace wi

#endif // __MEMPDBREADER_H__
