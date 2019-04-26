/*
 * =====================================================================================
 *       Filename:  sort.c
 *    Description:  study shell sort and binary sort algorithm 
 *        Version:  0.1
 *        Created:  10/15/2015 07:14:33 PM
 *       Revision:  2015.10.21
 *       Compiler:  gcc
 * =====================================================================================
 */
#include<stdio.h>
#include<string.h>

#define MAXLINE	5000

void shellsort(int v[], int n)
{
	int gap, i, j, temp;
	for(gap = n/2; gap > 0; gap /= 2)
	{
		for(i = gap; i < n; i++)
		{
			for(j=i-gap; j>=0 && v[j] > v[j+gap]; j -= gap)
			{
				temp = v[j];
				v[j] = v[j+gap];
				v[j+gap] = temp;
			}
		}
	}
}

int binsearch(int x, int v[], int n)
{
	int low, high, mid;
	
	low = 0;
	high = n - 1;
	while(low <= high)
	{
		mid = (low + high)/2;
		if(x > v[mid])
		{
			low = mid + 1;
		}
		else if(x < v[mid])
		{
			high = mid - 1;
		}
		else
		{
			return mid;
		}

	}
	return -1; // no match
}

void swap(int v[], int i, int j)
{
	int temp;
	temp = v[i];
	v[i] = v[j];
	v[j] = temp;
}


void quicksort(int v[], int left, int right)
{
	int i, last;

	if(left >= right) // do nothing if array contains fewer than two elements
		return;
	
	swap(v, left, (left+right)/2);
	
	last = left;
	for(i = left+1; i<=right; i++)
		if(v[i] > v[left])
			swap(v, ++last, i);

	swap(v,left,last);

	quicksort(v, left, last-1);
	quicksort(v, last+1, right);
}
