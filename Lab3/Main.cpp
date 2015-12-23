#include <stdio.h>
#include <mpi.h>
#include <math.h> 
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

int** initMatrix(int n)
{
	int **a = new int*[n];
	for (int i = 0; i < n; i++)
		a[i] = new int[n];

	return a;
}

int** fillInMatrix(int** a, int n)
{
	srand(time(NULL));
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			a[i][j] = rand() % 100;
		}
	}
	return a;
}

int* fillVector(int * vector, int n)
{
	for (int i = 0; i < n; i++)
		vector[i] = 1;
	return vector;
}

int main(int argc, char* argv[])
{
	int rank, size;
	double t1, t2;
	int matrixSize;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (rank == 0)
	{
		cout << "Enter matrix size\n";
		cin >> matrixSize;
	}
	MPI_Bcast(&matrixSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

	int **A = initMatrix(matrixSize);
	int * vector = new int[matrixSize];
	int * resultVector = new int[matrixSize];

	if (rank == 0)
	{
		A = fillInMatrix(A, matrixSize);
		vector = fillVector(vector, matrixSize);
	}
	MPI_Bcast(vector, matrixSize, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	int residue = matrixSize % size;
	int rowAmount = matrixSize / size;
	if (rank == 0)
	{
		t1 = MPI_Wtime();
		//cout << "\nrowAmount: " << rowAmount << "\n";
		//cout << "\nresidue: " << residue << "\n";
	}

	if (rank == 0)
	{
		//0 ����� ��������� ������ ����� �������
		for (int i = 0; i < rowAmount; i++)
		{
			resultVector[i] = 0;
			for (int j = 0; j < matrixSize; j++)
			{
				resultVector[i] += A[i][j] * vector[j];
			}
		}
		int r = 0;
		for (int m = 1; m < size; m++)
		{
			int amount = rowAmount;
			if (residue > 0 && size-m <= residue)
			{
				amount++;		
			}
			//�������� ������ ������� �� ����� �������
			int *row = new int[matrixSize*amount];
			for (int l = 0; l < amount; l++)
			{
				for (int j = 0; j < matrixSize; j++)
				{
					row[j + l*matrixSize] = A[m*rowAmount + l + r][j];
				}
			}
			MPI_Send(row, matrixSize*amount, MPI_INT, m, m, MPI_COMM_WORLD);			

			//��������� �� ������ ������� ����������� �������� �������
			MPI_Status status;
			int * resultElems = new int[amount];
			MPI_Recv(resultElems, amount, MPI_INT, m, m, MPI_COMM_WORLD, &status);

			for (int j = 0; j < amount; j++)
			{
				resultVector[m*rowAmount + j + r] = resultElems[j];
			}
			if (amount > rowAmount)
				r++;
		}
	}
	else
	{
		MPI_Status status;
		int *row;
		if (residue > 0 && size - rank <= residue)
		{			
			rowAmount++;
		}
		row = new int[matrixSize*rowAmount];
		MPI_Recv(row, matrixSize*rowAmount, MPI_INT, 0, rank, MPI_COMM_WORLD, &status);

		int* rankResultVector = new int[rowAmount];
		for (int i = 0; i < rowAmount; i++)
		{
			int c = 0;
			for (int j = 0; j < matrixSize; j++)
			{
				c += row[j + matrixSize*i] * vector[j];
			}
			rankResultVector[i] = c;
		}
		MPI_Send(rankResultVector, rowAmount, MPI_INT, 0, rank, MPI_COMM_WORLD);
	}

	if (residue > 0)

		MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0)
	{
		t2 = MPI_Wtime();
		if (matrixSize <= 100)
		{
			cout << "\nResult vector:\n";
			for (int i = 0; i < matrixSize; i++)
			{
				cout << resultVector[i] << " ";
			}
		}
		cout << "\nTime: " << (t2 - t1);
	}

	MPI_Finalize();
}