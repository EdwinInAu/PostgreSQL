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
	Bits matching = newBits(bm);
	setAllBits(matching);
    int pageInit = -10;
    for(int index = 0; index < pm; index++){
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
            andBits(matching, tmp);
            freeBits(tmp);
            // there is a problem
            q->nsigs++;
        }
    }

    for (int i = 0; i < nPages(relation); i++){
        if(bitIsSet(matching,i)){
            setBit(q->pages,i);
        }
    }
    // add
    freeBits(queryPageSignature);
    // add
    freeBits(matching);
    closeRelation(relation);
}

