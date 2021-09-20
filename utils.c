int
map_n(int n, int min1, int max1, int min2, int max2)
{
	return ((double)(n - min1) / (max1 - min1)) * (max2 - min2) + min2;
}
