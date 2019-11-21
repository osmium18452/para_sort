#include <iostream>
#include <fstream>
#include <cstring>
#include <mpi.h>
#include <algorithm>
#include <ctime>

using namespace std;

int main(int argc, char **argv)
{
	ios::sync_with_stdio(false);
	ifstream in(argv[1], ios::in);

	if (strcmp(argv[2], "nooutput") != 0) ofstream out(argv[2], ios::out);
	else ofstream out(argv, ios::app);

	MPI_Init(&argc, &argv);

	int size, rank, dataSize, processDataSize;
	int *forSorting;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//Read input files.
	clock_t readStart = clock();
	if (rank == 0) {
		printf("READING FROM %s\n", argv[1]);
		in >> dataSize;
	}
	MPI_Bcast(&dataSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

	forSorting = (int *) malloc(dataSize * sizeof(int));

	if (rank == 0) {
		for (int i = 0; i < dataSize; i++) {
			in >> forSorting[i];
		}
	}
	clock_t readEnd = clock();
	if (rank == 0) {
		printf("READING PROCESS TAKES %.3lf SECONDS.\n", (double) (readEnd - readStart) / CLOCKS_PER_SEC);
	}

	//Begin sorting...
	clock_t sortStart = clock();
	MPI_Bcast(forSorting, dataSize, MPI_INT, 0, MPI_COMM_WORLD);

	processDataSize = dataSize / size + (rank < dataSize % size);

	int begin = dataSize / size * rank + min(rank, dataSize % size);
	int end = begin + processDataSize;

	//Each process sort their own data.
	sort(forSorting + begin, forSorting + end);

	int *devItem = (int *) malloc((size - 1) * sizeof(int));
	int pivoSize = min(size, processDataSize);
	int *pivotalInProcess = (int *) malloc(pivoSize * sizeof(int));
	int cnt = 0;
	for (int i = begin; i < end; i += processDataSize / (pivoSize)) {
		pivotalInProcess[cnt++] = forSorting[i];
	}

	//select the pivotal elements.
	int *allPivotalCount = (int *) malloc(size * sizeof(int));
	int *allPivotalDp = (int *) malloc(size * sizeof(int));
	int *allPivotal;
	int allPivotalSize;

	MPI_Gather(&pivoSize, 1, MPI_INT, allPivotalCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0) {
		allPivotalDp[0] = 0;
		for (int i = 1; i < size; i++) {
			allPivotalDp[i] = allPivotalDp[i - 1] + allPivotalCount[i - 1];
		}
		allPivotalSize = allPivotalDp[size - 1] + allPivotalCount[size - 1];
		allPivotal = (int *) malloc(allPivotalSize * sizeof(int));
	}

	MPI_Gatherv(pivotalInProcess, pivoSize, MPI_INT, allPivotal, allPivotalCount, allPivotalDp, MPI_INT, 0,
	            MPI_COMM_WORLD);
	//receive pivotal element from all of the other processors
	if (rank == 0) {
		sort(allPivotal, allPivotal + allPivotalSize);
		int step = allPivotalSize / size;
		for (int i = step, cnt = 0; i < allPivotalSize; i += step) {
			devItem[cnt++] = allPivotal[i];
		}
		free(allPivotalDp);
		free(allPivotalCount);
		free(allPivotal);
	}
	MPI_Bcast(devItem, size - 1, MPI_INT, 0, MPI_COMM_WORLD);

	int *sendCount = (int *) malloc(size * sizeof(int));
	int *sendDisp = (int *) malloc(size * sizeof(int));
	int *recvCount = (int *) malloc(size * sizeof(int));
	int *recvDisp = (int *) malloc(size * sizeof(int));
	memset(sendCount, 0, size * sizeof(int));
	for (int i = begin, cnt = 0; i < end; i++) {
		if (cnt == size - 1) {
			sendCount[cnt] = end - i;
			break;
		}
		if (forSorting[i] <= devItem[cnt]) {
			sendCount[cnt]++;
		} else {
			cnt++, i--;
		}
	}

	MPI_Alltoall(sendCount, 1, MPI_INT,
	             recvCount, 1, MPI_INT, MPI_COMM_WORLD);
	sendDisp[0] = begin;
	recvDisp[0] = 0;
	for (int i = 1; i < size; i++) {
		sendDisp[i] = sendDisp[i - 1] + sendCount[i - 1];
		recvDisp[i] = recvDisp[i - 1] + recvCount[i - 1];
	}

	int recvSize = recvDisp[size - 1] + recvCount[size - 1];
	int *recvBuff = (int *) malloc(recvSize * sizeof(int));

	MPI_Alltoallv(forSorting, sendCount, sendDisp, MPI_INT,
	              recvBuff, recvCount, recvDisp, MPI_INT, MPI_COMM_WORLD);

	sort(recvBuff, recvBuff + recvSize);

	MPI_Gather(&recvSize, 1, MPI_INT, recvCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
	for (int i = 1; i < size; i++) {
		recvDisp[i] = recvDisp[i - 1] + recvCount[i - 1];
	}

	MPI_Gatherv(recvBuff, recvSize, MPI_INT,
	            forSorting, recvCount, recvDisp, MPI_INT, 0, MPI_COMM_WORLD);

	clock_t sortEnd = clock();

	if (rank == 0) {
		printf("SORTING PROCESS TAKES %.3lf SECOND.\n", (double) (sortEnd - sortStart) / CLOCKS_PER_SEC);
		printf("%d ELEMENTS ARE SORTED.\n", dataSize);
		if (strcmp(argv[2], "nooutput") == 0) {
			out << "dataSize:" << dataSize << " timeConsumed:"
			    << ((double) (sortEnd - sortStart) / CLOCKS_PER_SEC))<<endl;
		} else {
			for (int i = 0; i < dataSize; i++) {
				out << forSorting[i] << ' ';
			}
			printf("THE RESULTS ARE WRITTEN TO FILE %s. \n", argv[2]);
			out << endl;
		}
	}

	free(forSorting);
	free(recvBuff);
	free(recvCount);
	free(recvDisp);
	free(sendDisp);
	free(devItem);
	free(sendCount);

	MPI_Finalize();
	return 0;
}