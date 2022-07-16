#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
using namespace std;

struct signal_t{
	char* name;
	int id;
	char* value;
};

signal_t* create_signal(char* s_Name, int s_Id, char* s_value){
	signal_t* s_Message = (signal_t*) malloc(sizeof(signal_t));
	
	const size_t nameLength = strlen(s_Name) + 1;
	s_Message->name = (char*) malloc(sizeof(nameLength));
	strcpy(s_Message->name, s_Name);
	
	s_Message->id = s_Id;
	
	const size_t valueLength = strlen(s_value) + 1;
	s_Message->value = (char*) malloc(sizeof(valueLength));
	strcpy(s_Message->value, s_value);
	
	return s_Message;
}

void get_signalMessage(char* message, signal_t* signal_p){	
	sprintf(message, "%s,%i,%s;", signal_p->name, signal_p->id, signal_p->value);
}
void create_signalMessage(char* message, char* s_Name, int s_Id, float f_value){
	sprintf(message, "%s,%i,%f;", s_Name, s_Id, f_value);
}

void parseName(char* name, char* message){
	int splitter_0;
	const size_t messLength = strlen(message) + 1;
	int i=0;
	for (i = 0; i < messLength; i++)
	{
		if (message[i] == ',')
		{
			splitter_0 = i;
			break;
		}
	}
	for (i = 0; i < splitter_0; i++)
	{
		name[i] = message[i];
	}
	name[splitter_0] = '\0';
}

void parseID(char* id, char* message){
	int splitter_0;
	int splitter_1;
	const size_t messLength = strlen(message) + 1;
	int i=0;
	for (i = 0; i < messLength; i++)
	{
		if (message[i] == ',')
		{
			splitter_0 = i;
			break;
		}
	}
	for (i = splitter_0 + 1; i < messLength; i++)
	{
		if (message[i] == ',')
		{
			splitter_1 = i;
			break;
		}
	}
	for (i = 0; i < splitter_1 - splitter_0 - 1; i++)
	{
		id[i] = message[i + splitter_0 + 1];
	}
	id[splitter_1 - splitter_0] = '\0';
}

void parseValue(char* value, char* message){
	int splitter_0;
	int splitter_1;
	int splitter_2;
	const size_t messLength = strlen(message) + 1;
	int i=0;
	for (i = 0; i < messLength; i++)
	{
		if (message[i] == ',')
		{
			splitter_0 = i;
			break;
		}
	}
	for (i = splitter_0 + 1; i < messLength; i++)
	{
		if (message[i] == ',')
		{
			splitter_1 = i;
			break;
		}
	}
	for (i = splitter_1 + 1; i < messLength; i++)
	{
		if (message[i] == ';')
		{
			splitter_2 = i;
			break;
		}
	}
	for (i = 0; i < splitter_2 - splitter_1 - 1; i++)
	{
		value[i] = message[i + splitter_1 + 1];
	}
	value[splitter_2 - splitter_1] = '\0';
}










