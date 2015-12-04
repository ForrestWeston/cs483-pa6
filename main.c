#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>

struct Graph {
	uint64_t numNodes;
	uint64_t numEdges;
	struct Node **Nodes;
};

struct Node {
	int Id;
	int numlinks;
	struct Node **links;
};

struct Node* MakeNode(int id)
{
	struct Node *n = malloc(sizeof(struct Node));
	n->Id = id;
	n->numlinks = 0;
	n->links = NULL;
	return n;
}

void PrintNodeStats(struct Node *n)
{
	int i;
	printf("ID:\t%d\n", n->Id);
	printf("numlinks:\t%d\n", n->numlinks);
	for (i = 0; i < n->numlinks; i++) {
		printf("\t%d -> %d\n", n->Id, n->links[i]->Id);
	}
}

void AddLink(struct Node *n, int link)
{
	n->links = realloc(n->links, sizeof(struct Node*) * (n->numlinks + 1));
	n->links[n->numlinks] = MakeNode(link);
	n->numlinks++;
}

//File Name hard coded here
int InitGraph(struct Graph *g)
{
	FILE *f;
	char *line = NULL;
	char* tokens = NULL;
	size_t len = 0;
	int numNodes, numEdges;
	g = malloc(sizeof(struct Graph));

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

	//read University line
	getline(&line, &len, f);
	//now we can start reading the graph
	int cur, prev, link;

	fscanf(f, "%d\t%d", &cur, &link);
	while (!feof(f)) {
		prev = cur;
		struct Node *n = MakeNode(cur);
		while (cur == prev && !feof(f)) {
			printf("Current Node: %d, adding link %d\n", cur, link);
			AddLink(n, link);
			fscanf(f, "%d\t%d", &cur, &link);
		}
	}
	PrintNodeStats(g->Nodes[0]);
	return 0;
}

int main(int argc, char* argv[])
{
	int K;
	double D;
	struct Graph *g = malloc(sizeof(struct Graph));

	if (argc != 3) {
		printf("I take 2 parameters <K>: lenght of random walk, <D>: damping ratio\n");
		exit(1);
	}
	K = atoi(argv[1]);
	D = atof(argv[2]);
	printf("Walk distance %d, damping ratio %lf\n", K, D);

	InitGraph(g);

}
