#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>

#define PRIME 1039

struct Graph {
	uint64_t numNodes;
	uint64_t numEdges;
	double WalkTime;
	double EstimateTime;
	struct Node **Nodes;
};

struct Node {
	int Id;
	int numlinks;
	int visited;
	double pagerank;
	struct Node **links;
};

struct Node* MakeNode(int id)
{
	struct Node *n = malloc(sizeof(struct Node));
	n->Id = id;
	n->numlinks = 0;
	n->links = NULL;
	n->visited = 0;
	n->pagerank = 0.0;
	return n;
}

void PrintNodeStat(struct Node *n)
{
	printf("ID:\t\t%d\n", n->Id);
	printf("numlinks:\t%d\n", n->numlinks);
	printf("visited:\t%d\n", n->visited);
	printf("pagerank:\t%e\n", n->pagerank);
	putchar('\n');
}

void PrintNodeStatLong(struct Node* n)
{
	int i;
	PrintNodeStat(n);
	for (i = 0; i < n->numlinks; i++) {
		printf("\t%d -> %d\n", n->Id, n->links[i]->Id);
	}
	putchar('\n');
}

void AddLink(struct Node *n, struct Node *link)
{
	n->links = realloc(n->links, sizeof(struct Node*) * (n->numlinks + 1));
	n->links[n->numlinks] = link;
	n->numlinks++;
}

//File Name hard coded here
int InitGraph(struct Graph *g)
{
	int i;
	FILE *f;
	char *line = NULL;
	char* tokens = NULL;
	size_t len = 0;
	int numNodes, numEdges;

	f = fopen("web-NotreDame_sorted.txt", "r");
	if (f == NULL) {
		printf("Cannot open input file\n");
		exit(1);
	}
	//read the 'Directed graph...' line
	getline(&line, &len, f);
	//read FromNodeId ToNodeID line
	getline(&line, &len, f);
	//read Nodes: Edges: line, will tokenize this one
	getline(&line, &len, f);
	//this is pretty fucking ugly, but so is file io in c...so
	tokens = strtok(line, " ");//#
	tokens = strtok(NULL, " ");//Nodes:
	tokens = strtok(NULL, " ");//<numNodes>
	numNodes = atoi(tokens);
	tokens = strtok(NULL, " ");//Edges
	tokens = strtok(NULL, " ");//<numEdges>
	numEdges = atoi(tokens);
	printf("NumNodes: %d NumEdges: %d\n", numNodes, numEdges);

	g->numNodes = numNodes;
	g->numEdges = numEdges;
	g->Nodes = malloc(sizeof(struct Node*) * numNodes);

	//add all the nodes to he graph
	for (i = 0; i < numNodes; i++) {
		g->Nodes[i] = MakeNode(i);
	}


	//read University line
	getline(&line, &len, f);
	//now we can start reading the graph
	int cur, prev, link;

	fscanf(f, "%d\t%d", &cur, &link);
	while (!feof(f)) {
		prev = cur;
		struct Node *n = g->Nodes[cur];
		while (cur == prev && !feof(f)) {
			AddLink(n, g->Nodes[link]);
			fscanf(f, "%d\t%d", &cur, &link);
		}
	}
	return 0;
}

struct Node* FindTarget(struct Node* n, struct Graph* g, double d_ratio, struct drand48_data drandBuf)
{
	// 0.0 to 1.0
	double coin;
	unsigned int seed = time(NULL);
	drand48_r(&drandBuf, &coin);
	int index = 0;
	if (coin <= d_ratio && n->numlinks != 0) {
		//heads
		index = rand_r(&seed) % n->numlinks;

		return n->links[index];
	}
	//else tails
	index = rand_r(&seed) % g->numNodes;
	return g->Nodes[index];
}

double EstimatePageRank(struct Node* node, struct Graph* graph)
{
	int i;
	int sum = 0;
	int numNodes = graph->numNodes;
	int chunkSize = graph->numNodes/4;

#pragma omp parallel for schedule(dynamic,chunkSize) reduction(+:sum) private(i) shared(numNodes)
	for (i = 0; i < numNodes; i++) {
		sum += graph->Nodes[i]->visited;
	}

	if (sum <= 0) return 0;

	return (node->visited/sum);
}

int PageRank(struct Graph *graph, int length, double d_ratio)
{
	int i, j;
	double start;
	int numNodes = graph->numNodes;
	int sum;
	int chunkSize = graph->numNodes/4;
	struct drand48_data drandBuf;
	struct Node *node = NULL;

	start =  omp_get_wtime();

	// Random walk every node in G for lenght steps
#pragma omp parallel private(j,i, drandBuf, node) shared(numNodes, length)
{
	srand48_r(PRIME*omp_get_thread_num(), &drandBuf);

#pragma omp for schedule(dynamic,chunkSize)
	for (j = 0; j < numNodes; j++) {
		node = graph->Nodes[j];
		for (i = 0; i < length; i++) {
			#pragma omp atomic
			node->visited++;
			node = FindTarget(node, graph, d_ratio, drandBuf);
		}
	}
}
	graph->WalkTime = omp_get_wtime() - start;
	start = omp_get_wtime();

	//compute the total number of visits accross all nodes
#pragma omp parallel private(i) shared(numNodes)
{
#pragma omp for schedule(dynamic,chunkSize) reduction(+:sum)
	for (i = 0; i < numNodes; i++) {
		sum += graph->Nodes[i]->visited;
	}

}

	//compute each nodes page rank from the previous sum
#pragma omp parallel for schedule(dynamic,chunkSize) private(i)
	for (i = 0; i < numNodes; i++) {
		graph->Nodes[i]->pagerank =
			((double)graph->Nodes[i]->visited)/((double)sum - (double)graph->Nodes[i]->visited);
	}
	graph->EstimateTime = omp_get_wtime() - start;


	return 0;
}

int main(int argc, char* argv[])
{
	int K;
	double D;
	int P;
	struct Graph *g = malloc(sizeof(struct Graph));

	if (argc != 4) {
		printf("I take 3 parameters <P>: Num Threads <K>: lenght of random walk, <D>: damping ratio\n");
		exit(1);
	}
	P = atoi(argv[1]);
	K = atoi(argv[2]);
	D = atof(argv[3]);
	omp_set_num_threads(P);

	//add all nodes graph and their respective refrences
	InitGraph(g);
	PageRank(g, K, D);
	printf("Walk Time: %lf\nEstimation Time: %lf\n", g->WalkTime, g->EstimateTime);

}
