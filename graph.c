/*
graph.c

Set of vertices and edges implementation.

Implementations for helper functions for graph construction and manipulation.

Skeleton written by Grady Fitzpatrick for COMP20007 Assignment 1 2021
*/
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "graph.h"
#include "utils.h"
#include "pq.h"

#define INITIALEDGES 32
#define NULLINT -1
#define NOTCHECKED 0
#define PREVCHECK 1
#define CURCHECK 2
#define OUTAGE 3
#define TRUE 1
#define FALSE 2
#define INF 9999999
#define LOCKED 1
#define UNLOCKED 0

/* Definition of a graph. */
struct graph {
  int numVertices;
  int numEdges;
  int allocedEdges;
  struct edge **edgeList;
  int** adj;
};

/* Definition of an edge. */
struct edge {
  int start;
  int outage;
  int end;
};

struct Subnet {
  int start;
  int size;
  int* nodeArray;
  int numSubnets;
};

struct t4 {
  int diameterSize;
  int diameterStart;
  int diameterEnd;
  int *diameterArray;
};

struct critical {
  int* critArray;
  int count;
};

struct graph *newGraph(int numVertices) {
  struct graph *g = (struct graph *)malloc(sizeof(struct graph));
  assert(g);
  /* Initialise edges. */
  g->numVertices = numVertices;
  g->numEdges = 0;
  g->allocedEdges = 0;
  g->edgeList = NULL;
  g->adj = NULL;
  return g;
}

/* Adds an edge to the given graph. */
void addEdge(struct graph *g, int start, int end) {
  assert(g);
  struct edge *newEdge = NULL;
  /* Check we have enough space for the new edge. */
  if ((g->numEdges + 1) > g->allocedEdges) {
    if (g->allocedEdges == 0) {
      g->allocedEdges = INITIALEDGES;
    }
    else {
      (g->allocedEdges) *= 2;
    }
    g->edgeList = (struct edge **)realloc(g->edgeList, sizeof(struct edge *) * g->allocedEdges);
    assert(g->edgeList);
  }

  /* Create the edge */
  newEdge = (struct edge *)malloc(sizeof(struct edge));
  assert(newEdge);
  newEdge->start = start;
  newEdge->outage = FALSE;
  newEdge->end = end;

  /* Add the edge to the list of edges. */
  g->edgeList[g->numEdges] = newEdge;
  (g->numEdges)++;
}


/* Frees all memory used by graph. */
void freeGraph(struct graph *g) {
  int i;
  for (i = 0; i < g->numEdges; i++) {
    free((g->edgeList)[i]);
  }
  if (g->edgeList) {
    free(g->edgeList);
  }
  free(g);
}

/* Finds:
  - Number of connected subnetworks (before outage) (Task 2)
  - Number of servers in largest subnetwork (before outage) (Task 3)
  - SIDs of servers in largest subnetwork (before outage) (Task 3)
  - Diameter of largest subnetworks (after outage) (Task 4)
  - Number of servers in path with largest diameter - should be one more than
    Diameter if a path exists (after outage) (Task 4)
  - SIDs in path with largest diameter (after outage) (Task 4)
  - Number of critical servers (before outage) (Task 7)
  - SIDs of critical servers (before outage) (Task 7)
*/
struct solution *graphSolve(struct graph *g, enum problemPart part,
                            int numServers, int numOutages, int *outages) {
  struct solution *solution = (struct solution *)
      malloc(sizeof(struct solution));
  assert(solution);
  /* Initialise solution values */
  initaliseSolution(solution);
  if (part == TASK_2) {
    solution->connectedSubnets = largestSubnet(g, numServers).numSubnets;
  }
  else if (part == TASK_3) {
    struct Subnet subnet = largestSubnet(g, numServers);
    solution->largestSubnet = subnet.size;

    sortArray(subnet.nodeArray, subnet.size);
    solution->largestSubnetSIDs = subnet.nodeArray;
  }
  else if (part == TASK_4) {
    doOutage(g, outages, numOutages);

    struct t4 t4return = task4(g, outages, numOutages);
    solution->postOutageDiameter = t4return.diameterSize;
    solution->postOutageDiameterCount = t4return.diameterSize + 1;
    solution->postOutageDiameterSIDs = t4return.diameterArray;
  }
  else if (part == TASK_7) {
    adjacencyArray(g);
    struct critical critReturn = getCritical(g);
    solution->criticalServerCount = critReturn.count;
    solution->criticalServerSIDs = critReturn.critArray;
  }
  return solution;
}



