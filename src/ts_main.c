#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "svstring.h"
#include "svtree.h"
#include "svgraph.h"
#include "svqueue.h"
#include "svset.h"

#define TS_VER "1.0.0"

#define SZ_HAS_A_LOOP "Graph has a loop."

P_ARRAY_Z gbl_parrnd = NULL;

int _grpCBFCompareInteger(const void * px, const void * py);

typedef struct st_Node
{
	size_t    id;
	wchar_t * word;
} NODE, * P_NODE;

struct IndegreeTable { size_t vid; BOOL visited;  size_t indegree; };

int cbfcmpWChar_t(const void * px, const void * py)
{
	if (*(wchar_t *)px > *(wchar_t *)py) return  1;
	if (*(wchar_t *)px < *(wchar_t *)py) return -1;
	return 0;
}

int cbftvsInit(void * pitem, size_t param)
{
	P_VERTEX_L pvtx = (P_VERTEX_L)pitem;

	struct IndegreeTable it;
	it.vid = pvtx->vid;
	it.visited = FALSE;
	it.indegree = grpIndegreeVertexL((P_GRAPH_L)1[(size_t *)param], pvtx->vid);

	setInsertT((P_SET_T)0[(size_t *)param], &it, sizeof(struct IndegreeTable), _grpCBFCompareInteger);
	
	return CBF_CONTINUE;
}

int cbftvs_topo_decrease(void * pitem, size_t param)
{
	P_EDGE pedg = (P_EDGE)pitem;
	P_SET_T pset = (P_SET_T)0[(size_t *)param];

	P_BSTNODE pnode = treBSTFindData_X(*pset, &pedg->vid, _grpCBFCompareInteger);

	if (NULL != pnode)
	{
		struct IndegreeTable * pit = (struct IndegreeTable *)pnode->knot.pdata;
		--pit->indegree;
	}
	return CBF_CONTINUE;
}

int cbftvs_topo_increase(void * pitem, size_t param)
{
	P_EDGE pedg = (P_EDGE)pitem;
	P_SET_T pset = (P_SET_T)0[(size_t *)param];

	P_BSTNODE pnode = treBSTFindData_X(*pset, &pedg->vid, _grpCBFCompareInteger);

	if (NULL != pnode)
	{
		struct IndegreeTable * pit = (struct IndegreeTable *)pnode->knot.pdata;
		++pit->indegree;
	}
	return CBF_CONTINUE;
}

void topo_puppet(P_GRAPH_L pgrp, P_SET_T pset, P_DEQUE_DL seq, size_t n);

int cbftvs_print_seq(void * pitem, size_t param)
{
	DWC4100(param);
	wprintf(L"%ls ", ((P_NODE)strLocateItemArrayZ(gbl_parrnd, sizeof(NODE), *(size_t *)((P_NODE_D)pitem)->pdata - 1))->word);
	return CBF_CONTINUE;
}

int cbftvstopo(void * pitem, size_t param)
{
	P_VERTEX_L pvtx = (P_VERTEX_L)pitem;
	P_DEQUE_DL seq = (P_DEQUE_DL)1[(size_t *)param];
	P_SET_T pset = (P_SET_T)0[(size_t *)param];
	P_GRAPH_L pgrp = (P_GRAPH_L)2[(size_t *)param];
	BOOL * pbf = (BOOL *)3[(size_t *)param];
	size_t n = 4[(size_t *)param];

	P_BSTNODE pnode = treBSTFindData_X(*pset, &pvtx->vid, _grpCBFCompareInteger);

	if (NULL != pnode)
	{
		struct IndegreeTable * pit = (struct IndegreeTable *)pnode->knot.pdata;
		if (0 == pit->indegree && FALSE == pit->visited)
		{
			grpTraverseVertexEdgesL(pgrp, pit->vid, cbftvs_topo_decrease, param);
			queInjectDL(seq, &pit->vid, sizeof(size_t));
			pit->visited = TRUE;
			topo_puppet(pgrp, pset, seq, n);

			pit->visited = FALSE;
			queEjectDL(NULL, sizeof(size_t), seq);
			grpTraverseVertexEdgesL(pgrp, pit->vid, cbftvs_topo_increase, param);

			*pbf = TRUE;
		}
	}
	
	return CBF_CONTINUE;
}

void topo_puppet(P_GRAPH_L pgrp, P_SET_T pset, P_DEQUE_DL seq, size_t n)
{
	size_t a[5];
	
	BOOL bfound = FALSE;

	a[0] = (size_t)pset;
	a[1] = (size_t)seq;
	a[2] = (size_t)pgrp;
	a[3] = (size_t)&bfound;
	a[4] = (size_t)n;

	grpTraverseVerticesL(pgrp, cbftvstopo, (size_t)a);

	if (FALSE == bfound)
	{
		strTraverseLinkedListDC_A(seq->pfirst, NULL, cbftvs_print_seq, 0, FALSE);
		puts("\n");
	}
}

void AllTopologicalSorts(P_GRAPH_L pgrp)
{
	P_SET_T pset;
	P_DEQUE_DL seq = queCreateDL();

	size_t a[2];

	pset = setCreateT();

	a[0] = (size_t)pset;
	a[1] = (size_t)pgrp;
	grpTraverseVerticesL(pgrp, cbftvsInit, (size_t)a);

	topo_puppet(pgrp, pset, seq, grpVerticesCountL(pgrp));

	setDeleteT(pset);
	queDeleteDL(seq);
}

void PrintVersion(void)
{
	puts("tsort");
	puts(TS_VER);
}

