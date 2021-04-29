#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"



int fill_endline(unsigned char*endline, int next_blk, int end_flag)
{
    unsigned char str[9];
    sprintf(str, "%d", next_blk);
    sprintf(str+4, "%d", end_flag);
    memcpy(endline, str, 8);
    return 0;
}

void buf_swap(unsigned char*a, unsigned char*b)
{
    unsigned char tmp[9];
    memcpy(tmp, a, 8);
    memcpy(a, b, 8);
    memcpy(b, tmp, 8);
}

void find_data_in_buf(Buffer *buf, int *index)
{
    int i = *index;
    if(i % (buf->blkSize+1) == 0)
    {
        *index += 1;
    }
    else if(i % (buf->blkSize+1) == buf->blkSize+1-TUPLE_SIZE)
    {
        *index += TUPLE_SIZE + 1;
    }
}


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

int inner_sort(Buffer *buf)
{
    unsigned char *buf_start = buf->data;
    int i, j, k;
    int min;
    int data[2];
    for(i=0; i<buf->bufSize-TUPLE_SIZE; i=i+TUPLE_SIZE)
    {
        find_data_in_buf(buf, &i);
        read_tuple_from_blk(buf->data, i, data);
        min = data[0];
        k = i;
        for(j=i+TUPLE_SIZE; j<buf->bufSize-TUPLE_SIZE; j=j+TUPLE_SIZE)
        {
            find_data_in_buf(buf, &j);
            
            read_tuple_from_blk(buf->data, j, data);
            if(data[0] < min)
            {
                // printf("%d\n", j);
                k = j;
                min = data[0];
            }
        }
        if(i!=k)
        {
            // printf("%d, %d, %d\n", i, j, k);
            buf_swap(buf_start+i, buf_start+k);
        }     
    }
}

int merge_sort(Buffer *buf, int input_blk_start, int input_blk_end, int output_blk_start)
{
    int seg_num = (input_blk_end-input_blk_start) / buf->numAllBlk;
    unsigned char** buf_blk_ptrs = (unsigned char**)malloc(seg_num * sizeof(unsigned char*));
    int i, k;
    int seg_cnt = 0;
    int data[2];
    int min;
    unsigned char* output_blk;
    int output_blk_offset = 0;
    int next_blk = output_blk_start;

    for(i=0; i<seg_num; i++)
    {
        /* Read the block from the hard disk */
        if ((buf_blk_ptrs[i] = readBlockFromDisk(input_blk_start+(buf->numAllBlk)*i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }
    }
    output_blk = getNewBlockInBuffer(buf);

    while(seg_cnt < seg_num)
    {
        min = KEY_MAX;

        for(i=0; i<seg_num; i++)
        {
            if(buf_blk_ptrs[i] == NULL)
                continue;

            read_tuple_from_blk(buf_blk_ptrs[i], 0, data);
            if(data[1] == SEG_END) //读到段尾,将段指针置为空
            {
                seg_cnt += 1;
                buf_blk_ptrs[i] = NULL;
            }
            else if(data[1] == BLK_END) //读到块尾,重新读入一块到内存
            {
                /* Read the block from the hard disk */
                if ((buf_blk_ptrs[i] = readBlockFromDisk(data[0], &buf)) == NULL)
                {
                    perror("Reading Block Failed!\n");
                    return -1;
                }
            }
            else if(data[0] < min)
            {
                min = data[0];
                k = i;
            }
        }

        memcpy(output_blk+output_blk_offset, buf_blk_ptrs[k], TUPLE_SIZE);
        output_blk_offset += TUPLE_SIZE;
        if(output_blk_offset >= buf->blkSize - TUPLE_SIZE)
        {
            next_blk += 1;
            fill_endline(output_blk+output_blk_offset, next_blk, BLK_END);
            
        }



        
    }

    




    free(buf_blk_ptrs);
}

int TPMMS(int blk_start, int blk_end, int output_blk_start)
{
    Buffer buf;
    unsigned char *blk;
    int i;
    int next_blk = output_blk_start;

    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }
    
    int epoch = (blk_end - blk_start + 1) / buf.numAllBlk;
    for(i=0; i<epoch; i++)
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

        inner_sort(&buf);
        
        for(int j=0; j<8; j++)
        {
            next_blk += 1;
            blk = buf.data + 1 + j*(buf.blkSize+1);
            unsigned char *end_line = blk + buf.blkSize - TUPLE_SIZE;
            int end_flag = (j == 7) ? SEG_END : BLK_END;
            fill_endline(end_line, next_blk, end_flag);

            /* Write the block to the hard disk */
            if (writeBlockToDisk(blk, 201+i*buf.numAllBlk+j, &buf) != 0)
            {
                perror("Writing Block Failed!\n");
                return -1;
            }
        }
    }
}


int main(int argc, char **argv)
{
    // find_key_by_num(50);
    TPMMS(R_BIGIN, R_END, 201);

}
