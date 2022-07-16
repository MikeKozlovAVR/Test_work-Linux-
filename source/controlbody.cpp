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
using namespace std;
//Описание используемых структур
struct data_t{
	float kp;					//prop
	float ki;					//integ
	float ref_value;				//reference value
	float input;					//input value
	float output;					//output value
	float integr;					//e_integral_value
	float t_discr;					//period
	float output_prev;		// control_out in a prev. iteration
	int port_in;					//in TCP port
	int port_out;					//out TCP port
	int port_log;					//logger TCP port
	
};

struct recvsignal_data_t{
	TCP_server* tcp_server_p;
	int connectID;
	Queue* signal_queue;
	sem_t* sem_recvsignal_p;
};
//Базовый класс
class Base{
public:
	Base(){}
	virtual void init(){}
	virtual void update(){}
};
//Описание класса Control
class Control : Base{
public:	
	data_t controlData;              //Данные системы управления
	int receivedID;										//ID принятого сигнала
private:
	TCP_server* tcp_server_p;
	TCP_client* tcp_client_ctrl_p;
	TCP_client* tcp_client_log_p;
	int tcp_sockedID_ctrl;
	int tcp_sockedID_log;
	Queue* signals_p;
	sem_t* sem_recvsignal_p;
public:
	virtual void init();
	virtual void update();
	void showParameters();
	void send_controlOutput(int s_Id);
	void send_loggerResults(int s_Id);
	float pop_modelsignal_value();
	void waiting_newsignal();
	void stop_waiting_newsignal();
private:
	void get_DefaultState();
	float get_controlOutput(float input);
};

//Описание потока принимающего сигналы и складывающего в очередь
void *thread_recvSignal_f(void *arg){
	recvsignal_data_t* rsdata = (recvsignal_data_t*)arg;
	int clientN = rsdata->connectID;
	char name[256];
	char id[256];
	char value[256];
	while(1)
	{
		if (rsdata->tcp_server_p->readData(clientN))
		{
			char* readedVal = rsdata->tcp_server_p->readBuff;
			parseName(name, readedVal);
			parseID(id, readedVal);
			parseValue(value, readedVal);
			signal_t *signalMessage;
			signalMessage = create_signal(name, atoi(id), value);
			int count = rsdata->signal_queue->push(signalMessage);//кладем принятый сигнал в очередь
			rsdata->tcp_server_p->clearReadBuff();
			sem_post(rsdata->sem_recvsignal_p);
		}
	}
	pthread_exit(0);
}

void Control:: init(){

	sem_t sem_recvsignal; 
	sem_init(&sem_recvsignal, 0, 0);
	this->sem_recvsignal_p = &sem_recvsignal;
	
	//инициализация потокобезопасной очереди
	pthread_mutex_t queue_mutex;
	pthread_mutex_init(&queue_mutex, NULL);
	Queue signals(&queue_mutex);
	this->signals_p = &signals;

	//инициализация сервера принимающего сигнал от math_model
	TCP_server tcp_server(this->controlData.port_in, 1);
	this->tcp_server_p = &tcp_server;
	cout << "status: control_system (Server) run\n";

	//инициализация клиента передающего сигнал в math_model
	TCP_client tcp_client_ctrl("127.0.0.1", this->controlData.port_out);
	this->tcp_client_ctrl_p = &tcp_client_ctrl;
	cout << "status: control_system (Client) run\n";
	cout << "status: m.m. server connecting..\n";
	bool connectStatus_ctrl;
	int connectedSockedID_ctrl;
	while(!(connectStatus_ctrl = tcp_client_ctrl.connectToServer())){}
	if (connectStatus_ctrl)
	{
		cout << "status: m.m. server connected\n";
		connectedSockedID_ctrl = tcp_client_ctrl.sockedID;
		this->tcp_sockedID_ctrl = connectedSockedID_ctrl;
	}
	
	//инициализация клиента передающего сигнал в logger
	TCP_client tcp_client_log("127.0.0.1", this->controlData.port_log);
	this->tcp_client_log_p = &tcp_client_log;
	cout << "status: logger (Client) run\n";
	bool connectStatus_log;
	int connectedSockedID_log;
	cout << "status: logger connecting..\n";
	while(!(connectStatus_log = tcp_client_log.connectToServer())){}
	if (connectStatus_log)
	{
		cout << "status: logger server connected\n";
		connectedSockedID_log = tcp_client_log.sockedID;
		this->tcp_sockedID_log = connectedSockedID_log;
	}
	
	//ожидание подключения math_model (клиента)
	cout << "status: m.m. client connecting..\n";
	int clientN = tcp_server.clientIsConnected(); //ожидание подключения от math_model
	if (clientN != -1)
	{
		cout << "status: m.m. client connected\n";
		//инициализация потока поставщика (генератора) очереди сигналов
		recvsignal_data_t recvsignal_arg;
		recvsignal_arg.tcp_server_p = &tcp_server;
		recvsignal_arg.connectID = clientN;
		recvsignal_arg.signal_queue = &signals;
		recvsignal_arg.sem_recvsignal_p = &sem_recvsignal;
		recvsignal_data_t *recvsignal_arg_p;
		recvsignal_arg_p = new recvsignal_data_t;
		recvsignal_arg_p = &recvsignal_arg;
		pthread_t thread_recvsignal;      //идентификатор потока приема от подключенных хостов
		pthread_create(&thread_recvsignal, NULL, thread_recvSignal_f, (void *)recvsignal_arg_p);
	}
	this->get_DefaultState();
	cout << "status: Ready\n";
}

