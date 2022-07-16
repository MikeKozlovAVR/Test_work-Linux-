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
	float km;					//gain
	float tm;					//time const
	float input;					//input value
	float output;					//output value
	float output_prev;
	float t_discr;					//period
	int port_in;					//in TCP port
	int port_out;					//out TCP port
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
//Описание класса Mathmodel
class Mathmodel : Base{
public:
	data_t modelData;             //Данные об объекте управления
private:
	TCP_server* tcp_server_p;
	TCP_client* tcp_client_p;
	int tcp_sockedID;
	Queue* signals_p;
	sem_t* sem_recvsignal_p;
public:
	virtual void init();
	virtual void update();
	void showParameters();
	void send_modelOutput(int s_Id);
	void send_modelOutputPrev(int s_Id);
	float pop_controlsignal_value();
	void waiting_newsignal();
	void stop_waiting_newsignal();
private:
	void get_DefaultState();
	float get_objectOutput(float input);
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
//Методы класса Mathmodel
void Mathmodel:: init(){
	
	sem_t sem_recvsignal; 
	sem_init(&sem_recvsignal, 0, 0);
	this->sem_recvsignal_p = &sem_recvsignal;
	
	//инициализация потокобезопасной очереди
	pthread_mutex_t queue_mutex;
	pthread_mutex_init(&queue_mutex, NULL);
	Queue signals(&queue_mutex);
	this->signals_p = &signals;
		
	//инициализация сервера принимающего сигнал от control_system
	TCP_server tcp_server(this->modelData.port_in, 1);
	this->tcp_server_p = &tcp_server;
	cout << "status: math_model (Server) run\n";
	
	//инициализация клиента передающего сигнал в control_system
	TCP_client tcp_client("127.0.0.1", this->modelData.port_out);
	this->tcp_client_p = &tcp_client;
	cout << "status: math_model (Client) run\n";
	cout << "status: c.s. server connecting..\n";
	bool connectStatus;
	int connectedSockedID;
	while(!(connectStatus = tcp_client.connectToServer())){}
	if (connectStatus)
	{
		cout << "status: c.s. server connected\n";
		connectedSockedID = tcp_client.sockedID;
		this->tcp_sockedID = connectedSockedID;
	}
	
	//ожидание подключения control_system (клиента)
	cout << "status: c.s. client connecting..\n";
	int clientN = tcp_server.clientIsConnected(); //ожидание подключения от control_system
	if (clientN != -1)
	{
		cout << "status: c.s. client connected\n";
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

float Mathmodel:: get_objectOutput(float input){
	float output = 0;
	float km = this->modelData.km;
	float tm = this->modelData.tm;
	float output_prev = this->modelData.output_prev;
	float t_discr = this->modelData.t_discr;
	float a = 1/(1+t_discr/tm);			//преобразованная формула апериодического звена
	float b = km/(1 + tm/t_discr);
	output = a*output_prev + b*input;
	this->modelData.output_prev = output;
	return output;
}

void Mathmodel:: update(){
	this->modelData.output = get_objectOutput(this->modelData.input);
}

void Mathmodel::get_DefaultState(){
	this->modelData.output_prev = 0;
	this->modelData.output_prev = get_objectOutput(0);
}

void Mathmodel::showParameters(){
	cout << "port_in:"<< this->modelData.port_in <<"\n";
	cout << "port_out:"<< this->modelData.port_out <<"\n";
	cout << "Tm:"<< this->modelData.tm <<"\n";
	cout << "Km:"<< this->modelData.km <<"\n";
	cout << "period:"<< this->modelData.t_discr <<"\n";
}

void Mathmodel::send_modelOutput(int s_Id){
	char sendVal[256];
	create_signalMessage(sendVal, "model_output", s_Id, this->modelData.output);
	this->tcp_client_p->writeData(this->tcp_sockedID, sendVal);
}
void Mathmodel::send_modelOutputPrev(int s_Id){
	char sendVal[256];
	create_signalMessage(sendVal, "model_output", s_Id, this->modelData.output_prev);
	this->tcp_client_p->writeData(this->tcp_sockedID, sendVal);
}
float Mathmodel::pop_controlsignal_value(){	
	signal_t* inSignalMess;
	float controlsignalval;
	inSignalMess = (signal_t*)this->signals_p->pop();
	if (inSignalMess != NULL)
	{
		cout<<"->signal["<< this->signals_p->get_size() <<"]: "<< inSignalMess->name << " [ID:"<< inSignalMess->id << "] "<< inSignalMess->value <<"\n";
		controlsignalval = atof(inSignalMess->value);
		free(inSignalMess);
	}
	else
	{
		cout << "Error: in a queue null element\n";
		return NULL;
	}
	return controlsignalval;
}

void Mathmodel::waiting_newsignal()
{
	sem_wait(this->sem_recvsignal_p);				// ожидание сигнала от семафора
}
void Mathmodel::stop_waiting_newsignal()
{
	sem_destroy(this->sem_recvsignal_p);    // уничтожение семафора
}


