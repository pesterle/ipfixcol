/**
 * \file Kafka.cpp
 * \author Lukas Hutak <xhutak01@stud.fit.vutbr.cz>
 * \brief Kafka output (source file)
 *
 * Copyright (C) 2015 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in
 *	the documentation and/or other materials provided with the
 *	distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *	may be used to endorse or promote products derived from this
 *	software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include "Kafka.h"
#include <stdexcept>
#include <string>
#include <vector>

#include <cstring>
#include <cerrno>
#include <cstdio>
#include <sstream>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define DEF_WINDOW_SIZE (300)
#define DEF_WINDOW_ALIGN (true)

static const char *msg_module = "json_storage(kafka)";

/**
 * \brief Class constructor
 *
 * Parse output configuration and create an output file
 * \param config[in] XML configuration
 */
Kafka::Kafka(const pugi::xpath_node &config)
{
	std::string errstr;

	MSG_INFO(msg_module, "initializing");

	gconf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
	tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
	
	for ( pugi::xml_node prop = config.node().first_child(); prop; prop = prop.next_sibling() )
	{
		std::string name = prop.name();
		if ( name.compare("Topics") == 0 )
		{
			topic = config.node().child_value(prop.name()); 
			MSG_INFO(msg_module, "%s = %s", name.c_str(), topic.c_str());
		}
		else if ( name.compare("type") == 0 )
		{
			// ignore this
		}
		else
		{
			std::string value = config.node().child_value(name.c_str());
			MSG_INFO(msg_module, "%s = %s", name.c_str(), value.c_str());
			if ( gconf->set(name, value, errstr) != RdKafka::Conf::CONF_OK )
				MSG_INFO(msg_module, "errstr = %s", errstr.c_str());
		}
	}

	std::list<std::string> * conf = gconf->dump();
	for ( std::list<std::string>::iterator i = conf->begin(); i != conf->end(); i++ )
	{
		std::string s = *i;
		MSG_INFO(msg_module, "%s", s.c_str());
	}
	
	producer = RdKafka::Producer::create(gconf, errstr);	
	qcount = 0;

	MSG_INFO(msg_module, "initialized");
}

/**
 * \brief Class destructor
 *
 * Close all opened files
 */
Kafka::~Kafka()
{

}


/**
 * \brief Store a record to a file
 * \param[in] record JSON record
 */
void Kafka::ProcessDataRecord(const std::string &record)
{
	// Store the record
	//
	std::stringstream sbuf;
	sbuf << record;

	RdKafka::ErrorCode resp = producer->produce(GetTopic(topic), RdKafka::Topic::PARTITION_UA, RdKafka::Producer::RK_MSG_COPY, (void*)(sbuf.str().data()), sbuf.str().size(), NULL, NULL);
	if ( resp != RdKafka::ERR_NO_ERROR)
	{
		MSG_ERROR(msg_module, "Failed to push record to Kafka %s", RdKafka::err2str(resp).c_str());
	}
	else
	{
		//MSG_INFO(msg_module, "Record pushed to Kafka");
		qcount++;
		if (qcount > 1000 )
		{
			producer->flush(30000);
			qcount = 0;
		}
	}
}

RdKafka::Topic * Kafka::CreateTopic(std::string)
{
	std::string errstr;
	
	RdKafka::Topic *t = RdKafka::Topic::create(producer, topic, tconf, errstr);
	topics.insert(std::pair<std::string, RdKafka::Topic*>(topic, t));
	return t;
}

RdKafka::Topic * Kafka::GetTopic(std::string)
{
	std::map<std::string, RdKafka::Topic *>::iterator i = topics.find(topic);
	if ( i != topics.end() )
	{   
		RdKafka::Topic * t = i->second;
		return t;
	}   
	else
		return CreateTopic(topic);
}

