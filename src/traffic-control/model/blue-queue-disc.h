/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Vivek Jain <jain.vivek.anand@gmail.com>
 *          Sandeep Singh <hisandeepsingh@hotmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

/*
//TODO
 * PORT NOTE: This code was ported from ns-2.36rc1 (queue/??).
 * Most of the comments are also ported from the same.
 */

#ifndef BLUE_QUEUE_DISC_H
#define BLUE_QUEUE_DISC_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/timer.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

#define BURST_RESET_TIMEOUT 1.5

namespace ns3 {

class TraceContainer;
class UniformRandomVariable;

class BlueQueueDisc : public QueueDisc
{
public:
  BlueQueueDisc ();
  ~BlueQueueDisc ();

private:
        Queue::QueueMode m_mode;                //!< Mode (bytes or packets)
        uint32_t m_queueLimit;                  //!< Queue limit in bytes / packets
        Time m_qDelay;                          //!< Current value of queue delay
        Ptr<UniformRandomVariable> m_uv         //!< Rng stream
        int drop_front;                         // drop-from-front (rather than from tail)
	int bytes;                              //??
	int dummy;                              //??
	int setbit;                             // Whether to Use ECN (Cannot use this because ns-3 doesn't have support for ECN)
	int mean_pktsize;                       // Average Packet Size
	double decrement;                       // marking probability decrement value
	double increment;                       // marking probability increment value
	double iholdtime;                       // last time at which pmark incremented 
	double dholdtime;                       // last time at which pmark decremented
	int dalgorithm;                         // which decrement algo to use (refer to ns-2 code) (default is additive decrease)
	int ialgorithm;                         // which increment algo to use (refer to ns-2 code) (default is additive increase)
	double bandwidth;                       //

	int idle;                               //??
	double idletime;                        //??
	double ptc;                             //??
	double ifreezetime;                     // Time interval during which pmark cannot be increased
	double dfreezetime;                     // Time interval during which pmark cannot be decreased
	double pmark;                           // Marking Probability
        
        EventId event;                          // It is use to decide which event is triggered (Link is idle or queue bursting) 

protected:
         
        
	bool DoEnqueue(Ptr<QueueDiscItem> item);
	Ptr<QueueDiscItem> DoDequeue(void);
	
        void InitializeParams (void);
	
	void DoReset();
	void plot();
	void plot1(int qlen);
	void pmark_plot(int method);

        bool DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize);                
	void IncrementPmark(int how);           // how is used for specifing increment type  // I think it's not necessary to have 'how' as this is not specified in the paper
	void DecrementPmark(int how);           // how is used for specifing decrement type  // I think it's not necessary to have 'how' as this is not specified in the paper

};

};   // namespace ns3

#endif

