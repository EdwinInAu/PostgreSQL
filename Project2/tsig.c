// tsig.c ... functions on Tuple Signatures (tsig's)
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"

// make a tuple signature

Bits makeTupleSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
	int i;
	Count numberOfAttributes = nAttrs(r);
	Count m = tsigBits(r);
	Count k = codeBits(r);
	char **tupleValues = tupleVals(r, t);
    Bits tupleSignature = newBits(m);
    i = 0;
    for(i; i < numberOfAttributes; i++){
        Bits codeWord = codeword(tupleValues[i], m, k);
        orBits(tupleSignature, codeWord);
    }
	return tupleSignature;
}

// borrow from 7th lecture notes
bits codeword(char *attr_value, Count m, Count k)
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

void findPagesUsingTupSigs(Query q)
{
	assert(q != NULL);
	//TODO
	int pageId;
	int index;
	Reln relation = q->rel;
	Bits querySignature = makeTupleSig(relation, q->qstring);
    Bits pages = q->pages;
    unsetAllBits(pages);
    File tupleSignatureFile = tsigFile(relation);
    // number of tsig pages
    Count tupleSignaturePages = nTsigPages(relation);
    // max tuple signatures per page
    // Count maxTupleSignaturesPP = maxTsigsPP(relation);
    Count m = tsigBits(r);
    pageID = 0;
    index = 0;
    for (pageId; pageId < tupleSignaturePages; pageId++) {
        Page currentPage = getPage(tupleSignatureFile, pageId);
        Count numberOfPageItems = pageNitems(currentPage);
        for (index; index < numberOfPageItems; index++){
            Bits tmp = newBits(m);
            getBits(currentPage, index, tmp);
            if(isSubset(querySignature, tmp) == TRUE){
                // 这里有可能存在问题
                setBit(pages, pageId);
            }
            q->nsigs++;
        }
        q->nsigpages++;
    }
    // The printf below is primarily for debugging
	// Remove it before submitting this function
	printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}