float Control:: get_controlOutput(float input){
	float output = 0;
	float kp = this->controlData.kp;
	float ki = this->controlData.ki;
	float t_discr = this->controlData.t_discr;
	float e = this->controlData.ref_value - input;
	float integr = (this->controlData.integr + e*t_discr);
	output = kp*e + ki*integr;
	this->controlData.integr = integr;
	return output;
}

void Control:: update(){
	this->controlData.output = get_controlOutput(this->controlData.input);
}

void Control:: get_DefaultState(){
	this->controlData.integr = 0;
	this->controlData.output_prev = 0;
}

void Control::showParameters(){
	cout << "port_in:"<< this->controlData.port_in <<"\n";
	cout << "port_out:"<< this->controlData.port_out <<"\n";
	cout << "port_logger:"<< this->controlData.port_log <<"\n";
	cout << "Ref value:"<< this->controlData.ref_value <<"\n";
	cout << "Kp:"<< this->controlData.kp <<"\n";
	cout << "Ki:"<< this->controlData.ki <<"\n";
	cout << "period:"<< this->controlData.t_discr <<"\n";
}

void Control::send_controlOutput(int s_Id){
	char sendVal[256];
	create_signalMessage(sendVal, "control_out", s_Id, this->controlData.output);
	this->tcp_client_ctrl_p->writeData(this->tcp_sockedID_ctrl, sendVal);
}

void Control::send_loggerResults(int s_Id){
	char sendVal[256];
	create_signalMessage(sendVal, "math_in", s_Id, this->controlData.output_prev);
	this->tcp_client_log_p->writeData(this->tcp_sockedID_log, sendVal);
	while (!this->tcp_client_log_p->readData(this->tcp_sockedID_log)){} //ожид. ответа (подтв.приема)
	sendVal[0] = 0;
	create_signalMessage(sendVal, "math_out", s_Id, this->controlData.input);
	this->tcp_client_log_p->writeData(this->tcp_sockedID_log, sendVal);
	while (!this->tcp_client_log_p->readData(this->tcp_sockedID_log)){} //ожид. ответа (подтв.приема)
	sendVal[0] = 0;
	create_signalMessage(sendVal, "control_in", s_Id, this->controlData.ref_value);
	this->tcp_client_log_p->writeData(this->tcp_sockedID_log, sendVal);
	while (!this->tcp_client_log_p->readData(this->tcp_sockedID_log)){} //ожид. ответа (подтв.приема)
	sendVal[0] = 0;
	create_signalMessage(sendVal, "control_fb", s_Id, this->controlData.input);
	this->tcp_client_log_p->writeData(this->tcp_sockedID_log, sendVal);
	while (!this->tcp_client_log_p->readData(this->tcp_sockedID_log)){} //ожид. ответа (подтв.приема)
	sendVal[0] = 0;
	create_signalMessage(sendVal, "control_out", s_Id, this->controlData.output);
	this->tcp_client_log_p->writeData(this->tcp_sockedID_log, sendVal);
	while (!this->tcp_client_log_p->readData(this->tcp_sockedID_log)){} //ожид. ответа(подтв.приема)
	sendVal[0] = 0;
}


float Control::pop_modelsignal_value(){	
	signal_t* inSignalMess;
	float modelsignalval;
	inSignalMess = (signal_t*)signals_p->pop();
	if (inSignalMess != NULL)
	{
		cout<<"->signal["<< signals_p->get_size() <<"]: "<< inSignalMess->name << " [ID:"<< inSignalMess->id << "] "<< inSignalMess->value <<"\n";
		this->receivedID = inSignalMess->id;
		modelsignalval = atof(inSignalMess->value);
		free(inSignalMess);
	}
	else
	{
		cout << "Error: in a queue null element\n";
		return NULL;
	}
	return modelsignalval;
}

void Control::waiting_newsignal()
{
	sem_wait(this->sem_recvsignal_p);				// ожидание сигнала от семафора
}
void Control::stop_waiting_newsignal()
{
	sem_destroy(this->sem_recvsignal_p);    // уничтожение семафора
}







