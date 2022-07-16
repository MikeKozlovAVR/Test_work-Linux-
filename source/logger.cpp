#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "loggerbody.cpp"
using namespace std;

struct pullsignal_data_t{
	Logger* logger_p;
};

//Описание потока с методом update
void *thread_Update_f(void *arg){
	pullsignal_data_t* usdata = (pullsignal_data_t*)arg;
	Logger* logger_p = usdata->logger_p;
	while (1)
	{
		logger_p->waiting_newsignal();								// ожидание сигнала от семафора
		if (logger_p->get_signalsCount() >= 5)
		{
			cout<< "------------------------------------------\n";
			int counter = 0;
			while (counter < 5)
			{
				logger_p->pop_and_parse_signal();
				counter++;
			}
			logger_p->update();
		}
	}
	logger_p->stop_waiting_newsignal();
	pthread_exit(0);
}

void showHelp(){
	cout << "HELP: Application arguments:\n";	
	cout << "\t-i[in_port]\t\t:input signals TCP port; \n";	
	cout << "\t-p[path]\t\t:path to the log-file; \n";
	cout << "\tWithout arguments - starting with default settings; \n";
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
	if (argc == 2 && ((string(argv[1]) == "--help") || (string(argv[1]) == "-h")))
	{
		showHelp();
		return 0;
	}
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
		logger.showParameters();
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








