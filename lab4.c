#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"


int read_tuple_from_blk(unsigned char *buf, int start, int *result)
{
    int k;
    char str[5];
    for(k=0; k<4; k++)
    {
        str[k] = *(buf + start + k);
    }
    result[0] = atoi(str);

    for(k=0; k<4; k++)
    {
        str[k] = *(buf + start + k + 4);
    }
    result[1] = atoi(str);
    return 0;
}

int find_key_by_num(int num)
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

        int result[2];
        
        for(int j = 0; j < 7; j++)
        {
            read_tuple_from_blk(blk, j*TUPLE_SIZE, result);
            if(result[0] == num)
            {
                printf("(%d, %d)\n", result[0], result[1]);
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

int inner_sort(Buffer *buf, unsigned char **blk_pointers)
{
    
}

int merge_sort(Buffer *buf)
{

}
int TPMMS(int blk_start, int blk_end)
{
    Buffer buf;
    unsigned char *blk;
    unsigned char *blk_pointers[8];
    int i;

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    int epoch_R = (R_END - R_BIGIN + 1) / buf.numAllBlk;
    for(i=0; i<epoch_R; i++)
    {
        for(int j=1; j<=8; j++)
        {
            int blk_index = i*buf.numAllBlk + j;
            /* Read the block from the hard disk */
            if ((blk = readBlockFromDisk(blk_index, &buf)) == NULL)
            {
                perror("Reading Block Failed!\n");
                return -1;
            }
        }
        inner_sort(&buf, blk_pointers);


    }

    int epoch_S = (S_END - S_BEGIN + 1) / buf.numAllBlk;
    for(int i=1; i<S_END; i++)
    {
        /* Read the block from the hard disk */
        if ((blk = readBlockFromDisk(i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }

        
    }

}
int main(int argc, char **argv)
{
    // find_key_by_num(50);
    TPMMS(R_BIGIN, R_END);

}
