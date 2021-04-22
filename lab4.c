#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"


int main(int argc, char **argv)
{
    Buffer buf; /* A buffer */
    unsigned char *blk; /* A pointer to a block */
    int i = 0;

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    
    for(i = S_BEGIN; i <= S_END; i++)
    {
        /* Read the block from the hard disk */
        if ((blk = readBlockFromDisk(i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }

        int X = -1;
        int Y = -1;
        char str[5];
        
        for(int j = 0; j < 7; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                str[k] = *(blk + j*8 + k);
            }
            X = atoi(str);

            for (int k = 0; k < 4; k++)
            {
                str[k] = *(blk + j*8 + k + 4);
            }
            Y = atoi(str);
            if(X == 50)
            {
                printf("(%d, %d)\n", X, Y);
            }
        }

        /* Write the block to the hard disk */
        if (writeBlockToDisk(blk, i, &buf) != 0)
        {
            perror("Writing Block Failed!\n");
            return -1;
        }
    }

    printf("IO's is %d\n", buf.numIO); /* Check the number of IO's */

}
