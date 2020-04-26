// psig.c ... functions on page signatures (psig's)
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"
#include "bits.h"
#include "hash.h"

Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
    int i;
    // number of attributes of relation
    Count numberOfAttributes = nAttrs(r);
    // width of page signature
    Count m = psigBits(r);
    // bits set per attribute
    Count k = codeBits(r);
    char **tupleValues = tupleVals(r, t);
    Bits pageSignature = newBits(m);
    for(i = 0; i < numberOfAttributes; i++){
        Bits codeWord = pageSigCodeword(tupleValues[i], m, k);
        orBits(pageSignature, codeWord);
        freeBits(codeWord);
    }
    return pageSignature;
}

// borrow from 7th lecture notes
// make a bit changes
Bits pageSigCodeword(char *attr_value, Count m, Count k)
{
    int  nbits = 0;
    Bits cword = newBits(m);
    unsigned int hash_value = hash_any(attr_value, strlen(attr_value));
    srandom(hash_value);
    // if not '?'
    if (strcmp(attr_value, "?") != 0) {
        while (nbits < k) {
            int i = random() % m;
            if (bitIsSet(cword,i) == FALSE)
            {
                setBit(cword, i);
                nbits++;
            }
        }
    }
    return cword;
}

void findPagesUsingPageSigs(Query q) {
    assert(q != NULL);
    //TODO
    int pageId;
    int index;
    // get relation
    Reln relation = q->rel;
    // get page signature
    Bits queryPageSignature = makePageSig(relation, q->qstring);
    File pageSignatureFile = psigFile(relation);
    // number of psig pages
    Count pageSignaturePages = nPsigPages(relation);
    // width of page signature
    Count m = psigBits(relation);
    // max page signatures per page
    Count maxPageSigPP = maxPsigsPP(relation);
    // traverse page signature pages
    for (pageId = 0; pageId < pageSignaturePages; pageId++) {
        Page currentPage = getPage(pageSignatureFile, pageId);
        Count numberOfPageItems = pageNitems(currentPage);
        // traverse current page
        for (index = 0; index < numberOfPageItems; index++) {
            Bits tmp = newBits(m);
            // get current bits
            getBits(currentPage, index, tmp);
            // if page signature is equal to current bits
            if (isSubset(queryPageSignature, tmp) == TRUE) {
                // caculate corresponding data page id
                setBit(q->pages, pageId * maxPageSigPP + index);
            }
            freeBits(tmp);
            q->nsigs++;
        }
        q->nsigpages++;
    }
    freeBits(queryPageSignature);
}