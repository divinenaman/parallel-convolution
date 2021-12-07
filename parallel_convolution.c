#include<stdio.h>
#include<stdlib.h>
#include "/home/zorin/openMPI/include/mpi.h"

#define N 5
#define M 3

int async_preprocess_send(int* preprocess, int* data_point, int next_data_point, int size, int dest_rank, int tag, MPI_Request *req) {
    for(int i=0; i<size; i++) {
        for(int j=0; j<size; j++) {
            *(preprocess + j + i*size) = *(data_point + j); 
        }
        data_point+=next_data_point;
    }

    MPI_Isend(preprocess, size*size, MPI_INT, dest_rank, tag, MPI_COMM_WORLD, req);
    return 0;
}

int async_recv(int* buff, int size, int src_rank, int src_tag, MPI_Request *req) {
    MPI_Irecv(buff, size*size, MPI_INT, src_rank, src_tag, MPI_COMM_WORLD, req);
    return 0;
}

int convulate(int *data, int *kernel, int size) {
    int val = 0;
    for(int i=0; i<size; i++) {
        val += (*(data + i)) * (*(kernel + i));
    }
    return val;
}

int print(int *data, int size) {
    for(int i=0; i<size; i++) {
        for(int j=0; j<size; j++) {
            printf("%d  ",*(data + j + i*size));
        }
        printf("\n");
    }
    return 0;
}

int main(int argc, char *argv[]) {

    int a[N][N] = {
        {199,208,233,4,5},
        {200,140,10,212,13},
        {150,65,100,101,101},
        {140,50,255,90,10},
        {224,69,89,210,211}
    };

    int kernel[3][3];
    
    int rank, size, i, j, ele_per_procs, ele_remaining, kernel_size, r, req_no, input_offset_start, input_offset_end;
    int *input_data, *output_data;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //get rank of node's process
    MPI_Comm_size(MPI_COMM_WORLD, &size); //comm size
    
    MPI_Request *req;

    if (rank == 0) {
        kernel[0][0] = 0;
        kernel[0][1] = -1;
        kernel[0][2] = 0;
        
        kernel[1][0] = -1;
        kernel[1][1] = 5;
        kernel[1][2] = -1;

        kernel[2][0] = 0;
        kernel[2][1] = -1;
        kernel[2][2] = 0;

        int new_input_size = N+2*(M/2);
        int processed_input_size = new_input_size - (M-1);
        int processed_a[new_input_size][new_input_size];
        input_data = &processed_a[0][0];

        for (i=0; i<new_input_size; i++) {
            for(j=0; j<new_input_size; j++) {
                if(i < M/2 || i > N || j < M/2 || j > N) {
                    processed_a[i][j] = 256;       
                } 
                else {
                    processed_a[i][j] = a[i-1][j-1];
                }
            }
        }        

        print(&processed_a[0][0], new_input_size);

        kernel_size = M;

        MPI_Bcast(&kernel_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&kernel[0][0], kernel_size*kernel_size, MPI_INT, 0, MPI_COMM_WORLD);

        ele_per_procs = (processed_input_size*processed_input_size) / (size - 1);
        ele_remaining = (processed_input_size*processed_input_size) % (size -1);

        MPI_Bcast(&ele_per_procs, 1, MPI_INT, 0, MPI_COMM_WORLD);

        printf("input P: %d\n done bcasts\n size: %d\n", processed_input_size, size);
        printf("procs size: %d\n ele remain %d\n", ele_per_procs, ele_remaining);

        int *preprocess = (int *)malloc(sizeof(int)*kernel_size*kernel_size);
        req = (MPI_Request *)malloc(sizeof(MPI_Request)*ele_per_procs*(size -1));
        
        r = 0;
        req_no = 0;
        int is_limit = 0;
        for(i = 0; i<processed_input_size; i++) {
            for(j = 0; j<processed_input_size; j++) {
                r =  r + (req_no % ele_per_procs == 0);
                async_preprocess_send(preprocess, &processed_a[i][j], new_input_size, kernel_size, r, 0, req+req_no);                         
                req_no++;
                input_offset_start++;
                printf("%d-%d,  sent rq: %d\n", i,j,req_no);
            }
        }

        printf("sent all req\n");
        input_offset_end = ele_remaining;    
    }

    else {
        MPI_Recv(&kernel_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&kernel[0][0], kernel_size*kernel_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&ele_per_procs, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Barrier(MPI_COMM_WORLD);

        req = (MPI_Request *)malloc(sizeof(MPI_Request)*ele_per_procs);
        input_data = (int *)malloc(sizeof(int)*ele_per_procs*kernel_size*kernel_size);
        output_data = (int *)malloc(sizeof(int)*ele_per_procs);
        input_offset_start = 0;
        input_offset_end = 0;
        req_no = 0;
        for(i=0; i<ele_per_procs; i++) {
            async_recv(input_data+i, kernel_size, 0, 0, req+req_no);
            req_no++;
            input_offset_end++;
        }
    }

    MPI_Waitall(req_no, req, MPI_STATUS_IGNORE);

    for(i=input_offset_start; i<input_offset_end; i++) {
        *(output_data + i) = convulate(input_data + i, &kernel[0][0], kernel_size);
    }

    if (rank != 0) {
        req_no = 1;
        MPI_Isend(output_data, ele_per_procs, MPI_INT, 0, 0, MPI_COMM_WORLD, req+req_no);
    } 
    else {
        req_no = 0;
        for(r=1; r<size; r++) {
            async_recv(output_data + (r-1) * ele_per_procs, ele_per_procs, r, 0, req+req_no);
            req_no++;
        }
    }

    MPI_Waitall(req_no, req, MPI_STATUS_IGNORE);

    if (rank == 0) {
        printf("heeeheheh");
        print(output_data, N);
    }

    MPI_Finalize();
    return 0;
}