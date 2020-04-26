// reln.c ... functions on Relations
// part of SIMC signature files
// Written by John Shepherd, March 2020

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include "reln.h"
#include "page.h"
#include "tuple.h"
#include "tsig.h"
#include "psig.h"
#include "bits.h"
#include "hash.h"
// open a file with a specified suffix
// - always open for both reading and writing

File openFile(char *name, char *suffix)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.%s",name,suffix);
	File f = open(fname,O_RDWR|O_CREAT,0644);
	assert(f >= 0);
	return f;
}

// create a new relation (five files)
// data file has one empty data page

Status newRelation(char *name, Count nattrs, float pF,
                   Count tk, Count tm, Count pm, Count bm)
{
	Reln r = malloc(sizeof(RelnRep));
	RelnParams *p = &(r->params);
	assert(r != NULL);
	p->nattrs = nattrs;
	p->pF = pF,
	p->tupsize = 28 + 7*(nattrs-2);
	Count available = (PAGESIZE-sizeof(Count));
	p->tupPP = available/p->tupsize;
	p->tk = tk; 
	if (tm%8 > 0) tm += 8-(tm%8); // round up to byte size
	p->tm = tm; p->tsigSize = tm/8; p->tsigPP = available/(tm/8);
	if (pm%8 > 0) pm += 8-(pm%8); // round up to byte size
	p->pm = pm; p->psigSize = pm/8; p->psigPP = available/(pm/8);
	if (p->psigPP < 2) { free(r); return -1; }
	if (bm%8 > 0) bm += 8-(bm%8); // round up to byte size
	p->bm = bm; p->bsigSize = bm/8; p->bsigPP = available/(bm/8);
	if (p->bsigPP < 2) { free(r); return -1; }
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	addPage(r->dataf); p->npages = 1; p->ntups = 0;
	addPage(r->tsigf); p->tsigNpages = 1; p->ntsigs = 0;
	addPage(r->psigf); p->psigNpages = 1; p->npsigs = 0;
	addPage(r->bsigf); p->bsigNpages = 1; p->nbsigs = 0; // replace this
	// Create a file containing "pm" all-zeroes bit-strings,
    // each of which has length "bm" bits
	//TODO
    for (int index = 0; index < pm; index++){
        // get last bSig page index
        Count lastPageIndex = nBsigPages(r) - 1;
        File bitSignatureFile = bsigFile(r);
        Page currentPage = getPage(bitSignatureFile,lastPageIndex);
        // max bit-slices per page
        Count maxBsPP = maxBsigsPP(r);
        Bits bitSignature = newBits(bm);
        // if current page is full, add new bSig page
        if (pageNitems(currentPage) == maxBsPP) {
            addPage(r->bsigf);
            p->bsigNpages++;
            lastPageIndex++;
            free(currentPage);
            currentPage = newPage();
            if (currentPage == NULL) return NO_PAGE;
        }
        // put bit signature to current page and update page to the file
        putBits(currentPage, pageNitems(currentPage), bitSignature);
        addOneItem(currentPage);
        putPage(r->bsigf, lastPageIndex, currentPage);
        p->nbsigs++;
        freeBits(bitSignature);
    }

    closeRelation(r);
	return 0;
}

// check whether a relation already exists

Bool existsRelation(char *name)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.info",name);
	File f = open(fname,O_RDONLY);
	if (f < 0)
		return FALSE;
	else {
		close(f);
		return TRUE;
	}
}

// set up a relation descriptor from relation name
// open files, reads information from rel.info

Reln openRelation(char *name)
{
	Reln r = malloc(sizeof(RelnRep));
	assert(r != NULL);
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	read(r->infof, &(r->params), sizeof(RelnParams));
	return r;
}

// release files and descriptor for an open relation
// copy latest information to .info file
// note: we don't write ChoiceVector since it doesn't change

void closeRelation(Reln r)
{
	// make sure updated global data is put in info file
	lseek(r->infof, 0, SEEK_SET);
	int n = write(r->infof, &(r->params), sizeof(RelnParams));
	assert(n == sizeof(RelnParams));
	close(r->infof); close(r->dataf);
	close(r->tsigf); close(r->psigf); close(r->bsigf);
	free(r);
}

// insert a new tuple into a relation
// returns page where inserted
// returns NO_PAGE if insert fails completely

