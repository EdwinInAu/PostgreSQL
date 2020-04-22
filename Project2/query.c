// query.c ... query scan functions
// part of SIMC signature files
// Manage creating and using Query objects
// Written by John Shepherd, March 2020

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "bits.h"
#include "tsig.h"
#include "psig.h"
#include "bsig.h"

// check whether a query is valid for a relation
// e.g. same number of attributes

int checkQuery(Reln r, char *q)
{
	if (*q == '\0') return 0;
	char *c;
	int nattr = 1;
	for (c = q; *c != '\0'; c++)
		if (*c == ',') nattr++;
	return (nattr == nAttrs(r));
}

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q, char sigs)
{
	Query new = malloc(sizeof(QueryRep));
	assert(new != NULL);
	if (!checkQuery(r,q)) return NULL;
	new->rel = r;
	new->qstring = q;
	new->nsigs = new->nsigpages = 0;
	new->ntuples = new->ntuppages = new->nfalse = 0;
	new->pages = newBits(nPages(r));
	switch (sigs) {
	case 't': findPagesUsingTupSigs(new); break;
	case 'p': findPagesUsingPageSigs(new); break;
	case 'b': findPagesUsingBitSlices(new); break;
	default:  setAllBits(new->pages); break;
	}
	new->curpage = 0;
	return new;
}

// scan through selected pages (q->pages)
// search for matching tuples and show each
// accumulate query stats

void scanAndDisplayMatchingTuples(Query q) {
    assert(q != NULL);
    //TODO
    int pageIndex;
    int tupleIndex;
    int count;

    Reln relation = q->rel;
    assert(relation != NULL);
    // number of data pages
    Count npages = nPages(relation);
    // list of pages to examine
    Bits pages = q->pages;
    File file = dataFile(relation);
    Tuple queryString = malloc(strlen(q->qstring) + 2);
    strcpy(queryString, q->qstring);
    assert(queryString != NULL);
    pageIndex = 0;
    q->ntuples = 0;
    q->ntuppages = 0;
    q->nfalse = 0;

    for (pageIndex; pageIndex < npages; pageIndex++) {
        if (bitIsSet(pages, pageIndex) == FALSE) {
            continue;
        }
        Page currentPage = getPage(file, pageIndex)
        tupleIndex = 0;
        Count pageItems = pageNitems(currentPage);
        count = 0;
        for (tupleIndex; tupleIndex < pageItems; tupleIndex++) {
            Tuple currentTuple = getTupleFromPage(relation, currentPage, tupleIndex);
            if (tupleMatch(relation, currentTuple, queryString) == TRUE) {
                showTuple(relation, currentTuple);
                count++;
            }
            q->ntuples++;
        }
        if (count == 0) {
            q->nfalse++;
        }
        q->ntuppages++;
    }
    //free(queryString);
}

// print statistics on query

void queryStats(Query q)
{
	printf("# sig pages read:    %d\n", q->nsigpages);
	printf("# signatures read:   %d\n", q->nsigs);
	printf("# data pages read:   %d\n", q->ntuppages);
	printf("# tuples examined:   %d\n", q->ntuples);
	printf("# false match pages: %d\n", q->nfalse);
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	free(q->pages);
	free(q);
}

