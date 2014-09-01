#include "primes.h"

static int is_prime(unsigned long x)
{
	unsigned long o = 4, i = 5;
	do {
		unsigned long q = x / i;
		if (q < i)
			return 1;
		if (x == q * i)
			return 0;
		o ^= 6;
		i += o;
	} while (1);
	return 1;
}

unsigned long next_prime(unsigned long n)
{
	unsigned long k, i, o;

	switch (n) {
	case 0:
	case 1:
	case 2:
		return 2;
	case 3:
		return 3;
	case 4:
	case 5:
		return 5;
	}

	k = n / 6;
	i = n - 6 * k;
	o = i < 2 ? 1 : 5;
	n = 6 * k + o;
	for (i = (3 + o) / 2; !is_prime(n); n += i)
		i ^= 6;
	return n;
}