struct Subnet largestSubnet(struct graph *g, int numServers) {
  int i, j, k;
  
  struct Subnet returnStruct;
  returnStruct.size = NULLINT;
  returnStruct.numSubnets = 0;
  returnStruct.nodeArray = 0;

  
  // Array for storing if a vertice has been checked before
  int *checkedArray = malloc(g->numVertices * sizeof(int));

  int curSubnetStart = NULLINT;
  int curSubnetSize;

  // Initialise checkedArray
  for (i=0; i<g->numVertices; i++) {
    checkedArray[i] = NOTCHECKED;
  }

  // For each vertice
  for (i=0; i<g->numVertices; i++) {

    if (((curSubnetStart != NULLINT) && (checkedArray[i] == NOTCHECKED)) || (i == g->numVertices - 1)) {

      curSubnetSize = 0;
      for (j=0; j<g->numVertices; j++) {
        if (checkedArray[j] == CURCHECK) {
          curSubnetSize ++;
        }
      }

      
      // If current subnet bigger then maxsubnet, set new vals
      if (curSubnetSize > returnStruct.size) {
        returnStruct.size = curSubnetSize;
        returnStruct.start = curSubnetStart;

        // Reassign nodeArray
        free(returnStruct.nodeArray);
        returnStruct.nodeArray = malloc(curSubnetSize * sizeof(int));

        // Fill node array with nodes of val CURCHECK
        j = 0;
        while(j<curSubnetSize) {
          for (k=0; k<g->numVertices; k++) {
            if (checkedArray[k] == CURCHECK) {
              returnStruct.nodeArray[j] = k;
              j++;
            }
          }
        }
      }

      // Set all CURCHECK values to PREVCHECK
      for (j=0; j<g->numVertices; j++) {
        if (checkedArray[j] == CURCHECK) {
          checkedArray[j] = PREVCHECK;
        }
      }

      // Increment num subnets
      returnStruct.numSubnets++;
    }

    if ((checkedArray[i] == NOTCHECKED)) {

      curSubnetStart = i;
      checkedArray[i] = CURCHECK;

      // Get connections to subnet
      getConnections(g, i, checkedArray);
    }

  }

  free(checkedArray);
  return returnStruct;
  
}

void getConnections(struct graph *g, int vertice, int *checkedArray) {
  int i;
  int start;
  int end;
  int newVertice;

  // For edge in edgeList
  for (i=0; i<g->numEdges; i++) {

    if (g->edgeList[i]->outage == FALSE){

      // Initalise start and end int
      start = g->edgeList[i]->start;
      end = g->edgeList[i]->end;
      newVertice = NULLINT;

      // If start of edge is current vertice 
      if((start == vertice)) {
        newVertice = end;
      }
      // if end of edge is current vertice
      else if ((end == vertice)) {

        newVertice = start;
      }

      // If connection found
      if (newVertice != NULLINT) {

        // Check start of edge if ithasnt been checked yet
        if ((checkedArray[newVertice] == NOTCHECKED)) {
            checkedArray[newVertice] = CURCHECK;

            // Recursively call getConnections on new vertice known to be in current subnet
            getConnections(g, newVertice, checkedArray);
        }
      }
    }
  }
}


// Sets outage edges to TRUE
void doOutage(struct graph *g, int *outages, int numOutages) {
  int i;
  int j;


  // Sets edges with outages as TRUE
  for (i=0; i<numOutages; i++){
    for (j=0; j<g->numEdges; j++) {
      if ((g->edgeList[j]->start == outages[i]) || (g->edgeList[j]->end == outages[i])) {
        g->edgeList[j]->outage = TRUE;
      }
    }
  }
  
}


