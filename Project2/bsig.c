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
    // get relation
	Reln relation = q->rel;
	File bitSignatureFile = bsigFile(relation);
    // get page signature
    Bits queryPageSignature = makePageSig(relation, q->qstring);
    // max bit-slices per page
    Count maxBitSlicesPP = maxBsigsPP(relation);
    // width of page signature
    Count pm = psigBits(relation);
    // width of bit-slice
	Count bm = bsigBits(relation);
    // matching pages
	Bits matching = newBits(bm);
	setAllBits(matching);
    int pageInit = -10;
    // travese whole page signature
    for(int index = 0; index < pm; index++){
        // when the position where page signature is set 1
        if(bitIsSet(queryPageSignature, index) == TRUE) {
            // calculate page id
            int pageID = index / maxBitSlicesPP;
            // check whether it's the same page id as that in last loop
            if(pageID != pageInit){
                pageInit = pageID;
                q->nsigpages++;
            }
            Page currentPage = getPage(bitSignatureFile, pageID);
            Bits tmp = newBits(bm);
            // calculate position in the current page
            Offset position = index % maxBitSlicesPP;
            getBits(currentPage, position, tmp);
            // 'and'
            andBits(matching, tmp);
            freeBits(tmp);
            q->nsigs++;
        }
    }
    // update matching page id to query pages
    for (int i = 0; i < nPages(relation); i++){
        if(bitIsSet(matching,i)){
            setBit(q->pages,i);
        }
    }
    freeBits(queryPageSignature);
    freeBits(matching);
}

