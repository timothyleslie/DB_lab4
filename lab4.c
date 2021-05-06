#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"



int write_tuple_to_blk(unsigned char*blk, int key1, int key2)
{
    unsigned char str[9];
    memset(str, 0, 9);
    sprintf(str, "%d", key1);
    sprintf(str+4, "%d", key2);
    memcpy(blk, str, 8);
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
    int seg_num = (input_blk_end-input_blk_start) / buf->numAllBlk + 1;
    unsigned char** buf_blk_ptrs = (unsigned char**)malloc(seg_num * sizeof(unsigned char*));
    int i, k;
    int seg_cnt = 0;
    int data[2];
    int min;
    unsigned char* output_blk;
    int output_blk_offset;
    int next_blk = output_blk_start;

    for(i=0; i<seg_num; i++)
    {
        /* Read the block from the hard disk */
        if ((buf_blk_ptrs[i] = readBlockFromDisk(input_blk_start+(buf->numAllBlk)*i, buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }
    }
    output_blk = getNewBlockInBuffer(buf);
    output_blk_offset = 0;

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
                /* free the block */
                freeBlockInBuffer(buf_blk_ptrs[i] - buf->blkSize + TUPLE_SIZE, buf);
                seg_cnt += 1;
                buf_blk_ptrs[i] = NULL;
            }
            else if(data[1] == BLK_END) //读到块尾,重新读入一块到内存
            {
                /* free the block */
                freeBlockInBuffer(buf_blk_ptrs[i] - buf->blkSize + TUPLE_SIZE, buf);

                /* Read the block from the hard disk */
                if ((buf_blk_ptrs[i] = readBlockFromDisk(data[0], buf)) == NULL)
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
        if(buf_blk_ptrs[k] != NULL)
        {
            memcpy(output_blk+output_blk_offset, buf_blk_ptrs[k], TUPLE_SIZE);
            output_blk_offset += TUPLE_SIZE;
            buf_blk_ptrs[k] += TUPLE_SIZE;
        }

        if(output_blk_offset >= buf->blkSize - TUPLE_SIZE)
        {
            next_blk += 1;
            write_tuple_to_blk(output_blk+output_blk_offset, next_blk, BLK_END);
            /* Write the block to the hard disk */
            if (writeBlockToDisk(output_blk, next_blk-1, buf) != 0)
            {
                perror("Writing Block Failed!\n");
                return -1;
            }

            output_blk = getNewBlockInBuffer(buf);
            clearBlockInBuffer(output_blk, buf);
            output_blk_offset = 0;
        } 
         
    }
    free(buf_blk_ptrs);
}

int TPMMS(int blk_start, int blk_end, int inner_blk_start, int inner_blk_end, int output_blk_start)
{
    Buffer buf;
    unsigned char *blk;
    int i;
    int next_blk = inner_blk_start;

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
            // printf("%d\n", end_flag);
            write_tuple_to_blk(end_line, next_blk, end_flag);
            /* Write the block to the hard disk */
            if (writeBlockToDisk(blk, inner_blk_start+i*buf.numAllBlk+j, &buf) != 0)
            {
                perror("Writing Block Failed!\n");
                return -1;
            }
        }
    }
    merge_sort(&buf, inner_blk_start, inner_blk_end, output_blk_start);
}

