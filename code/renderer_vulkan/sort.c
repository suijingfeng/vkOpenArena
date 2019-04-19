/*
 * =====================================================================================
 *       Filename:  sort.c
 *    Description:  study shell sort and binary sort algorithm 
 *        Version:  0.1
 *        Created:  10/15/2015 07:14:33 PM
 *       Revision:  2015.10.21
 *       Compiler:  gcc
 *         Author:  Sui Jingfeng (moon), Jingfengsui@gmail.com
 *        Company:  CASIA(2014 ~ 2017)
 * =====================================================================================
 */
#include<stdio.h>
#include<string.h>
//#define DEBUG	1

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
#ifdef DEBUG
		printf("%d_",mid);
#endif		
	}
	return -1; // no match
}

void quicksort(int v[], int left, int right)
{
	int i, last;
	void swap(int v[], int i, int j);

	if(left >= right) // do nothing if array contains fewer than two elements
		return;
	
	swap(v, left, (left+right)/2);
	
	last = left;
	for(i = left+1; i<=right; i++)
		if(v[i] < v[left])
			swap(v, ++last, i);

	swap(v,left,last);

	quicksort(v, left, last-1);
	quicksort(v, last+1, right);
}


void swap(int v[], int i, int j)
{
	int temp;

	temp = v[i];
	v[i] = v[j];
	v[j] = temp;
}


int binsearch2(int x, int v[],int n)
{
	int low, high, mid;
	for(low = 0,high=n-1,mid = (low + high)/2; (low < high)&&(v[mid] != x) ; mid = (low+high)/2)
	{
		if(x < v[mid])
			high = mid - 1;
		else
			low = mid + 1;
	}
	if(v[mid]==x)
		return mid;
	else
		return 0;

#ifdef DEBUG
		printf("%d ",mid);
#endif		
}

int main()
{
	int i = 0;
	int n[15] = {1,8,9,3,0,7,6,2,4,5,11,13,15,17,19};
//	shellsort(n, 15);
	quicksort(n,0,14);
	int j=0;
	j = sizeof(n);
printf("%d\n", sizeof(n));
	for(i=0;i<15;i++)
		printf("%d\t", n[i]);
	printf("\n");

	if((j = binsearch2(4, n, 15)) == -1)
		printf("can't find 4 in array n[8]\n");
	else
		printf("find at n[%d]\n",j);
	if((i = binsearch2(6, n, 15)) != -1)
		printf("find at a[%d]\n",i);
		
	return 0;
}
