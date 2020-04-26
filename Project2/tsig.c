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
    // number of attributes of relation
    Count numberOfAttributes = nAttrs(r);
    // width of tuple signature
    Count m = tsigBits(r);
    // bits set per attribute
    Count k = codeBits(r);
    char **tupleValues = tupleVals(r, t);
    Bits tupleSignature = newBits(m);
    unsetAllBits(tupleSignature);
    for (i = 0; i < numberOfAttributes; i++) {
        // if not '?'
        if (strcmp(tupleValues[i], "?") != 0) {
            Bits codeWord = tupleSigCodeword(tupleValues[i], m, k);
            orBits(tupleSignature, codeWord);
            freeBits(codeWord);
        }
    }
    return tupleSignature;
}

// borrow from 7th lecture notes
// make a bit changes
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
    // get relation
    Reln relation = q->rel;
    // get tuple signature
    Bits queryTupleSignature = makeTupleSig(relation, q->qstring);
    File tupleSignatureFile = tsigFile(relation);
    // number of tsig pages
    Count tupleSignaturePages = nTsigPages(relation);
    // max tuple signatures per page
    Count maxTupleSignaturesPP = maxTsigsPP(relation);
    // width of tuple signature
    Count m = tsigBits(relation);
    // max tuples per page
    Count maxTuplesPP = maxTupsPP(relation);
    // traverse tuple signature pages
    for (pageId = 0; pageId < tupleSignaturePages; pageId++) {
        Page currentPage = getPage(tupleSignatureFile, pageId);
        Count numberOfPageItems = pageNitems(currentPage);
        // traverse current page
        for (index = 0; index < numberOfPageItems; index++) {
            Bits tmp = newBits(m);
            // get current bits
            getBits(currentPage, index, tmp);
            q->nsigs++;
            // if tuple signature is equal to current bits
            if (isSubset(queryTupleSignature, tmp) == TRUE) {
                // caculate corresponding data page id
                setBit(q->pages, (index + pageId * maxTupleSignaturesPP) / maxTuplesPP);
            }
            freeBits(tmp);
        }
        q->nsigpages++;
    }
    freeBits(queryTupleSignature);
}
