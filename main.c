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
	struct Node **Nodes;
};

struct Node {
	int Id;
	int numlinks;
	int visited;
	int pagerank;
	struct Node **links;
};

struct Node* MakeNode(int id)
{
	struct Node *n = malloc(sizeof(struct Node));
	n->Id = id;
	n->numlinks = 0;
	n->links = NULL;
	n->visited = 0;
	n->pagerank = 0;
	return n;
}

void PrintNodeStat(struct Node *n)
{
	printf("ID:\t\t%d\n", n->Id);
	printf("numlinks:\t%d\n", n->numlinks);
	printf("visited:\t%d\n", n->visited);
	printf("pagerank:\t%d\n", n->pagerank);
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
	index = rand() % g->numNodes;
	return g->Nodes[index];
}

double EstimatePageRank(struct Node* node, struct Graph* graph)
{
	int i;
	int sum = 0;
	int numNodes = graph->numNodes;
#pragma omp parallel for schedule(static,32) reduction(+:sum) private(i) shared(numNodes)
	for (i = 0; i < numNodes; i++) {
		sum += graph->Nodes[i]->visited;
	}

	if (sum <= 0) return 0;

	return (node->visited/sum);
}

int PageRank(struct Graph *graph, int length, double d_ratio)
{
	int i, j;
	int numNodes = graph->numNodes;
	int sum;
	struct drand48_data drandBuf;
	struct Node *node = NULL;

#pragma omp parallel private(j,i, drandBuf, node) shared(numNodes, length)
{
	srand48_r(PRIME*omp_get_thread_num(), &drandBuf);

#pragma omp for schedule(static,32)
	for (j = 0; j < numNodes; j++) {
		node = graph->Nodes[j];
		for (i = 0; i < length; i++) {
			#pragma omp atomic
			node->visited++;
			node = FindTarget(node, graph, d_ratio, drandBuf);
		}
	}
}
printf("Walking Complete\n");

#pragma omp parallel private(i) shared(numNodes)
{
#pragma omp for schedule(static,32) reduction(+:sum)
	for (i = 0; i < numNodes; i++) {
		sum += graph->Nodes[i]->visited;
	}

}

printf("Summing Complete %d\n", sum);
int pr;
#pragma omp parallel for schedule(static,32) private(i)
	for (i = 0; i < numNodes; i++) {
		pr = (graph->Nodes[i]->visited)/(sum - graph->Nodes[i]->visited);
		graph->Nodes[i]->pagerank = pr;
		if(pr > 0)
			PrintNodeStat(graph->Nodes[i]);
	}

printf("PageRank Complete\n");

return 0;
}

int main(int argc, char* argv[])
{
	int K;
	double D;
	struct Graph *g = malloc(sizeof(struct Graph));
	omp_set_num_threads(4);

	if (argc != 3) {
		printf("I take 2 parameters <K>: lenght of random walk, <D>: damping ratio\n");
		exit(1);
	}
	K = atoi(argv[1]);
	D = atof(argv[2]);
	printf("Walk distance %d, damping ratio %lf\n", K, D);

	//add all nodes graph and their respective refrences
	InitGraph(g);
	PageRank(g, K, D);

}
