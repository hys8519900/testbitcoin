#include "pow.h"
#include "random.h"

int main()
{
	uint256 hash = GetRandHash();
	CheckProofOfWork(hash, 0);

}
