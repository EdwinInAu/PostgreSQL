// psig.c ... functions on page signatures (psig's)
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"

Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
    int i;
    Count numberOfAttributes = nAttrs(r);
    Count m = psigBits(r);
    Count k = codeBits(r);
    char **tupleValues = tupleVals(r, t);
    Bits pageSignature = newBits(m);
    i = 0;
    for(i; i < numberOfAttributes; i++){
        Bits codeWord = pageSigCodeword(tupleValues[i], m, k);
        orBits(pageSignature, codeWord);
    }
    return pageSignature;
}

// borrow from 7th lecture notes
bits pageSigCodeword(char *attr_value, Count m, Count k)
{
    int  nbits = 0;
    Bits cword = newBits(m);
    srandom(hash_any(attr_value, strlen(attr_value)));
    if (strcmp(attr_value, "?") != 0) {
        while (nbits < k) {
            int i = random() % m;
            if (((1 << i) & cword) == 0) {
                cword |= (1 << i);
                nbits++;
            }
        }
    }
    return cword;
}

void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);
	//TODO
    int pageId;
    int index;
    Reln relation = q->rel;
    Bits queryPageSignature = makePageSig(relation, q->qstring);
    Bits pages = q->pages;
    unsetAllBits(pages);
    File pageSignatureFile = psigFile(relation);
    Count pageSignaturePages = nPsigPages(relation);
    Count m = psigBits(r);
    pageID = 0;
    index = 0;
    for (pageId; pageId < pageSignaturePages; pageId++) {
        Page currentPage = getPage(pageSignatureFile, pageId);
        Count numberOfPageItems = pageNitems(currentPage);
        for (index; index < numberOfPageItems; index++){
            Bits tmp = newBits(m);
            getBits(currentPage, index, tmp);
            if(isSubset(queryPageSignature, tmp) == TRUE){
                // 这里有可能存在问题
                setBit(pages, pageId);
            }
            freeBits(tmp);
            q->nsigs++;
        }
        q->nsigpages++;
    }
}

