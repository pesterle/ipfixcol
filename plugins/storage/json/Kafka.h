/**
 * \file Kafka.h
 *
 */

#ifndef KAFKA_H
#define KAFKA_H

#include "json.h"

#include <string>
#include <ctime>
#include <cstdio>
#include <map>

#include <pthread.h>

#include <librdkafka/rdkafkacpp.h>


/**
 * \brief The class for file output interface
 */
class Kafka : public Output {
public:
	// Constructor & destructor
	Kafka(const pugi::xpath_node &config);
	~Kafka();

	// Store a record to the file
	void ProcessDataRecord(const std::string &record);

private:

	std::string topic;
	std::map<std::string, RdKafka::Topic *> topics;
	RdKafka::Topic * CreateTopic(std::string);
	RdKafka::Topic * GetTopic(std::string);
	RdKafka::Producer * producer;

	RdKafka::Conf * gconf;
	RdKafka::Conf * tconf;

	int qcount;
};

#endif // KAFKA_H

