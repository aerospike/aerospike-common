
#include <rbuffer.h>

int main()
{
	if (0 != cf_rbuffer_test1(100000))
		fprintf(stderr, "Test 1: FAIL\n"); 
	else
		fprintf(stderr, "Test 1: PASS\n"); 
	fprintf(stderr, "\n"); 
	
	if (0 != cf_rbuffer_test2(100000))
		fprintf(stderr, "Test 2: FAIL\n"); 
	else
		fprintf(stderr, "Test 2: PASS\n"); 

	fprintf(stderr, "\n"); 
	if (0 != cf_rbuffer_test3(100000))
		fprintf(stderr, "Test 3: FAIL\n"); 
	else
		fprintf(stderr, "Test 3: PASS\n"); 

	fprintf(stderr, "\n"); 
	if (0 != cf_rbuffer_test4(100000))
		fprintf(stderr, "Test 4: FAIL\n"); 
	else
		fprintf(stderr, "Test 4: PASS\n"); 
	
	fprintf(stderr, "\n"); 
	if (0 != cf_rbuffer_test5(100000))
		fprintf(stderr, "Test 5: FAIL\n"); 
	else
		fprintf(stderr, "Test 5: PASS\n"); 

	fprintf(stderr, "\n"); 

	if (0 != cf_rbuffer_test6(100000))
		fprintf(stderr, "Test 6: FAIL\n"); 
	else
		fprintf(stderr, "Test 6: PASS\n"); 
	
	return 0;
}

