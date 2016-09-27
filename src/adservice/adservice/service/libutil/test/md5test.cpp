#include <iostream>
#include <iostream>
#include "AdMd5.h"


int main(int argc, char ** argv)
{
	std::string sStr = "ABCDEFG";
	unsigned char key[AD_MD5 * 2];
	if(argc > 1)
		sStr = argv[1];
	AdMd5 md5;
	md5.AdMd5Run((char *)sStr.c_str(), sStr.size(),  key);
	printf("Input: %s\n", sStr.c_str());
	printf("Output: %s\n", key);

	printf("Output Str: ");
    for(int j = 0; j < 16; j++ )  
    {  
       printf("%02x", key[j]);  
    }  
    printf("\n ");  
	return 0;
}