PageID addToRelation(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL && strlen(t) == tupSize(r));
	Page p;  PageID pid;
	RelnParams *rp = &(r->params);
	
	// add tuple to last page
	pid = rp->npages-1;
	p = getPage(r->dataf, pid);
	// check if room on last page; if not add new page
	if (pageNitems(p) == rp->tupPP) {
		addPage(r->dataf);
		rp->npages++;
		pid++;
		free(p);
		p = newPage();
		if (p == NULL) return NO_PAGE;
	}
	addTupleToPage(r, p, t);
	rp->ntups++;  //written to disk in closeRelation()
	putPage(r->dataf, pid, p);

    // compute tuple signature and add to tsigf

    //TODO
    // get tuple signature 
    Bits tupleSignature = makeTupleSig(r, t);
    // last page index of tsig pages
    Count lastPageIndexTs = nTsigPages(r) - 1;
    File tupleSignatureFile = tsigFile(r);
    // last page of tsig pages
    Page lastPageTs = getPage(tupleSignatureFile, lastPageIndexTs);
    // max tuple signatures per page
    Count maxTupleSignaturesPP = maxTsigsPP(r);
    // number of items of last page
    Count pageNumberOfItems = pageNitems(lastPageTs);

    // if the last page is full, add a new tsig page
    if (pageNumberOfItems == maxTupleSignaturesPP) {
        addPage(r->tsigf);
        free(lastPageTs);
        lastPageTs = newPage();
        if (lastPageTs == NULL) {
            return NO_PAGE;
        }
        lastPageIndexTs++;
        rp->tsigNpages++;
        // put tuple signature to this page and update page to tsig file
        putBits(lastPageTs, pageNitems(lastPageTs), tupleSignature);
        addOneItem(lastPageTs);
        putPage(r->tsigf, lastPageIndexTs, lastPageTs);
    } else {
        // put tuple signature to this page and update page to tsig file
        putBits(lastPageTs, pageNumberOfItems, tupleSignature);
        addOneItem(lastPageTs);
        putPage(r->tsigf, lastPageIndexTs, lastPageTs);
    }
    freeBits(tupleSignature);
    rp->ntsigs++;

    // compute page signature and add to psigf

    //TODO
    // get page signature 
    Bits pageSignature = makePageSig(r, t);
    // get number of data pages
    Count numberOfDataPages = nPages(r);
    // get number of psig
    Count numberOfPageSignatures = nPsigs(r);
    // max page signatures per page
    Count maxPageSignaturesPP = maxPsigsPP(r);
    File pageSignatureFile = psigFile(r);
    // number of psig pages
    Count numberOfPageSignaturePages = nPsigPages(r);
    // get last page index and last page
    Count lastPageIndexPs = numberOfPageSignaturePages - 1;
    Page lastPagePs = getPage(pageSignatureFile, lastPageIndexPs);
    Count lastPageItems = pageNitems(lastPagePs);
    Count m = psigBits(r);

    // if add new data page 
    if (numberOfDataPages != numberOfPageSignatures) {
        // if last psig page is full, add a new psig page
        if (lastPageItems == maxPageSignaturesPP) {
            addPage(r->psigf);
            rp->psigNpages++;
            lastPageIndexPs++;
            free(lastPagePs);
            lastPagePs = newPage();
            if (lastPagePs == NULL) return NO_PAGE;
        }
        // put page signature to this page and update page to tsig file
        putBits(lastPagePs, pageNitems(lastPagePs), pageSignature);
        addOneItem(lastPagePs);
        putPage(r->psigf, lastPageIndexPs, lastPagePs);
        rp->npsigs++;
    } else {
        // get the last item of last psig page, and 'or' operation
        Offset position = lastPageItems - 1;
        Bits tmp = newBits(m);
        getBits(lastPagePs, position, tmp);
        orBits(tmp, pageSignature);
        // put new page signature back
        putBits(lastPagePs, position, tmp);
        putPage(r->psigf, lastPageIndexPs, lastPagePs);
        freeBits(tmp);
    }
    freeBits(pageSignature);

    // use page signature to update bit-slices

    //TODO
    // get page signature
    Bits queryPageSignature = makePageSig(r, t);
    // get last page index of psig pages
    Count dataPageId = nPsigs(r) - 1;
    // max bit-slices per page
    Count maxBitSlicesPP = maxBsigsPP(r);
    File bitSignatureFile = bsigFile(r);
    Count bm = bsigBits(r);
    // traverse whole page signature 
    for (int i = 0; i < psigBits(r); i++) {
        if (bitIsSet(queryPageSignature, i) == TRUE) {
            // get current bsig page id
            int pageID = i / maxBitSlicesPP;
            Page currentPage = getPage(bitSignatureFile, pageID);
            Bits tm = newBits(bm);
            Offset pos = i % maxBitSlicesPP;
            // get current bit signature
            getBits(currentPage, pos, tm);
            // update data page id to bit signature
            setBit(tm, dataPageId);
            putBits(currentPage, pos, tm);
            putPage(r->bsigf, pageID, currentPage);
            freeBits(tm);
        }
    }
    freeBits(queryPageSignature);

    return nPages(r) - 1;
}

// displays info about open Reln (for debugging)

void relationStats(Reln r)
{
	RelnParams *p = &(r->params);
	printf("Global Info:\n");
	printf("Dynamic:\n");
    printf("  #items:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->ntups, p->ntsigs, p->npsigs, p->nbsigs);
    printf("  #pages:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->npages, p->tsigNpages, p->psigNpages, p->bsigNpages);
	printf("Static:\n");
    printf("  tups   #attrs: %d  size: %d bytes  max/page: %d\n",
			p->nattrs, p->tupsize, p->tupPP);
	printf("  sigs   bits/attr: %d\n", p->tk);
	printf("  tsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->tm, p->tsigSize, p->tsigPP);
	printf("  psigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->pm, p->psigSize, p->psigPP);
	printf("  bsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->bm, p->bsigSize, p->bsigPP);
}