int create_index(int input_blk_start, int input_blk_end, int output_blk_start)
{
    Buffer buf;
    unsigned char *blk;
    int i;
    int next_blk = output_blk_start;
    unsigned char* output_blk;
    int output_blk_offset;
    int data[2];
    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    output_blk = getNewBlockInBuffer(&buf);
    output_blk_offset = 0;
    for(i=input_blk_start; i<=input_blk_end; i++)
    {
        if ((blk = readBlockFromDisk(i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }

        read_tuple_from_blk(blk, 0, data);
        write_tuple_to_blk(output_blk+output_blk_offset, data[0], i);
        output_blk_offset += TUPLE_SIZE;
        freeBlockInBuffer(blk, &buf);

        if(output_blk_offset >= buf.blkSize-TUPLE_SIZE)
        {
            next_blk += 1;
            printf("%d\n", next_blk);
            write_tuple_to_blk(output_blk+output_blk_offset, next_blk, BLK_END);
            if (writeBlockToDisk(output_blk, next_blk-1, &buf) != 0)
            {
                perror("Writing Block Failed!\n");
                return -1;
            }
            freeBlockInBuffer(output_blk, &buf);
            output_blk = getNewBlockInBuffer(&buf);
            output_blk_offset = 0;
        }
    }

}


int search_by_index(int target_key, int index_blk_start, int index_blk_end, int output_blk_start)
{
    Buffer buf;
    unsigned char *blk;
    int i, j;
    int next_blk = output_blk_start;
    unsigned char* output_blk;
    int output_blk_offset;
    int data[2];
    int search_blk_start;
    int search_blk_end;


    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    // 找到要带搜索的磁盘块
    i = index_blk_start;
    int search_flag = 0;
    while(i<=index_blk_end)
    {
        for(j=0; j<buf.numAllBlk && ((i+j)<=index_blk_end); j++)
        {
            if ((blk = readBlockFromDisk(i+j, &buf)) == NULL)
            {
                perror("Reading Block Failed!\n");
                return -1;
            }
        }

        int valid_blk_num = j;
        search_blk_start = i;
        search_blk_end = j;
        
        printf("%d, %d\n", search_blk_start, search_blk_end);
        for(j=0; j<=valid_blk_num*(buf.blkSize+1); j+=TUPLE_SIZE)
        {
            find_data_in_buf(&buf, &j);
            read_tuple_from_blk(buf.data, j, data);
            printf("%d, %d\n", data[0], data[1]);
            if(data[0] > target_key)
            {        
                search_blk_end = data[1] - 1;
                search_flag = 1;
                break;
            }

            else if(data[0] < target_key)
            {
                search_blk_start = data[1];
            }

            printf("%d, %d\n", search_blk_start, search_blk_end);
        }
        if(search_flag == 1)    break;
        i += buf.numAllBlk;
    }

    printf("%d, %d\n", search_blk_start, search_blk_end);
    // 在带搜索的磁盘块中寻找目标值
    output_blk = getNewBlockInBuffer(&buf);
    output_blk_offset = 0;
    for(i=search_blk_start; i<=search_blk_end; i++)
    {
        if ((blk = readBlockFromDisk(i, &buf)) == NULL)
        {
            perror("Reading Block Failed!\n");
            return -1;
        }

        for(j=0; j<buf.blkSize-TUPLE_SIZE; j++)
        {
            read_tuple_from_blk(blk, j, data);
            if(data[0] == target_key)
            {
                printf("(%d, %d)\n", data[0], data[1]);
                write_tuple_to_blk(output_blk+output_blk_offset, data[0], data[1]);
                output_blk_offset += TUPLE_SIZE;

                if(output_blk_offset >= buf.blkSize - TUPLE_SIZE)
                {
                    next_blk += 1;
                    write_tuple_to_blk(output_blk+output_blk_offset, next_blk, BLK_END);
                    if (writeBlockToDisk(output_blk, next_blk-1, &buf) != 0)
                    {
                        perror("Writing Block Failed!\n");
                        return -1;
                    }

                    output_blk = getNewBlockInBuffer(&buf);
                    clearBlockInBuffer(output_blk, &buf);
                    output_blk_offset = 0;
                }
            }
        }    
    }

    //如果最后还有数据但是不满一个磁盘块
    if(output_blk_offset != 0)
    {
        next_blk += 1;
        write_tuple_to_blk(output_blk+output_blk_offset, next_blk, BLK_END);
        if (writeBlockToDisk(output_blk, next_blk-1, &buf) != 0)
        {
            perror("Writing Block Failed!\n");
            return -1;
        } 
    }

    /* Check the number of IO's */
    printf("IO's is %d\n", buf.numIO); 
}


int sort_merge_join(int R_start, int R_end, int S_start, int S_end, int output_blk_start)
{
    Buffer buf;
    unsigned char *blk;
    unsigned char *s;
    unsigned char *r;
    int s_offset, r_offset;
    int s_blk, r_blk;
    int i, j;
    int next_blk = output_blk_start;
    unsigned char* output_blk;
    int output_blk_offset;
    int r_data[2], s_data[2];
    int s_tmp_start;
    int merge_end_flag = 0;


    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    /* initialize for loop */
    r_blk = R_start;
    s_blk = S_start;
    if ((r = readBlockFromDisk(r_blk, &buf)) == NULL)
    {
        perror("Reading Block Failed!\n");
        return -1;
    }
    r_offset = 0;

    if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
    {
        perror("Reading Block Failed!\n");
        return -1;
    }
    s_offset = 0;

    output_blk = getNewBlockInBuffer(&buf);
    clearBlockInBuffer(output_blk, &buf);
    output_blk_offset = 0;
    next_blk = output_blk_start;

    while(!merge_end_flag)
    {
        read_tuple_from_blk(r+r_offset, 0, r_data);
        read_tuple_from_blk(s+s_offset, 0, s_data);
        if(r_data[1] == BLK_END)
        {
            if(r_blk == R_end)
            {
                merge_end_flag = 1;
                break;
            }
            else 
            {
                freeBlockInBuffer(r, &buf);
                r_blk += 1;
                if ((r = readBlockFromDisk(r_blk, &buf)) == NULL)
                {
                    perror("Reading Block Failed!\n");
                    return -1;
                }
                r_offset = 0;
                read_tuple_from_blk(r+r_offset, 0, r_data);
            }
        }

        if(s_data[1] == BLK_END)
        {
            if(s_blk == S_end)
            {
                merge_end_flag = 1;
                break;
            }
            else 
            {
                freeBlockInBuffer(s, &buf);
                s_blk += 1;
                if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
                {
                    perror("Reading Block Failed!\n");
                    return -1;
                }
                s_offset = 0;
                read_tuple_from_blk(s+s_offset, 0, s_data);
            }
        }
        printf("%d, %d\n", r_blk, s_blk);
        if(r_data[0] < s_data[0])
        {
            r_offset += TUPLE_SIZE;
        }
        else if(r_data[0] > s_data[0])
        {
            s_offset += TUPLE_SIZE;
        }

        else
        {
            s_tmp_start = s_blk;

            while(r_data[0] == s_data[0])
            {
                write_tuple_to_blk(output_blk+output_blk_offset, r_data[0], r_data[1]);
                output_blk_offset += TUPLE_SIZE;
                if(output_blk_offset >= buf.blkSize - TUPLE_SIZE)
                {
                    next_blk += 1;
                    write_tuple_to_blk(output_blk+output_blk_offset, next_blk, BLK_END);
                    if (writeBlockToDisk(output_blk, next_blk-1, &buf) != 0)
                    {
                        perror("Writing Block Failed!\n");
                        return -1;
                    }

                    output_blk = getNewBlockInBuffer(&buf);
                    clearBlockInBuffer(output_blk, &buf);
                    output_blk_offset = 0;
                }

                write_tuple_to_blk(output_blk+output_blk_offset, s_data[0], s_data[1]);
                output_blk_offset += TUPLE_SIZE;
                if(output_blk_offset >= buf.blkSize - TUPLE_SIZE)
                {
                    next_blk += 1;
                    write_tuple_to_blk(output_blk+output_blk_offset, next_blk, BLK_END);
                    if (writeBlockToDisk(output_blk, next_blk-1, &buf) != 0)
                    {
                        perror("Writing Block Failed!\n");
                        return -1;
                    }

                    output_blk = getNewBlockInBuffer(&buf);
                    clearBlockInBuffer(output_blk, &buf);
                    output_blk_offset = 0;
                }

                s_offset += TUPLE_SIZE;
                read_tuple_from_blk(s+s_offset, 0, s_data);
                // printf("%d\n", r_data[0]);
                // printf("%d, %d\n", s_data[0], s_data[1]);
                if(s_data[1] == BLK_END)
                {
                    s_blk += 1;
                    if(s_blk > S_end)
                    {
                        break;
                    }
                    else
                    {
                        freeBlockInBuffer(s, &buf);
                        if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
                        {
                            perror("Reading Block Failed!\n");
                            return -1;
                        }
                    } 
                }
            }

            // printf("sblk: %d\n", s_blk);
            r_offset += TUPLE_SIZE;
            s_blk = s_tmp_start;
            freeBlockInBuffer(s, &buf);
            if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
            {
                perror("Reading Block Failed!\n");
                return -1;
            }
            s_offset = 0;
        }

    }

    /* Check the number of IO's */
    printf("IO's is %d\n", buf.numIO); 
}


int intersect(int R_start, int R_end, int S_start, int S_end, int output_blk_start)
{
    Buffer buf;
    unsigned char *blk;
    unsigned char *s;
    unsigned char *r;
    int s_offset, r_offset;
    int s_blk, r_blk;
    int i, j;
    int next_blk = output_blk_start;
    unsigned char* output_blk;
    int output_blk_offset;
    int r_data[2], s_data[2];
    int s_tmp_start;
    int merge_end_flag = 0;


    /* Initialize the buffer */
    if (!initBuffer(520, 64, &buf))
    {
        perror("Buffer Initialization Failed!\n");
        return -1;
    }

    /* initialize for loop */
    r_blk = R_start;
    s_blk = S_start;
    if ((r = readBlockFromDisk(r_blk, &buf)) == NULL)
    {
        perror("Reading Block Failed!\n");
        return -1;
    }
    r_offset = 0;

    if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
    {
        perror("Reading Block Failed!\n");
        return -1;
    }
    s_offset = 0;

    output_blk = getNewBlockInBuffer(&buf);
    clearBlockInBuffer(output_blk, &buf);
    output_blk_offset = 0;
    next_blk = output_blk_start;

    while(!merge_end_flag)
    {
        read_tuple_from_blk(r+r_offset, 0, r_data);
        read_tuple_from_blk(s+s_offset, 0, s_data);
        if(r_data[1] == BLK_END)
        {
            if(r_blk == R_end)
            {
                merge_end_flag = 1;
                break;
            }
            else 
            {
                freeBlockInBuffer(r, &buf);
                r_blk += 1;
                if ((r = readBlockFromDisk(r_blk, &buf)) == NULL)
                {
                    perror("Reading Block Failed!\n");
                    return -1;
                }
                r_offset = 0;
                read_tuple_from_blk(r+r_offset, 0, r_data);
            }
        }

        if(s_data[1] == BLK_END)
        {
            if(s_blk == S_end)
            {
                merge_end_flag = 1;
                break;
            }
            else 
            {
                freeBlockInBuffer(s, &buf);
                s_blk += 1;
                if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
                {
                    perror("Reading Block Failed!\n");
                    return -1;
                }
                s_offset = 0;
                read_tuple_from_blk(s+s_offset, 0, s_data);
            }
        }
        // printf("%d, %d\n", r_blk, s_blk);
        if(r_data[0] < s_data[0])
        {
            r_offset += TUPLE_SIZE;
        }
        else if(r_data[0] > s_data[0])
        {
            s_offset += TUPLE_SIZE;
        }

        else
        {
            s_tmp_start = s_blk;

            while(r_data[0] == s_data[0])
            {
                if(r_data[1] == s_data[1])
                {
                    printf("%d, %d\n", r_data[0], r_data[1]);
                    write_tuple_to_blk(output_blk+output_blk_offset, r_data[0], r_data[1]);
                    output_blk_offset += TUPLE_SIZE;
                    if(output_blk_offset >= buf.blkSize - TUPLE_SIZE)
                    {
                        next_blk += 1;
                        write_tuple_to_blk(output_blk+output_blk_offset, next_blk, BLK_END);
                        if (writeBlockToDisk(output_blk, next_blk-1, &buf) != 0)
                        {
                            perror("Writing Block Failed!\n");
                            return -1;
                        }

                        output_blk = getNewBlockInBuffer(&buf);
                        clearBlockInBuffer(output_blk, &buf);
                        output_blk_offset = 0;
                    }

                }
                
                s_offset += TUPLE_SIZE;
                read_tuple_from_blk(s+s_offset, 0, s_data);
                // printf("%d\n", r_data[0]);
                // printf("%d, %d\n", s_data[0], s_data[1]);
                if(s_data[1] == BLK_END)
                {
                    s_blk += 1;
                    if(s_blk > S_end)
                    {
                        break;
                    }
                    else
                    {
                        freeBlockInBuffer(s, &buf);
                        if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
                        {
                            perror("Reading Block Failed!\n");
                            return -1;
                        }
                    } 
                }
            }

            // printf("sblk: %d\n", s_blk);
            r_offset += TUPLE_SIZE;
            s_blk = s_tmp_start;
            freeBlockInBuffer(s, &buf);
            if ((s = readBlockFromDisk(s_blk, &buf)) == NULL)
            {
                perror("Reading Block Failed!\n");
                return -1;
            }
            s_offset = 0;
        }

    }

    /* Check the number of IO's */
    printf("IO's is %d\n", buf.numIO); 
}

int main(int argc, char **argv)
{
    // find_key_by_num(50);
    // TPMMS(R_BEGIN, R_END, 201, 216, 301);
    // TPMMS(S_BEGIN, S_END, 217, 248, 317);
    // create_index(317, 348, 417);
    // search_by_index(50, 417, 420, 517);
    // sort_merge_join(301, 316, 317, 348, 601);
    intersect(301, 316, 317, 348, 701);

}
