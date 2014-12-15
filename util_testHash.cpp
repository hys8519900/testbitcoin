#include "hash.h"
#include <iostream>
#include <string>

using namespace std;

int main()
{
	string str = "abcde";
	cout << Hash(str.begin(), str.end()).ToString() << endl;
}