struct t4 task4(struct graph *g, int *outages, int numOutages) {
  int i;
  int j;
  int index;

  struct t4 t4return;
  t4return.diameterSize = NULLINT;
  t4return.diameterStart = INF;
  struct t4 returnSubnet;
  struct Subnet t4Subnet;
  struct Subnet curSubnet;
  curSubnet.start = NULLINT;

  int *checkedArray = malloc(g->numVertices * sizeof(int));

  // Initialise checkedArray
  for (i=0; i<g->numVertices; i++) {
    checkedArray[i] = NOTCHECKED;
    for (j=0; j<numOutages; j++) {
      if (i == outages[j]) {
        checkedArray[i] = OUTAGE;
      }
    }
  }

  // For each vertice
  for (i=0; i<g->numVertices; i++) {
    
    // If not already checked, finalise subnet and search for next subnet
    if (((checkedArray[i] == NOTCHECKED) && (curSubnet.start != NULLINT) && (checkedArray[i] != OUTAGE)) || (i == g->numVertices - 1)) {

      // Get size of this subnet
      curSubnet.size = 0;
      for (j=0; j<g->numVertices; j++) {
        if(checkedArray[j] == CURCHECK){
          curSubnet.size++;
        }
      }

    // Malloc nodeArray, for storing nodes of this subnet
    curSubnet.nodeArray = malloc(curSubnet.size * sizeof(int));

    // Add nodes of this subnet to nodeArray
    index = 0;
      for (j=0; j<g->numVertices; j++) {
        if(checkedArray[j] == CURCHECK){
          curSubnet.nodeArray[index] = j;
          index++;
          checkedArray[j] = PREVCHECK;
        }
      }

      // Find diameter of subnet
      returnSubnet = findDiameter(g, curSubnet);
      
      // Reassign t4 subnet if larger diameter found
      if ((returnSubnet.diameterSize > t4return.diameterSize ) || ((returnSubnet.diameterSize > t4return.diameterSize) && (returnSubnet.diameterStart < t4return.diameterStart))) {
          t4Subnet = curSubnet;
          t4return.diameterSize = returnSubnet.diameterSize;
          t4return.diameterStart = returnSubnet.diameterStart;
          t4return.diameterEnd = returnSubnet.diameterEnd;
      }

    }



    // If not already checked, and not an outage, search for new subnet
    if ((checkedArray[i] == NOTCHECKED) && (checkedArray[i] != OUTAGE)){
      curSubnet.start = i;
      checkedArray[i] = CURCHECK;
      getConnections(g, i, checkedArray);
    }

  }

  // Get shortes path, from largest diameter
  t4return.diameterArray = shortestPath(g, t4return, t4Subnet);

  return t4return;
}




// Insertion sort on int array of size arraySize
void sortArray(int *Array, int arraySize) {
  int i, j, temp;

  for (i=0; i<arraySize; i++) {
    j=i;
    while ( j > 0 && Array[j-1] > Array[j]) {	        
      temp = Array[j];
      Array[j] = Array[j-1];
      Array[j-1] = temp;
      j--;
    }
  }
}

struct t4 findDiameter(struct graph *g, struct Subnet curSubnet){
  int i, j, k;
  int curStart, curEnd;
  int size = curSubnet.size;

  sortArray(curSubnet.nodeArray, curSubnet.size); // Sort NodeArray

  struct t4 returnT4;
  returnT4.diameterSize = NULLINT;
  
  // Malloc 2D Graph array
  int (*matrix)[size];
  matrix = malloc(sizeof(*matrix) * size);
  matrix = malloc(sizeof(int[size][size]));

  // Fill 2D Graph
  for(i=0; i<size; i++) {
    curStart = curSubnet.nodeArray[i];
    for(j=0; j<size; j++) {
      curEnd = curSubnet.nodeArray[j];
      matrix[i][j] = INF;
      matrix[j][i] = INF;
      for(k=0; k<g->numEdges; k++) {
        if (((g->edgeList[k]->start == curStart) && (g->edgeList[k]->end == curEnd)) || ((g->edgeList[k]->start == curEnd) && (g->edgeList[k]->end == curStart))){
          matrix[i][j] = 1;
          matrix[j][i] = 1;
        }
        if (i == j) {
          matrix[i][j] = 0;
          matrix[j][i] = 0;
        }
      }
    }
      
  }

