#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
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
	int cycleN = 0;
	int cycleMax = 1;
	long microsec = (long)(mathmodel_p->modelData.t_discr*1000000);
	int signalID = 0;
	mathmodel_p->send_modelOutputPrev(signalID);	//отправка дефолтного сост. объекта в control_system
	signalID++;
	if (modeling_time != 0)    //проверка условия бесконечности цикла
	{
		cycleMax = (int)((float)modeling_time / mathmodel_p->modelData.t_discr);
		cycleN++;
	}
	usleep(microsec); 				// ожидание первого интервала
	while (1)
	{
		mathmodel_p->waiting_newsignal();				// ожидание сигнала от семафора
		float contrl_value = mathmodel_p->pop_controlsignal_value();
		if (cycleN == cycleMax + 1) { break; }
		mathmodel_p->modelData.input = contrl_value;
		mathmodel_p->update();
		cout<<"<----model output=" << mathmodel_p->modelData.output <<"\n";
		mathmodel_p->send_modelOutput(signalID);
		signalID++;
		if (modeling_time != 0)
		{
			cycleN = signalID;
		}
		usleep(microsec); //ожидание заданного интервала
	}
	cout << "- Cycle ended\n";
	mathmodel_p->stop_waiting_newsignal();
	pthread_exit(0);
}

void showHelp(){
	cout << "HELP: Application arguments:\n";	
	cout << "\t-i[in_port]\t\t:input signal TCP port; \n";	
	cout << "\t-o[out_port]\t\t:output signal TCP port; \n";	
	cout << "\t--Tm[time_const]\t:time constant aperiodic link; \n";	
	cout << "\t--Km[gain]\t\t:gain aperiodic link; \n";	
	cout << "\t-t[period]\t\t:iteration period (sampling time) in sec; \n";	
	cout << "\tWithout arguments - starting with default settings; \n";
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
	if (argc == 2 && ((string(argv[1]) == "--help") || (string(argv[1]) == "-h")))
	{
		showHelp();
		return 0;
	}
	else if (argv_parseOK(argc, argv, &mathmodel))
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








