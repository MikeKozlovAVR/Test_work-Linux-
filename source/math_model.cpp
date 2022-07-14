#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "tcplib.cpp"
#include "queue.cpp"
#include "signal.cpp"
#include "modelbody.cpp"
using namespace std;

int modeling_time = 0;  // глобальная переменная времени моделирования, (если 0 то бесконечно)

struct pullsignal_data_t{
	Mathmodel* mathmodel_p;
};

//Описание потока с методом update
void *thread_Update_f(void *arg){
	pullsignal_data_t* usdata = (pullsignal_data_t*)arg;
	Mathmodel* mathmodel_p = usdata->mathmodel_p;
	Queue* signals_p = mathmodel_p->signals_p;
	TCP_client* tcp_client_p = mathmodel_p->tcp_client_p;
	sem_t* sem_recvsignal_p = mathmodel_p->sem_recvsignal_p;
	int cycleN = 0;
	int cycleMax = 1;
	long microsec = (long)(mathmodel_p->modelData.t_discr*1000000);
	char sendVal[256];
	int signalID = 0;
	
	//отправка дефолтного состояния объекта в control_system;
	float outputval = mathmodel_p->modelData.output_prev;
	create_signalMessage(sendVal, "math_model_output", signalID, outputval);
	tcp_client_p->writeData(mathmodel_p->tcp_sockedID, sendVal);
	signalID++;
	if (modeling_time != 0)    //проверка условия бесконечности цикла
	{
		cycleMax = (int)((float)modeling_time / mathmodel_p->modelData.t_discr);
		cycleN++;
	}
	usleep(microsec); 				// ожидание первого интервала
	while (1)
	{
		sem_wait(sem_recvsignal_p);				// ожидание сигнала от семафора
		signal_t* inSignalMess;
		inSignalMess = (signal_t*)signals_p->pop();
		if (inSignalMess != NULL)
		{
			cout<<"->signal["<< signals_p->get_size() <<"]: "<< inSignalMess->name << " [ID:"<< inSignalMess->id << "] "<< inSignalMess->value <<"\n";
				
			if (cycleN == cycleMax + 1)
			{
				break;
			}
			mathmodel_p->modelData.input = atof(inSignalMess->value);
			mathmodel_p->update();
			cout<<"<----model output=" << mathmodel_p->modelData.output <<"\n";
			outputval = mathmodel_p->modelData.output;
			create_signalMessage(sendVal, "model_output", signalID, outputval);
			tcp_client_p->writeData(mathmodel_p->tcp_sockedID, sendVal);
			signalID++;
			free(inSignalMess);
			if (modeling_time != 0)
			{
				cycleN = signalID;
			}
		}
		else{ cout << "Error: in a queue null element\n"; }
		usleep(microsec); //ожидание заданного интервала
	}
	cout << "- Cycle ended\n";
	sem_destroy(sem_recvsignal_p);
	pthread_exit(0);
}

bool argv_parseOK(int argc, char *argv[], Mathmodel* mathmodel){
	bool isOK = 0;
	int i=1;
	bool port_in_en = 0;
	bool port_out_en = 0;
	bool tm_en = 0;
	bool km_en = 0;
	bool t_discr_en = 0;
	if (argc == 11)
	{
		for (i=1; i< argc; i++)
		{
			if(string(argv[i])=="-i"){
				mathmodel->modelData.port_in = atoi(argv[i + 1]);
				port_in_en = 1;
			}
			if(string(argv[i])=="-o"){
				mathmodel->modelData.port_out = atoi(argv[i + 1]);
				port_out_en = 1;
			}
			if(string(argv[i])=="--Tm"){
				mathmodel->modelData.tm = atof(argv[i + 1]);
				tm_en = 1;
			}
			if(string(argv[i])=="--Km"){
				mathmodel->modelData.km = atof(argv[i + 1]);
				km_en = 1;
			}
			if(string(argv[i])=="-t"){
				mathmodel->modelData.t_discr = atof(argv[i + 1]);
				t_discr_en = 1;
			}
		}
		isOK = port_in_en & port_out_en & tm_en & km_en & t_discr_en;
	}
	else { return 0; }
	return isOK;
}

int main(int argc, char *argv[]){

	Mathmodel mathmodel;
	if (argv_parseOK(argc, argv, &mathmodel))
	{
		mathmodel.showParameters();
	}
	else
	{
		cout << "- error: Wrong arguments!\n";
		//return 0;
		
		cout << "- Will use defaults:\n";				//default для тестов
		mathmodel.modelData.port_in = 8004;
		mathmodel.modelData.port_out = 8005;
		mathmodel.modelData.tm = 0.5;
		mathmodel.modelData.km = 10;
		mathmodel.modelData.t_discr = 0.01;
		mathmodel.showParameters();
	}
	mathmodel.init();
	
	//обработка команд от пользователя
	string command;
	while(1)
	{
		cout << "- Enter the 'start'\n";
		cin >> command;
		if (command == "start")
		{
			modeling_time = -1;
			while (modeling_time < 0)   //небольшая "защита от дураков" :)
			{
				cout << "- Enter the modeling time (sec) or '0' for infinity cycle\n";
				char* timeStr;
				cin >>timeStr;
				modeling_time = atoi(timeStr);
			}
			cout << "- Process started\n";
			//инициализация потока с методом update - потребителя (обработчика) очереди сигналов
			pullsignal_data_t updatesignal_arg;
			updatesignal_arg.mathmodel_p = &mathmodel;
			pullsignal_data_t* updatesignal_arg_p;
			updatesignal_arg_p = new pullsignal_data_t;
			updatesignal_arg_p = &updatesignal_arg;
			pthread_t thread_update;             //идентификатор потока с методом update
			pthread_create(&thread_update, NULL, thread_Update_f, (void *)updatesignal_arg_p);
			
			pthread_join(thread_update, NULL);
			return 0;
		}
		else 
		{
			cout << "- Wrong command\n";
		}
	}
	return 0;
}








