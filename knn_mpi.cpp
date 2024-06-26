#include<iostream>
#include<fstream>
#include<string>
#include<string.h>
#include<cmath>
#include<sstream>
#include"mpi.h"
#include<algorithm>
#include<time.h>

using namespace std;

struct train_data_dis
{
	/*
	train_data_dis�ṹ�崢���˲��Լ�����֤����ĳһ������ѵ������ĳһ����ľ����Լ���������ꡣͨ����������ṹ�����ǿ��Ը��������KNN�Ĺ�����ѡ�������С��K���������Լ���Ӧ��ꡣ
	label����ѵ�������������
	dis�����Լ�����֤�����ѵ���������ľ���
	*/
	int label;
	double dis;
};

bool Comp(train_data_dis a, train_data_dis b)
{
	/*
	Comp�����Զ����˿��������㷨sort�ıȽϺ�����ͨ�����ַ�ʽ���ǿ��Զ�train_data_dis��Ķ�����ݴ���ľ����С��������
	a��b����Ҫ�Ƚϵ���������
	*/
	return a.dis < b.dis;
}

double Euclidean_D(int a, int b, int dim, double* Data_train, double* Data_test_buffer)
{
	/*
	Euclidean_D�������ص�������������֮��ľ��룬����ʹ�õĵķ�ʽ��ŷʽ���롣
	a�����Լ�����֤��Ŀ����������
	b��ѵ����Ŀ����������
	dim���������ά��
	Data_train������ѵ����
	Data_test_buffer�������ڸý��̻������ڵĲ��Լ�����֤��
	*/
	double result = 0;
	for (int i = 0; i < dim; i++)
	{
		result += (Data_test_buffer[a * dim + i] - Data_train[b * dim + i]) * (Data_test_buffer[a * dim + i] - Data_train[b * dim + i]);
	}
	result = sqrt(result);
	return result;
}
double Manhattan_D(int a, int b, int dim, double* Data_train, double* Data_test_buffer)
{
	/*
	Manhattan_D�������ص�������������֮��ľ��룬����ʹ�õĵķ�ʽ�������پ��롣
	a�����Լ�����֤��Ŀ����������
	b��ѵ����Ŀ����������
	dim���������ά��
	Data_train������ѵ����
	Data_test_buffer�������ڸý��̻������ڵĲ��Լ�����֤��
	*/
	double result = 0;
	for (int i = 0; i < dim; i++)
	{
		result += abs(Data_test_buffer[a * dim + i] - Data_train[b * dim + i]);
	}
	return result;
}

double acc_calc(int* real_label, int* label, int size)
{
	/*
	acc_calc�����ڼ����㷨��֤��Ԥ������׼ȷ�ʵģ�������֤������ʵ����Լ�KNNԤ����꣬����ֵ׼ȷ�ʡ�
	real_label����֤������ʵ���
	label���㷨Ԥ�������֤�����
	size����֤����С
	*/
	double acc = 0;
	for (int i = 0; i < size; i++)
	{
		if (real_label[i] == label[i]) acc++;
	}
	acc /= size;
	return acc;
}

