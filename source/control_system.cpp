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
#include "controlbody.cpp"
using namespace std;

struct pullsignal_data_t{
	Control* control_p;
};

//Описание потока с методом update
void *thread_Update_f(void *arg){
	pullsignal_data_t* usdata = (pullsignal_data_t*)arg;
	Control* control_p = usdata->control_p;
	Queue* signals_p = control_p->signals_p;
	TCP_client* tcp_client_ctrl_p = control_p->tcp_client_ctrl_p;
	TCP_client* tcp_client_log_p = control_p->tcp_client_log_p;
	sem_t* sem_recvsignal_p = control_p->sem_recvsignal_p;
	int connSockedID_ctrl = control_p->tcp_sockedID_ctrl;
	int connSockedID_log = control_p->tcp_sockedID_log;
	char sendVal[256];
	float math_in = 0;
	int signalId = 0;
	while (1)
	{
		sem_wait(sem_recvsignal_p);				// ожидание сигнала от семафора
		//if ((signals_p->get_size()) > 0)
		//{
			signal_t* inSignalMess;
			inSignalMess = (signal_t*)signals_p->pop();
			if (inSignalMess != NULL)
			{
				signalId = inSignalMess->id;
				cout<<"->signal["<< signals_p->get_size() <<"]: "<< inSignalMess->name << " [ID:"<< inSignalMess->id << "] "<< inSignalMess->value <<"\n";
				
				control_p->controlData.input = atof(inSignalMess->value);
				control_p->update();
				cout<<"<----control output=" << control_p->controlData.output<<"\n";
				//отправка результатов итерации в логгер
				create_signalMessage(sendVal, "math_in", signalId, math_in);
				tcp_client_log_p->writeData(connSockedID_log, sendVal);
				while (!tcp_client_log_p->readData(connSockedID_log)){} //ожидание ответа (подтвержд.приема)
				sendVal[0] = 0;
				create_signalMessage(sendVal, "math_out", signalId, control_p->controlData.input);
				tcp_client_log_p->writeData(connSockedID_log, sendVal);
				while (!tcp_client_log_p->readData(connSockedID_log)){} //ожидание ответа (подтвержд.приема)
				sendVal[0] = 0;
				create_signalMessage(sendVal, "control_in", signalId, control_p->controlData.ref_value);
				tcp_client_log_p->writeData(connSockedID_log, sendVal);
				while (!tcp_client_log_p->readData(connSockedID_log)){} //ожидание ответа (подтвержд.приема)
				sendVal[0] = 0;
				create_signalMessage(sendVal, "control_fb", signalId, control_p->controlData.input);
				tcp_client_log_p->writeData(connSockedID_log, sendVal);
				while (!tcp_client_log_p->readData(connSockedID_log)){} //ожидание ответа (подтвержд.приема)
				sendVal[0] = 0;
				create_signalMessage(sendVal, "control_out", signalId, control_p->controlData.output);
				tcp_client_log_p->writeData(connSockedID_log, sendVal);
				while (!tcp_client_log_p->readData(connSockedID_log)){} //ожидание ответа (подтвержд.приема)
				sendVal[0] = 0;
				//отправка control_out в control_system
				create_signalMessage(sendVal, "control_out", signalId, control_p->controlData.output);
				tcp_client_ctrl_p->writeData(connSockedID_ctrl, sendVal);
				
				math_in = control_p->controlData.output;
				free(inSignalMess);
			}
			else{ cout << "Error: in a queue null element\n"; }
		//}
	}
	pthread_exit(0);
}

bool argv_parseOK(int argc, char *argv[], Control* control){
	bool isOK = 0;
	int i=1;
	bool port_in_en = 0;
	bool port_out_en = 0;
	bool port_log_en = 0;
	bool ref_value_en = 0;
	bool kp_en = 0;
	bool ki_en = 0;
	bool t_discr_en = 0;
	if (argc == 15)
	{
		for (i=1; i< argc; i++)
		{
			if(string(argv[i])=="-i"){
				control->controlData.port_in = atoi(argv[i + 1]);
				port_in_en = 1;
			}
			if(string(argv[i])=="-o"){
				control->controlData.port_out = atoi(argv[i + 1]);
				port_out_en = 1;
			}
			if(string(argv[i])=="-l"){
				control->controlData.port_log = atoi(argv[i + 1]);
				port_log_en = 1;
			}
			if(string(argv[i])=="--ref"){
				control->controlData.ref_value = atof(argv[i + 1]);
				ref_value_en = 1;
			}
			if(string(argv[i])=="--kp"){
				control->controlData.kp = atof(argv[i + 1]);
				kp_en = 1;
			}
			if(string(argv[i])=="--ki"){
				control->controlData.ki = atof(argv[i + 1]);
				ki_en = 1;
			}
			if(string(argv[i])=="-t"){
				control->controlData.t_discr = atof(argv[i + 1]);
				t_discr_en = 1;
			}
		}
		isOK = port_in_en & port_out_en & port_log_en & ref_value_en & kp_en & ki_en & t_discr_en;
	}
	else { return 0; }
	return isOK;
}


int main(int argc, char *argv[]){

	Control control;
	
	if (argv_parseOK(argc, argv, &control))
	{
		control.showParameters();
	}
	else
	{
		cout << "- error: Wrong arguments!\n";
		//return 0;
		
		cout << "- Will use defaults:\n";				//default для тестов
		control.controlData.port_in = 8005;
		control.controlData.port_out = 8004;
		control.controlData.port_log = 8006;
		control.controlData.ref_value = 10;
		control.controlData.kp = 1;
		control.controlData.ki = 0.1;
		control.controlData.t_discr = 0.01;
	}
	control.init();
	
	//инициализация потока с методом update - потребителя (обработчика) очереди сигналов
	pullsignal_data_t updatesignal_arg;
	updatesignal_arg.control_p = &control;
	pullsignal_data_t* updatesignal_arg_p;
	updatesignal_arg_p = new pullsignal_data_t;
	updatesignal_arg_p = &updatesignal_arg;
	pthread_t thread_update;            	//идентификатор потока с методом update - потребителя
	pthread_create(&thread_update, NULL, thread_Update_f, (void *)updatesignal_arg_p);
	
	pthread_join(thread_update, NULL);

	return 0;
}










