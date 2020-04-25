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
Bits makeTupleSig(Reln r, Tuple t) {
    assert(r != NULL && t != NULL);
    //TODO
    int i;
    Count numberOfAttributes = nAttrs(r);
    Count m = tsigBits(r);
    Count k = codeBits(r);
    char **tupleValues = tupleVals(r, t);
    Bits tupleSignature = newBits(m);
    unsetAllBits(tupleSignature);
    for (i = 0; i < numberOfAttributes; i++) {
        if (strcmp(tupleValues[i], "?") != 0) {
            Bits codeWord = tupleSigCodeword(tupleValues[i], m, k);
            orBits(tupleSignature, codeWord);
            freeBits(codeWord);
        }
    }
    return tupleSignature;
}

// borrow from 7th lecture notes
Bits tupleSigCodeword(char *attr_value, Count m, Count k) {
    int nbits = 0;
    Bits cword = newBits(m);
    unsigned int hash_value = hash_any(attr_value, strlen(attr_value));
    srandom(hash_value);
    while (nbits < k) {
        int i = random() % m;
        if (bitIsSet(cword, i) == FALSE) {
            setBit(cword, i);
            nbits++;
        }
    }
    return cword;
}

void findPagesUsingTupSigs(Query q) {
    assert(q != NULL);
    //TODO
    int pageId;
    int index;
    Reln relation = q->rel;
    Bits queryTupleSignature = makeTupleSig(relation, q->qstring);
    File tupleSignatureFile = tsigFile(relation);
    Count tupleSignaturePages = nTsigPages(relation);
    Count maxTupleSignaturesPP = maxTsigsPP(relation);
    Count m = tsigBits(relation);
    Count maxTuplesPP = maxTuplesPP(relation);

    for (pageId = 0; pageId < tupleSignaturePages; pageId++) {
        Page currentPage = getPage(tupleSignatureFile, pageId);
        Count numberOfPageItems = pageNitems(currentPage);
        for (index = 0; index < numberOfPageItems; index++) {
            Bits tmp = newBits(m);
            getBits(currentPage, index, tmp);
            q->nsigs++;
            if (isSubset(queryTupleSignature, tmp) == TRUE) {
                // 这里有可能存在问题
                setBit(q->pages, (index + pageId * maxTupleSignaturesPP) / maxTuplesPP);
            }
            freeBits(tmp);
        }
        q->nsigpages++;
    }
    // The printf below is primarily for debugging
    // Remove it before submitting this function
    printf("Matched Pages:");
    showBits(q->pages);
    putchar('\n');
}
