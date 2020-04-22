// bsig.c ... functions on Tuple Signatures (bsig's)
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "bsig.h"
#include "psig.h"

void findPagesUsingBitSlices(Query q)
{
	assert(q != NULL);
	//TODO
	Reln relation = q->rel;
	File bitSignatureFile = bsigFile(relation);
    Bits queryPageSignature = makePageSig(relation, q->qstring);
    Count maxBitSlicesPP = maxBsigsPP(relation);
    Count pm = psigBits(relation);
	Count bm = bsigBits(relation);
	Bits pages = newBits(bm);
    setAllBits(pages);
    int pageInit = -10;
    for(int index = 0; index < pm; idex++){
        if(bitIsSet(queryPageSignature, index) == TRUE) {
            int pageID = index / maxBitSlicesPP;
            if(pageID != pageInit){
                pageInit = pageID;
                q->nsigpages++;
            }
            Page currentPage = getPage(bitSignatureFile, pageID);
            Bits tmp = newBits(bm);
            Offset position = index % maxBitSlicesPP;
            getBits(currentPage, position, tmp);
            andBits(pages, tmp);
            freeBits(tmp);
            q->nsigs++;
        }
    }
    // 好像还缺个q->pages
}