void PrintHelp(void)
{
	puts("tsort [OPTION] [FILE]");
	puts("\t[-h|--help] Print this helping text.");
	puts("\t[-v|--version] Print version.");
	puts("\t[-a|--all] Print all available topological sorts.");
}

void DestroyArrayZND(P_ARRAY_Z parrnd)
{
	size_t i;
	
	for (i = 0; i < strLevelArrayZ(parrnd); ++i)
	{
		if (0 == ((P_NODE)strLocateItemArrayZ(parrnd, sizeof(NODE), i))->id)
			break;
		free(((P_NODE)strLocateItemArrayZ(parrnd, sizeof(NODE), i))->word);
	}
	
	strDeleteArrayZ(parrnd);
}

int cbftvsRslt(void * pitem, size_t param)
{
	P_ARRAY_Z parrnd = (P_ARRAY_Z)param;
	
	wprintf(L"%ls\n", ((P_NODE)strLocateItemArrayZ(parrnd, sizeof(NODE), *(size_t *)pitem - 1))->word);
	
	return CBF_CONTINUE;
}

void Scanner(FILE * fp, BOOL b)
{
	P_GRAPH_L pg = grpCreateL();
	size_t i = 0, j = 0, k = 1;
	size_t va[2] = { 0 }, v;
	P_ARRAY_Z parrsz = strCreateArrayZ(BUFSIZ, sizeof(wchar_t));
	P_ARRAY_Z parrnd = strCreateArrayZ(BUFSIZ, sizeof(NODE));
	P_ARRAY_Z presult;
	P_TRIE_A pt = treCreateTrieA();
	
	while (!feof(fp))
	{
		wint_t wi = fgetwc(fp);
		wchar_t c = (wchar_t)wi;
		size_t * pr;
		switch (wi)
		{
		case WEOF:
		case L'\n':
		case L'\r':			
		case L' ':  /* Register a word. */
		case L'\t':
			if (0 == i)
				break;
			
			pr = treSearchTrieA(pt, parrsz->pdata, i, sizeof(wchar_t), cbfcmpWChar_t);
			
			if (NULL == pr)
			{
				NODE nd;
				nd.id = j + 1;
				nd.word = wcsdup((wchar_t *)parrsz->pdata);	/* Gotta free mem here, dude! */
				memcpy(strLocateItemArrayZ(parrnd, sizeof(NODE), j), &nd, sizeof(NODE));
				
				treInsertTrieA(pt, parrsz->pdata, i, sizeof(wchar_t), (v = nd.id), cbfcmpWChar_t);
				
				++j;
				if (j >= strLevelArrayZ(parrnd))
					strResizeBufferedArrayZ(parrnd, sizeof(NODE), +BUFSIZ);
				((P_NODE)strLocateItemArrayZ(parrnd, sizeof(NODE), j))->id = 0;
				
				grpInsertVertexL(pg, j);
			}
			else
			{
				v = *pr;
			}
			
			va[((k - 1) & 1)] = v; 
			
			if (0 == (k & 1))
			{	/* Register a pair. */
				grpInsertEdgeL(pg, va[0], va[1], 0);
			}
			
			++k;
			i = 0;
			
			break;
		default:
			*(wchar_t *)strLocateItemArrayZ(parrsz, sizeof(wchar_t), i) = c;
			++i;
			if (i >= strLevelArrayZ(parrsz))
				strResizeBufferedArrayZ(parrsz, sizeof(wchar_t), +BUFSIZ);
			*(wchar_t *)strLocateItemArrayZ(parrsz, sizeof(wchar_t), i) = L'\0';
		}
	}
	
	if (!b)
	{
		presult = grpTopologicalSortL(pg);
		if (NULL != presult)
		{
			if (strLevelArrayZ(presult) == j)
				strTraverseArrayZ(presult, sizeof(size_t), cbftvsRslt, (size_t)parrnd, FALSE);
			else
				puts(SZ_HAS_A_LOOP);
			strDeleteArrayZ(presult);
		}
		else
		{
			puts(SZ_HAS_A_LOOP);
		}
	}
	else
	{
		presult = grpTopologicalSortL(pg);
		if (NULL != presult)
		{
			strDeleteArrayZ(presult);
			gbl_parrnd = parrnd;
			AllTopologicalSorts(pg);
			wprintf(L"\n");
		}
		else
		{
			puts(SZ_HAS_A_LOOP);
		}
	}
	
	strDeleteArrayZ(parrsz);
	treDeleteTrieA(pt, sizeof(wchar_t));
	DestroyArrayZND(parrnd);
	grpDeleteL(pg);
}

int main(int argc, char ** argv)
{
	BOOL b = FALSE;
	FILE * fpin = stdin;
	
	if (argc >= 2)
	{
		if (0 == strcmp(argv[1], "-v"))
		{
			PrintVersion();
		}
		else if (0 == strcmp(argv[1], "-h"))
		{
			PrintHelp();
		}
		else if (0 == strcmp(argv[1], "--version"))
		{
			PrintVersion();
		}
		else if (0 == strcmp(argv[1], "--help"))
		{
			PrintHelp();
		}
		else if (0 == strcmp(argv[1], "--all"))
		{
			b = TRUE;
		}
		else if (0 == strcmp(argv[1], "-a"))
		{
			b = TRUE;
		}
		else if (2 == argc)
		{
			fpin = fopen(argv[1], "r");
			if (NULL == fpin)
				return 1;
		}
		if (3 == argc)
		{
			fpin = fopen(argv[2], "r");
			if (NULL == fpin)
				return 1;
		}
		
		Scanner(fpin, b);
		
		if (fpin != stdin)
			fclose(fpin);
	}
	return 0;
}