  // Shortest path algorithm, go through all edges. If we find shorter path on the way
  // To another node update shortest path
  for (k = 0; k < size; k++) {
    for (i = 0; i < size; i++) {
      for (j = 0; j < size; j++) {
        // If vertex k is on the shortest path from i to j update [i][j]
        if (matrix[i][j] > (matrix[i][k] + matrix[k][j]) && (matrix[k][j] != INF && matrix[i][k] != INF)) {
                    matrix[i][j] = matrix[i][k] + matrix[k][j];
            }
        }
    }
  }


  // Find largest diameter in adj matrix, first instance of largest diameter will
  // Have the lowest values
  for (i=0; i<size; i++) {
    for (j=0; j<size; j++) {
      if (matrix[i][j] > returnT4.diameterSize){
        returnT4.diameterSize = matrix[i][j];
        returnT4.diameterStart = curSubnet.nodeArray[i];
        returnT4.diameterEnd = curSubnet.nodeArray[j];
      }
    }
  }

  free(matrix);

  return returnT4;
}

int *shortestPath(struct graph *g, struct t4 diameter, struct Subnet subnet){
  int i, j;
  int *nodeIndex;
  int neighIndex;
  int curNode;
  int curNeigh;
  int altPath;
  int size = diameter.diameterSize + 1;
  struct pq *pq = newPQ();
  

  int *path = malloc(size * sizeof(int));
  int *nodeDist = malloc(subnet.size * sizeof(int));
  int *nodePrev = malloc(subnet.size * sizeof(int));

  // Sets all initial values, if diameter start nodeDist = 0
  for (i=0; i<subnet.size; i++) {
    nodeDist[i] = INF;
    nodePrev[i] = NULLINT;
    if (subnet.nodeArray[i] == diameter.diameterStart) {
      nodeDist[i] = 0;
    }

    // Queue nodes to PQ
    enqueue(pq, &i, nodeDist[i]);
  }


  // for size of subnet that largest diameter is in
  for (i=0; i<subnet.size; i++) {

    // Get min nodeDist
    nodeIndex = deletemin(pq);
    curNode = subnet.nodeArray[*nodeIndex]; // Gets curNode number

    // In edge List
    for (j=0; j<g->numEdges; j++) {
      
      curNeigh = NULLINT; // No neighbour exist by default

      // If edge not out
      if (g->edgeList[j]->outage != TRUE) {
        

        // If we find edge with curNode, reassign neighbour
        if(g->edgeList[j]->start == curNode) {
          curNeigh = g->edgeList[j]->end;
        }

        if(g->edgeList[j]->end == curNode) {
          curNeigh = g->edgeList[j]->start;
        }

        // if Neighbour is found
        if (curNeigh != NULLINT) {

          // Get indexs and distance
          neighIndex = arrayPos(subnet.nodeArray, curNeigh, subnet.size);
          altPath = nodeDist[arrayPos(subnet.nodeArray, curNode, subnet.size)] + 1;

          // If alt path is smaller, reassign prev node and node dist
          if (altPath < nodeDist[neighIndex]) {
            nodeDist[neighIndex] = altPath;
            nodePrev[neighIndex] = curNode;
          }
        }
      }
    }
  }


  // Iterate through prev array, to find shortest path to diameter.end
  j = subnet.nodeArray[arrayPos(subnet.nodeArray, diameter.diameterEnd, subnet.size)];
  for(i=diameter.diameterSize; i>=0; i--) {
    path[i] = j;
    j = nodePrev[arrayPos(subnet.nodeArray, j, subnet.size)];
  }

  return path;
}


