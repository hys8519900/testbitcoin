#include "bloom.h"
#include "base58.h"
#include "key.h"
#include "serialize.h"
#include "uint256.h"
#include "util.h"

#include <vector>
#include <iostream>

using namespace std;


int main()
{
  CBloomFilter filter(3, 0.01, 0, BLOOM_UPDATE_ALL);
  filter.insert(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8"));
  if(filter.contains(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8"))){
    cout << "filter contains 99108ad8ed9bb6274d3980bab5a85c048f0950c8" << endl;
  }
  if(!filter.contains(ParseHex("19108ad8ed9bb6274d3980bab5a85c048f0950c8"))){
    cout << "filter not contain after change first byte" << endl;
  }
}
