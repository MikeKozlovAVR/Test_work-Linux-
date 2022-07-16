#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "controlbody.cpp"
using namespace std;

struct pullsignal_data_t{
	Control* control_p;
};

//Описание потока с методом update
void *thread_Update_f(void *arg){
	pullsignal_data_t* usdata = (pullsignal_data_t*)arg;
	Control* control_p = usdata->control_p;
	int signalId = 0;
	while (1)
	{
		control_p->waiting_newsignal();				// ожидание сигнала от семафора
		float model_value = control_p->pop_modelsignal_value();

		signalId = control_p->receivedID;
		control_p->controlData.input = model_value;
		control_p->update();
		cout<<"<----control output=" << control_p->controlData.output<<"\n";
		control_p->send_loggerResults(signalId);		//отправка результатов итерации в логгер
		control_p->send_controlOutput(signalId);		//отправка control_out в control_system		
		control_p->controlData.output_prev = control_p->controlData.output;
	}
	control_p->stop_waiting_newsignal();
	pthread_exit(0);
}

void showHelp(){
	cout << "HELP: Application arguments:\n";	
	cout << "\t-i[in_port]\t\t:input signal TCP port; \n";	
	cout << "\t-o[out_port]\t\t:output signal TCP port; \n";
	cout << "\t-l[log_port]\t\t:logger TCP port; \n";	
	cout << "\t--ref[reference_value]\t:input signal value (reference); \n";	
	cout << "\t--kp[prop]\t\t:Kp PI-controller prop. coefficient; \n";	
	cout << "\t--ki[integ]\t\t:Ki PI-controller integr. coefficient; \n";
	cout << "\t-t[period]\t\t:iteration period (sampling time) in sec; \n";
	cout << "\tWithout arguments - starting with default settings; \n";
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
	if (argc == 2 && ((string(argv[1]) == "--help") || (string(argv[1]) == "-h")))
	{
		showHelp();
		return 0;
	}
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
		control.showParameters();
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