// Gets array positon of a given number, used as arraypos does not match node number
int arrayPos(int *array, int item, int size){
  int i;

  for (i=0; i<size; i++){
    if (array[i] == item){
      return i;
    }
  }
  return NULLINT;
}

void adjacencyArray(struct graph *g) {
  int i;
  int j;
  g->adj =(int **)malloc(g->numVertices * sizeof(int *));
  for (i=0; i<g->numVertices; i++) {
    g->adj[i] = malloc((g->numVertices * sizeof(int *)));
  }

  // Fills adjacency with 0s
  for (i=0; i<g->numVertices; i++){
    for (j=0; j<g->numVertices; j++) {
      g->adj[i][j] = 0;
    }
  }

  // Fills connections in array
  for (i=0; i<g->numEdges; i++){
    g->adj[g->edgeList[i]->start][g->edgeList[i]->end] = 1;
    g->adj[g->edgeList[i]->end][g->edgeList[i]->start] = 1;
  }

}

struct critical getCritical(struct graph *g) {

  int i;
  int count = 0; // To track pushOrder
  
  // Order in which node was first found
  int* pushOrder = malloc(g->numVertices * sizeof(int));

  // Lowest push order of an ancestor to the node
  int* HRA = malloc(g->numVertices * sizeof(int));

  // True false if node visited
  int* visited = malloc(g->numVertices * sizeof(int));

  // Parent of node, node we came from to initially find child
  int* parent = malloc(g->numVertices * sizeof(int));

  // True false of if node is critical
  int* criticalNodes = malloc(g->numVertices * sizeof(int));


  // Initialise Arrays
  for(i=0; i<g->numVertices; i++) {
    pushOrder[i] = 0;
    HRA[i] = INF;
    visited[i] = FALSE;
    parent[i] = NULLINT;
    criticalNodes[i] = FALSE;
  }

  // For vertice
  for(i=0; i<g->numVertices; i++) {

    // If not visited, DFS search from node
    if (visited[i] == FALSE){
      DFS(g, pushOrder, HRA, visited, parent, criticalNodes, i, &count);
    }
  }

  // Initialise return array and count
  int* returnArray = malloc(g->numVertices * sizeof(int));
  int criticalCount = 0;

  // Insert criticalNodes into array
  for (i=0; i<g->numVertices; i++){
    if (criticalNodes[i] == TRUE) {
      returnArray[criticalCount++] = i;
    }
  }

  // return
  struct critical returnS;
  returnS.count = criticalCount;
  returnS.critArray = returnArray;

  return returnS;
 
}

void DFS(struct graph *g, int* pushOrder, int* HRA, int* visited, int* parent, int* criticalNodes, int curVer, int *count){
  int i;
  
  visited[curVer] = TRUE; // Set visisted
  pushOrder[curVer] = HRA[curVer] = ++*count; // Set push order and HRA for current vertex
  int children = 0; // Children of current Ver
  


  // For vertice
  for (i=0; i<g->numVertices; i++) {

    // If adj to current ver
    if (g->adj[curVer][i] == 1) {

      // And not visited
      if (visited[i] == FALSE) {
        children += 1;

        // Set parent
        parent[i] = curVer;
        // Search from new ver, i
        DFS(g, pushOrder, HRA, visited, parent, criticalNodes, i, count);

        // if child has lower HRA, set curVer HRA
        if (HRA[i] < HRA[curVer]) {
          HRA[curVer] = HRA[i];
        }

        // If no parent and more then 1 child, is root of DFS with more then 1 child. So must be critical
        if (parent[curVer] == NULLINT && children > 1) {
          criticalNodes[curVer] = TRUE;
        }

        // If has parent, and HRA is greater then push order of parent, node must be critical
        if (parent[curVer] != NULLINT && HRA[i] >= pushOrder[curVer]) {
          criticalNodes[curVer] = TRUE;
        }

      }
      // Else if parent is not current node
      else if (parent[curVer] != i) {
        
        // Reassign HRA[curVer] if pushorder[i] is less then
        if (pushOrder[i] < HRA[curVer]) {
          HRA[curVer] = pushOrder[i];
        }
      }

    }
  }
}