int main(int argc, char* argv[])
{
	int dim, N_train, N_test, N_val, K, class_cnt;
	bool Euclidean_distance, Normalize, Validation;

	double* max_value, * min_value;
	double* max_value_buffer, * min_value_buffer;

	double* Data_train; //ѵ��������
	int* Train_label; //ѵ�������

	double* Data_test; //���Լ�����
	double* Data_test_buffer; //�ý��̻������ڲ��Լ�����
	int* Test_label; //���Լ�Ԥ�����
	int* Test_label_buffer; //�ý��̻������ڲ��Լ�Ԥ�����

	double* Data_val; //��֤������
	double* Data_val_buffer; //�ý��̻���������֤������
	int* Val_label; //��֤��Ԥ�����
	int* Val_label_buffer; //�ý��̻���������֤��Ԥ�����
	int* Val_label_real; //��֤����ʵ���

	dim = 784; //�������ά��
	K = 50; //KNN�е�Kֵ
	N_train = 60000; //ѵ������С
	N_test = 10000; //���Լ���С
	N_val = 10000; //��֤����С
	class_cnt = 10; //�������
	Euclidean_distance = true; //�Ƿ�����ŷʽ���룬Ϊfalse��ʹ�������پ���
	Normalize = true; //�Ƿ��һ����Ϊfalse�򲻶�ѵ������֤�����Լ����ݽ��й�һ��
	Validation = true; //�Ƿ�ʹ����֤����Ϊfalse��������֤��
	string train_file_name = "mnist_train.csv"; //ѵ�����Ĵ洢·�����ļ���
	string validation_file_name = "mnist_validation.csv"; //��֤���Ĵ洢·�����ļ���
	string test_file_name = "mnist_test.csv"; //���Լ��Ĵ洢·�����ļ���

	int myid, numprocs;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid); //��ȡ��ǰ����id
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs); //��ȡ�ܽ�����

	if (N_train % numprocs != 0) MPI_Abort(MPI_COMM_WORLD, 1);
	if (N_test % numprocs != 0) MPI_Abort(MPI_COMM_WORLD, 1);
	if (N_val % numprocs != 0) MPI_Abort(MPI_COMM_WORLD, 1);

	double start, finish;
	double totaltime; //�㷨��������ʱ��
	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();

	int batch_test = N_test / numprocs;
	int batch_train = N_train / numprocs; 
	int batch_val = N_val / numprocs;

	Data_train = new double[N_train * dim];
	Train_label = new int[N_train];

	Data_test = new double[N_test * dim];
	Data_test_buffer = new double[batch_test * dim];
	Test_label = new int[N_test];
	Test_label_buffer = new int[batch_test];

	Data_val = new double[N_val * dim];
	Val_label_real = new int[N_val];
	Data_val_buffer = new double[batch_val * dim];
	Val_label = new int[N_val];
	Val_label_buffer = new int[batch_val];

	if (myid == 0)
	{
		/*
		����0��ȡѵ��������������Ϣ�����ֱ𴢴�
		*/
		ifstream infile;
		infile.open(train_file_name);
		string s;
		int cnt = 0;
		while (getline(infile, s))
		{
			stringstream ss(s);
			string a;
			while (getline(ss, a, ','))
			{
				if (cnt % (dim + 1) == 0) Train_label[cnt / (dim + 1)] = atoi(a.c_str());
				else Data_train[cnt - (cnt / (dim + 1)) - 1] = atof(a.c_str());
				cnt++;
			}
		}
		infile.close();
	}

	if (myid == 1)
	{
		/*
		����1��ȡ���Լ�������������Ϣ
		*/
		ifstream infile;
		infile.open(test_file_name);
		string s;
		int cnt = 0;
		while (getline(infile, s))
		{
			stringstream ss(s);
			string a;
			while (getline(ss, a, ','))
			{
				Data_test[cnt] = atof(a.c_str());
				cnt++;
			}
		}
		infile.close();
	}
	if (Validation)
	{
		if (myid == 2)
		{
			/*
			����2��ȡ��֤������������Ϣ����ʵ���ֱ𴢴�
			*/
			ifstream infile;
			infile.open(validation_file_name);
			string s;
			int cnt = 0;
			while (getline(infile, s))
			{
				stringstream ss(s);
				string a;
				while (getline(ss, a, ','))
				{
					if (cnt % (dim + 1) == 0) Val_label_real[cnt / (dim + 1)] = atoi(a.c_str());
					else Data_val[cnt - (cnt / (dim + 1)) - 1] = atof(a.c_str());
					cnt++;
				}
			}
			infile.close();
		}
	}

	MPI_Bcast(Data_train, N_train * dim, MPI_DOUBLE, 0, MPI_COMM_WORLD); //����0�㲥ѵ����������Ϣ
	MPI_Bcast(Train_label, N_train, MPI_INT, 0, MPI_COMM_WORLD); //����0�㲥ѵ���������Ϣ
	MPI_Scatter(Data_test, batch_test * dim, MPI_DOUBLE, Data_test_buffer, batch_test * dim, MPI_DOUBLE, 1, MPI_COMM_WORLD); //����1�ַ����Լ�������Ϣ
	if(Validation) MPI_Scatter(Data_val, batch_val* dim, MPI_DOUBLE, Data_val_buffer, batch_val* dim, MPI_DOUBLE, 2, MPI_COMM_WORLD); //����2�ַ���֤��������Ϣ

	if (Normalize)
	{
		/*
		��һ��ѵ��������֤���Լ����Լ���������Ϣ
		*/
		max_value = new double[dim]; //�洢ÿһά��������Ϣ��ȫ�����ֵ
		min_value = new double[dim]; //�洢ÿһά��������Ϣ��ȫ����Сֵ

		max_value_buffer = new double[dim]; //�洢ÿһά��������Ϣ�ڵ�ǰ���̵����ֵ
		min_value_buffer = new double[dim]; //�洢ÿһά��������Ϣ�ڵ�ǰ���̵���Сֵ
		for (int i = 0; i < dim; i++)
		{
			max_value_buffer[i] = -1;
			min_value_buffer[i] = 999999;
		}

		for (int i = 0; i < batch_train; i++)
		{
			for (int j = 0; j < dim; j++)
			{
				double data = Data_train[(myid * batch_train * dim) + (i * dim) + j];
				if (data > max_value_buffer[j]) max_value_buffer[j] = data;
				if (data < min_value_buffer[j]) min_value_buffer[j] = data;
			}
		}
		for (int i = 0; i < batch_test; i++)
		{
			for (int j = 0; j < dim; j++)
			{
				double data = Data_test_buffer[i * dim + j];
				if (data > max_value_buffer[j]) max_value_buffer[j] = data;
				if (data < min_value_buffer[j]) min_value_buffer[j] = data;
			}
		}
		if (Validation)
		{
			for (int i = 0; i < batch_val; i++)
			{
				for (int j = 0; j < dim; j++)
				{
					double data = Data_val_buffer[i * dim + j];
					if (data > max_value_buffer[j]) max_value_buffer[j] = data;
					if (data < min_value_buffer[j]) min_value_buffer[j] = data;
				}
			}
		}

		MPI_Allreduce(max_value_buffer, max_value, dim, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD); //ͳ��ÿһά��������Ϣ��ȫ�����ֵ
		MPI_Allreduce(min_value_buffer, min_value, dim, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD); //ͳ��ÿһά��������Ϣ��ȫ����Сֵ

		for (int i = 0; i < N_train; i++)
		{
			for (int j = 0; j < dim; j++)
			{
				int aim = i * dim + j;
				if (max_value[j] - min_value[j] != 0) Data_train[aim] = (Data_train[aim] - min_value[j]) / (max_value[j] - min_value[j]);
			}
		}
		for (int i = 0; i < batch_test; i++)
		{
			for (int j = 0; j < dim; j++)
			{
				int aim = i * dim + j;
				if (max_value[j] - min_value[j] != 0) Data_test_buffer[aim] = (Data_test_buffer[aim] - min_value[j]) / (max_value[j] - min_value[j]);
			}
		}
		if (Validation)
		{
			for (int i = 0; i < batch_val; i++)
			{
				for (int j = 0; j < dim; j++)
				{
					int aim = i * dim + j;
					if (max_value[j] - min_value[j] != 0) Data_val_buffer[aim] = (Data_val_buffer[aim] - min_value[j]) / (max_value[j] - min_value[j]);
				}
			}
		}
	}

	if (Validation)
	{
		/*
		����֤������������KNN
		*/
		train_data_dis* d1;
		d1 = new train_data_dis[N_train];
		for (int i = 0; i < batch_val; i++)
		{
			for (int j = 0; j < N_train; j++)
			{
				d1[j].label = Train_label[j];
				if (Euclidean_distance) d1[j].dis = Euclidean_D(i, j, dim, Data_train, Data_val_buffer);
				else d1[j].dis = Manhattan_D(i, j, dim, Data_train, Data_val_buffer);
			}
			sort(d1, d1 + N_train, Comp);
			double max_cnt = 0; int max_label = -1;
			double* label_cnt;
			label_cnt = new double[class_cnt];
			for (int j = 0; j < class_cnt; j++) label_cnt[j] = 0;
			for (int j = 0; j < K; j++)
			{
				label_cnt[d1[j].label] ++;
				if (label_cnt[d1[j].label] > max_cnt)
				{
					max_label = d1[j].label;
					max_cnt = label_cnt[d1[j].label];
				}
			}
			Val_label_buffer[i] = max_label;
		}

		MPI_Gather(Val_label_buffer, batch_val, MPI_INT, Val_label, batch_val, MPI_INT, 2, MPI_COMM_WORLD); //ͳ��ȫ����֤�������������2

		if (myid == 2)
		{
			/*
			�ڽ���2�м�����֤��Ԥ��׼ȷ��
			*/
			double acc = acc_calc(Val_label_real, Val_label, N_val);
			cout << "accuracy = " << acc << endl;
		}
	}

	/*
	�Բ��Լ�����������KNN
	*/
	train_data_dis* d2;
	d2 = new train_data_dis[N_train];

	for (int i = 0; i < batch_test; i++)
	{
		for (int j = 0; j < N_train; j++)
		{
			d2[j].label = Train_label[j];
			if (Euclidean_distance) d2[j].dis = Euclidean_D(i, j, dim, Data_train, Data_test_buffer);
			else d2[j].dis = Manhattan_D(i, j, dim, Data_train, Data_test_buffer);
		}
		sort(d2, d2 + N_train, Comp);
		double max_cnt = 0; int max_label = -1;
		double* label_cnt;
		label_cnt = new double[class_cnt];
		for (int j = 0; j < class_cnt; j++) label_cnt[j] = 0;
		for (int j = 0; j < K; j++)
		{
			label_cnt[d2[j].label] ++;
			if (label_cnt[d2[j].label] > max_cnt)
			{
				max_label = d2[j].label;
				max_cnt = label_cnt[d2[j].label];
			}
		}
		Test_label_buffer[i] = max_label;
	}

	MPI_Gather(Test_label_buffer, batch_test, MPI_INT, Test_label, batch_test, MPI_INT, 1, MPI_COMM_WORLD);//ͳ��ȫ�ֲ��Լ������������1

	if (myid == 1)
	{
		/*
		�ڽ���1��������Լ�Ԥ�����
		*/
		ofstream outfile("Test_label.csv");
		for (int i = 0; i < N_test; i++) outfile << Test_label[i] << endl;
		outfile.close();
	}

	MPI_Barrier(MPI_COMM_WORLD);
	finish = MPI_Wtime();
	MPI_Finalize();
	if (!myid) cout << "Running time is " << (double)finish - start << " second" << endl;
}