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
#include <fstream>
#include "loggerbody.cpp"
using namespace std;

struct pullsignal_data_t{
	Logger* logger_p;
};

//Описание потока с методом update
void *thread_Update_f(void *arg){
	pullsignal_data_t* usdata = (pullsignal_data_t*)arg;
	Logger* logger_p = usdata->logger_p;
	Queue* signals_p = logger_p->signals_p;
	sem_t* sem_recvsignal_p = logger_p->sem_recvsignal_p;
	while (1)
	{
		sem_wait(sem_recvsignal_p);				// ожидание сигнала от семафора
		if (signals_p->get_size() >= 5)
		{
			cout<< "------------------------------------------\n";
			int counter = 0;
			while (counter < 5)
			{
				signal_t* inSignalMess;
				inSignalMess = (signal_t*)signals_p->pop();
				if (inSignalMess != NULL)
				{
					if (string(inSignalMess->name) == "math_in")
					{
						logger_p->loggerData.log_data.math_in = inSignalMess->value;
					}
					if (string(inSignalMess->name) == "math_out")
					{
						logger_p->loggerData.log_data.math_out = inSignalMess->value;
					}
					if (string(inSignalMess->name) == "control_in")
					{
						logger_p->loggerData.log_data.control_in = inSignalMess->value;
					}
					if (string(inSignalMess->name) == "control_fb")
					{
						logger_p->loggerData.log_data.control_fb = inSignalMess->value;
					}
					if (string(inSignalMess->name) == "control_out")
					{
						logger_p->loggerData.log_data.control_out = inSignalMess->value;
					}
					cout<<"->signal["<< signals_p->get_size() <<"]: "<< inSignalMess->name << " [ID:"<< inSignalMess->id << "] "<< inSignalMess->value <<"\n";
					
					counter++;
					free(inSignalMess);
				}
				else{ cout << "Error: in a queue null element\n"; }
			}
			logger_p->update();
		}
	}
	pthread_exit(0);
}

bool argv_parseOK(int argc, char *argv[], Logger* logger){
	bool isOK = 0;
	int i=1;
	bool port_in_en = 0;
	bool path_en = 0;
	if (argc == 5)
	{
		for (i=1; i< argc; i++)
		{
			if(string(argv[i])=="-i"){
				logger->loggerData.port_in = atoi(argv[i + 1]);
				port_in_en = 1;
			}
			if(string(argv[i])=="-p"){
				logger->loggerData.path = argv[i + 1];
				path_en = 1;
			}
		}
		isOK = port_in_en & path_en;
	}
	else { return 0; }
	return isOK;
}

int main(int argc, char *argv[]){

	Logger logger;
	if (argv_parseOK(argc, argv, &logger))
	{
		logger.showParameters();
	}
	else
	{
		cout << "- error: Wrong arguments!\n";
		//return 0;
		
		cout << "- Will use defaults:\n";				//default для тестов
		logger.loggerData.port_in = 8006;
		logger.loggerData.path = "log.csv";
	}
	logger.init();
	
	//инициализация потока потребителя (обработчика) очереди сигналов
	pullsignal_data_t updatesignal_arg;
	updatesignal_arg.logger_p = &logger;
	pullsignal_data_t* updatesignal_arg_p;
	updatesignal_arg_p = new pullsignal_data_t;
	updatesignal_arg_p = &updatesignal_arg;
	pthread_t thread_update;             //идентификатор потока потребителя очереди сигналов
	pthread_create(&thread_update, NULL, thread_Update_f, (void *)updatesignal_arg_p);
	
	pthread_join(thread_update, NULL);
	
	return 0;
}








