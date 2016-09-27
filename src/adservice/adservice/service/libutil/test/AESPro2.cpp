#include <stdio.h>
#include "AdAes.h"

int main(int argc, char *argv[])
{	
    char mingwen[] = "125.00";
    char miwen_hex[1024];
    char result[1024];
    unsigned char key[] = "hsylgwk-20120101";
    AES aes(key);
    printf("key:%s\n",key);
    printf("src:%s\n",mingwen);
    aes.Bm53Cipher(mingwen,miwen_hex);
    printf("%s\n",miwen_hex);
    aes.Bm53InvCipher(miwen_hex, result);	
    printf("%s\n",result);
    getchar();
    return 0;
}
