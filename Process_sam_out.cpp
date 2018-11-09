/*
  Authors:
  Haoyu Cheng
	  chhy@mail.ustc.edu.cn
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Auxiliary.h"
#include "Process_sam_out.h"
#include <pthread.h>

pthread_mutex_t o_readinputMutex;
pthread_mutex_t o_queueMutex;
pthread_mutex_t o_terminateMutex;
pthread_cond_t o_flushCond;
pthread_cond_t o_readinputflushCond;
pthread_cond_t o_stallCond;
pthread_cond_t o_readinputstallCond;
pthread_mutex_t o_doneMutex;



FILE *_out_fp;

int sub_block_init_length = 10000;


Output_buffer buffer_out;
Output_buffer_sub_block tmp_buffer_sub_block;

Output_methy_buffer buffer_methy_out;
Methylation tmp_methy_sub_block;

///void finalizeTXOutput()
void finalizeOutput()
{
  fclose(_out_fp);
}

void init_buffer_sub_block(Output_buffer_sub_block* sub_block)
{
	sub_block->length = 0;
	sub_block->size = sub_block_init_length;
	sub_block->buffer = (char*)malloc(sub_block->size);
}




///��ʼ��������
void init_output_buffer(int thread_number)
{


	///ouput_buffer_size
	///size�ǿռ��С
	buffer_out.sub_block_size = ouput_buffer_size * thread_number;
	buffer_out.sub_block_number = 0;

	buffer_out.sub_buffer = (Output_buffer_sub_block*)malloc(sizeof(Output_buffer_sub_block)*buffer_out.sub_block_size);



	for (int i = 0; i < buffer_out.sub_block_size; i++)
	{
		init_buffer_sub_block(&(buffer_out.sub_buffer[i]));
	}

	buffer_out.all_buffer_end = 0;
}


void init_output_methy_buffer(int thread_number)
{


	///ouput_buffer_size
	///size�ǿռ��С
	buffer_methy_out.sub_block_size = methylation_buffer_times * thread_number;
	
	buffer_methy_out.sub_block_number = 0;

	buffer_methy_out.sub_buffer =
		(Methylation*)malloc(sizeof(Methylation)*buffer_methy_out.sub_block_size);


	
	for (int i = 0; i < buffer_methy_out.sub_block_size; i++)
	{
		init_methylation(&(buffer_methy_out.sub_buffer[i]), methylation_size);
	}

	buffer_methy_out.all_buffer_end = 0;
	
}


inline int if_empty_buffer()
{

	if (buffer_out.sub_block_number == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


inline int if_empty_methy_buffer()
{

	if (buffer_methy_out.sub_block_number == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}



inline int if_full_buffer()
{

	if (buffer_out.sub_block_number >= buffer_out.sub_block_size)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


inline int if_full_methy_buffer()
{

	if (buffer_methy_out.sub_block_number >= buffer_methy_out.sub_block_size)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


///�����Ҫȡһ�����ȡ��curr_sub_block��
inline void pop_single_buffer(Output_buffer_sub_block* curr_sub_block)
{
	buffer_out.sub_block_number--;
	///ֻ����ָ��
	///�ǵ�Ҫ������ֱ�Ӹ�ֵ���ڴ�й¶�����Ұָ��
	char *k;
	k = buffer_out.sub_buffer[buffer_out.sub_block_number].buffer;
	buffer_out.sub_buffer[buffer_out.sub_block_number].buffer = curr_sub_block->buffer;
	curr_sub_block->buffer = k;

	
	long long tmp_size;
	tmp_size = curr_sub_block->size;
	curr_sub_block->size = buffer_out.sub_buffer[buffer_out.sub_block_number].size;
	buffer_out.sub_buffer[buffer_out.sub_block_number].size = tmp_size;


	curr_sub_block->length = buffer_out.sub_buffer[buffer_out.sub_block_number].length;
	buffer_out.sub_buffer[buffer_out.sub_block_number].length = 0;




}








///buffer_methy_out


inline void pop_single_methy_buffer(Methylation* curr_sub_block)
{
	buffer_methy_out.sub_block_number--;

	


	

	///����ռ�Ӧ����genome_cut+1
	bitmapper_bs_iter* cut_index;   ///���Ҫ����
	///���ĸ����鹹��һ��������Ԫ��
	bitmapper_bs_iter* sites;  ///���Ҫ����
	uint16_t* r_length;  ///���Ҫ����
	uint16_t* r_size;  ///���Ҫ����
	char** reads;  ///���Ҫ����


	cut_index = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].cut_index;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].cut_index = curr_sub_block->cut_index;
	curr_sub_block->cut_index = cut_index;

	sites = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].sites;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].sites = curr_sub_block->sites;
	curr_sub_block->sites = sites;

	r_length = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_length;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_length = curr_sub_block->r_length;
	curr_sub_block->r_length = r_length;

	r_size = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_size;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_size = curr_sub_block->r_size;
	curr_sub_block->r_size = r_size;


	reads = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].reads;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].reads = curr_sub_block->reads;
	curr_sub_block->reads = reads;

	
	curr_sub_block->current_size = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].current_size;
	///���clearӦ���ǿ��Բ�Ҫ��
	clear_methylation(&(buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number]));




}





///�����Ҫ��curr_sub_block�Ľ���ŵ�������
inline void push_single_buffer(Output_buffer_sub_block* curr_sub_block)
{


	///ֻ����ָ��
	///�ǵ�Ҫ������ֱ�Ӹ�ֵ���ڴ�й¶�����Ұָ��
	char *k;
	k = buffer_out.sub_buffer[buffer_out.sub_block_number].buffer;
	buffer_out.sub_buffer[buffer_out.sub_block_number].buffer = curr_sub_block->buffer;
	curr_sub_block->buffer = k;


	long long tmp_size;
	tmp_size = curr_sub_block->size;
	curr_sub_block->size = buffer_out.sub_buffer[buffer_out.sub_block_number].size;
	buffer_out.sub_buffer[buffer_out.sub_block_number].size = tmp_size;




	buffer_out.sub_buffer[buffer_out.sub_block_number].length = curr_sub_block->length;
	curr_sub_block->length = 0;



	buffer_out.sub_block_number++;
}




///�����Ҫ��curr_sub_block�Ľ���ŵ�������
inline void push_single_methy_buffer(Methylation* curr_sub_block)
{



	///����ռ�Ӧ����genome_cut+1
	bitmapper_bs_iter* cut_index;   ///���Ҫ����
	///���ĸ����鹹��һ��������Ԫ��
	bitmapper_bs_iter* sites;  ///���Ҫ����
	uint16_t* r_length;  ///���Ҫ����
	uint16_t* r_size;  ///���Ҫ����
	char** reads;  ///���Ҫ����


	cut_index = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].cut_index;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].cut_index = curr_sub_block->cut_index;
	curr_sub_block->cut_index = cut_index;

	sites = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].sites;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].sites = curr_sub_block->sites;
	curr_sub_block->sites = sites;

	r_length = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_length;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_length = curr_sub_block->r_length;
	curr_sub_block->r_length = r_length;

	r_size = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_size;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].r_size = curr_sub_block->r_size;
	curr_sub_block->r_size = r_size;


	reads = buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].reads;
	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].reads = curr_sub_block->reads;
	curr_sub_block->reads = reads;





	buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number].current_size = curr_sub_block->current_size;
	///���Ӧ�÷����ٽ���֮����
	///clear_methylation(curr_sub_block);



	buffer_methy_out.sub_block_number++;
}


void finish_output_buffer()
{
	if (output_methy == 0)
	{
		///���ҲҪ�ӻ�����
		pthread_mutex_lock(&o_doneMutex);
		buffer_out.all_buffer_end++;


		if (buffer_out.all_buffer_end == THREAD_COUNT)
			pthread_cond_signal(&o_flushCond);
		pthread_mutex_unlock(&o_doneMutex);
	}
	else
	{



		///���ҲҪ�ӻ�����
		pthread_mutex_lock(&o_doneMutex);
		buffer_methy_out.all_buffer_end++;


		if (buffer_methy_out.all_buffer_end == THREAD_COUNT)
			pthread_cond_signal(&o_flushCond);
		pthread_mutex_unlock(&o_doneMutex);
	}

	
}


void push_results_to_buffer(Output_buffer_sub_block* sub_block)
{
	
	pthread_mutex_lock(&o_queueMutex);

	///if_full_buffer
	while (if_full_buffer())
	{
		///����������ź����ƺ����÷�
		///��Ϊ���в�����һ����һ�߿�
		pthread_cond_signal(&o_flushCond);


		pthread_cond_wait(&o_stallCond, &o_queueMutex);
	}


	push_single_buffer(sub_block);



	pthread_cond_signal(&o_flushCond);
	pthread_mutex_unlock(&o_queueMutex);
	
}


void push_methy_to_buffer(Methylation* methy)
{

	pthread_mutex_lock(&o_queueMutex);

	///if_full_buffer
	while (if_full_methy_buffer())
	{
		///����������ź����ƺ����÷�
		///��Ϊ���в�����һ����һ�߿�
		pthread_cond_signal(&o_flushCond);


		pthread_cond_wait(&o_stallCond, &o_queueMutex);
	}


	push_single_methy_buffer(methy);



	pthread_cond_signal(&o_flushCond);
	pthread_mutex_unlock(&o_queueMutex);

	clear_methylation(methy);

}


void* pop_buffer(void*)
{
	
	FILE* output_file = get_Ouput_Dec();

	init_buffer_sub_block(&tmp_buffer_sub_block);

	

	while (buffer_out.all_buffer_end<THREAD_COUNT)
	{

		pthread_mutex_lock(&o_queueMutex);


		while (if_empty_buffer() && (buffer_out.all_buffer_end<THREAD_COUNT))
		{
			///����������ź����ƺ����÷�
			///��Ϊ���в�����һ����һ�߿�
			pthread_cond_signal(&o_stallCond);


			pthread_cond_wait(&o_flushCond, &o_queueMutex);
		}


		if (!if_empty_buffer())
		{
			pop_single_buffer(&tmp_buffer_sub_block);
		}

		pthread_cond_signal(&o_stallCond);
		pthread_mutex_unlock(&o_queueMutex);

		if (tmp_buffer_sub_block.length != 0)
		{
			fprintf(output_file, "%s", tmp_buffer_sub_block.buffer);
		}

	}


	while (buffer_out.sub_block_number>0)
	{
		buffer_out.sub_block_number--;
		fprintf(output_file, "%s", buffer_out.sub_buffer[buffer_out.sub_block_number].buffer);

	}

}



void* pop_methy_buffer(void*)
{
	///Ҫ�������С��ÿ���߳��ڲ���block��Сһ��
	int my_methylation_size = methylation_size;

	init_methylation(&tmp_methy_sub_block, my_methylation_size);


	while (buffer_methy_out.all_buffer_end<THREAD_COUNT)
	{

		pthread_mutex_lock(&o_queueMutex);


		while (if_empty_methy_buffer() && (buffer_methy_out.all_buffer_end<THREAD_COUNT))
		{
			///����������ź����ƺ����÷�
			///��Ϊ���в�����һ����һ�߿�
			pthread_cond_signal(&o_stallCond);


			pthread_cond_wait(&o_flushCond, &o_queueMutex);
		}


		if (!if_empty_methy_buffer())
		{
			pop_single_methy_buffer(&tmp_methy_sub_block);
		}

		pthread_cond_signal(&o_stallCond);
		pthread_mutex_unlock(&o_queueMutex);

		if (tmp_methy_sub_block.current_size != 0)
		{
			///�������������û��֣������ڼ����߳�����
			///assign_cuts(&tmp_methy_sub_block);

			output_methylation(&tmp_methy_sub_block);

			///������Ҳ�ǲ��������
			///clear_methylation(&tmp_methy_sub_block);
		}

	}


	while (buffer_methy_out.sub_block_number>0)
	{
		buffer_methy_out.sub_block_number--;
		
		///�������������û��֣������ڼ����߳�����
		///assign_cuts(&tmp_methy_sub_block);

		output_methylation(&(buffer_methy_out.sub_buffer[buffer_methy_out.sub_block_number]));

		///������Ҳ�ǲ��������
		///clear_methylation(&tmp_methy_sub_block);

	}

}


void OutPutSAM_Nounheader(_rg_name_l  *_ih_refGenName, int refChromeCont, int argc, char *argv[])
{
	fprintf(_out_fp, "@HD\tVN:1.4\tSO:unsorted\n");
	int i = 0;
	for (i = 0; i<refChromeCont; i++)
	{
		fprintf(_out_fp, "@SQ\tSN:%s\tLN:%u\n", _ih_refGenName[i]._rg_chrome_name, _ih_refGenName[i]._rg_chrome_length);
	}

	fprintf(_out_fp, "@PG\tID:BitMapperBS\tVN:%s\tCL:", versionN);
	for (size_t i = 0; i < argc; i++)
	{
		fprintf(_out_fp, "%s ", argv[i]);
	}

	fprintf(_out_fp, "\n");
}

FILE* get_Ouput_Dec()
{
    return _out_fp;
}


void multi_outputQ(SAM map, int thread_id)
{
    if(map.FLAG<=16)
    {
        fprintf(_out_fp, "%s\t%d\t%s\t%u\t%d\t%s\t%s\t%d\t%d\t%s\t%s",
	  map.QNAME,
	  map.FLAG,
	  map.RNAME,
	  map.POS,
	  map.MAPQ,
	  map.CIGAR,
	  map.MRNAME,
	  map.MPOS,
	  map.ISIZE,
	  map.SEQ,
	  map.QUAL);
    }
    else
    {
        fprintf(_out_fp, "%s\t%d\t%s\t%u\t%d\t%s\t%s\t%d\t%d\t%c\t%c",
	  map.QNAME,
	  map.FLAG,
	  map.RNAME,
	  map.POS,
	  map.MAPQ,
	  map.CIGAR,
	  map.MRNAME,
	  map.MPOS,
	  map.ISIZE,
	  '*',
	  '*');
    }
  int i;

  for ( i = 0; i < map.optSize; i++)
    {
      switch (map.optFields[i].type)
	{
	case 'A':
	  fprintf(_out_fp, "\t%s:%c:%c", map.optFields[i].tag, map.optFields[i].type, map.optFields[i].cVal);
	  break;
	case 'i':
	  fprintf(_out_fp, "\t%s:%c:%d", map.optFields[i].tag, map.optFields[i].type, map.optFields[i].iVal);
	  break;
	case 'f':
	  fprintf(_out_fp, "\t%s:%c:%f", map.optFields[i].tag, map.optFields[i].type, map.optFields[i].fVal);
	  break;
	case 'Z':
	case 'H':
	  fprintf(_out_fp, "\t%s:%c:%s", map.optFields[i].tag, map.optFields[i].type, map.optFields[i].sVal);
	  break;
	}
    }

  fprintf(_out_fp, "\n");
}






int Output_gene ( char *fileName)
{


    _out_fp = fopen (fileName, "w");
    char tmpFileName[NAME_LENGTH];

    if (_out_fp == NULL)
	{
	  return 0;
	}
    ///finalizeOutput = &finalizeTXOutput;

  return 1;
}

