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
using namespace std;
//Описание используемых структур
struct log_data_t{
	char* math_in;
	char* math_out;
	char* control_in;
	char* control_fb;
	char* control_out;
};

struct data_t{
	char* path;							//log-file path
	int port_in;						//in TCP port
	log_data_t log_data;				//логгируемая информация
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
//Описание класса Logger
class Logger : Base{
public:
	data_t loggerData;      //Данные логгера
private:
	TCP_server* tcp_server_p;
	Queue* signals_p;
	sem_t* sem_recvsignal_p;
	ofstream* fout_p;
public:
	virtual void init();
	virtual void update();
	void showParameters();
	void pop_and_parse_signal();
	void waiting_newsignal();
	void stop_waiting_newsignal();
	int get_signalsCount();
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
			int count = rsdata->signal_queue->push(signalMessage);  //кладем принятый сигнал в очередь
			rsdata->tcp_server_p->writeData(clientN, rsdata->tcp_server_p->readBuff);
			rsdata->tcp_server_p->clearReadBuff();
			sem_post(rsdata->sem_recvsignal_p);
		}
	}
}

void Logger:: init(){
	//Запись заголовка в csv-файл
	ofstream fout(this->loggerData.path);
	if(fout.is_open())
	{
		fout << "math_in" << "\t" << "math_out" << "\t" << "control_in" << "\t" << "control_fb" << "\t" << "control_out" << "\n";
		fout.close();
	}
	else
	{
		cout << "!file path is wrong\n";
	}
	
	sem_t sem_recvsignal; 
	sem_init(&sem_recvsignal, 0, 0);
	this->sem_recvsignal_p = &sem_recvsignal;
	
	//инициализация потокобезопасной очереди
	pthread_mutex_t queue_mutex;
	pthread_mutex_init(&queue_mutex, NULL);
	Queue signals(&queue_mutex);
	this->signals_p = &signals;

	//инициализация сервера принимающего сигнал от control_system
	TCP_server tcp_server(this->loggerData.port_in, 1);
	this->tcp_server_p = &tcp_server;
	cout << "status: logger (Server) run\n";
	
	//ожидание подключения control_system (клиента)
	cout << "status: c.s. client connecting..\n";
	while (1)
	{
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
			pthread_t thread_recvsignal;      //идентификатор потока приема сообщений от подключенных хостов
			pthread_create(&thread_recvsignal, NULL, thread_recvSignal_f, (void *)recvsignal_arg_p);
			break;
		}
	}
	cout << "status: Ready\n";
}

void Logger::update(){
	ofstream fout(string(this->loggerData.path), ios::app);
	if(fout.is_open())
	{
		char* math_in = this->loggerData.log_data.math_in;
		char* math_out = this->loggerData.log_data.math_out;
		char* control_in = this->loggerData.log_data.control_in;
		char* control_fb = this->loggerData.log_data.control_fb;
		char* control_out = this->loggerData.log_data.control_out;
		fout << math_in << "\t" << math_out << "\t" << control_in << "\t" << control_fb << "\t" << control_out << "\n";
	
		fout.close();
	}
}

void Logger::showParameters(){
	cout << "port_in:"<< this->loggerData.port_in <<"\n";
	cout << "path:"<< this->loggerData.path <<"\n";
}

int Logger::get_signalsCount(){
	return this->signals_p->get_size();
}

void Logger::pop_and_parse_signal(){
	signal_t* inSignalMess;
	inSignalMess = (signal_t*)this->signals_p->pop();
	if (inSignalMess != NULL)
	{
		if (string(inSignalMess->name) == "math_in"){
			this->loggerData.log_data.math_in = inSignalMess->value;	
		}
		if (string(inSignalMess->name) == "math_out"){
			this->loggerData.log_data.math_out = inSignalMess->value;
		}
		if (string(inSignalMess->name) == "control_in"){
			this->loggerData.log_data.control_in = inSignalMess->value;
		}
		if (string(inSignalMess->name) == "control_fb"){
			this->loggerData.log_data.control_fb = inSignalMess->value;
		}
		if (string(inSignalMess->name) == "control_out"){
			this->loggerData.log_data.control_out = inSignalMess->value;
		}
		cout<<"->signal["<< this->signals_p->get_size() <<"]: "<< inSignalMess->name << " [ID:"<< inSignalMess->id << "] "<< inSignalMess->value <<"\n";
					
		free(inSignalMess);
	}
	else{ cout << "Error: in a queue null element\n"; }
}

void Logger::waiting_newsignal()
{
	sem_wait(this->sem_recvsignal_p);				// ожидание сигнала от семафора
}
void Logger::stop_waiting_newsignal()
{
	sem_destroy(this->sem_recvsignal_p);    // уничтожение семафора
}


