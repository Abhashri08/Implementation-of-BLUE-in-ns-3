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

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "blue-queue-disc.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BlueQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (BlueQueueDisc);

TypeId PieQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BlueQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<BlueQueueDisc> ()
    .AddAttribute ("Mode",
                   "Determines unit for QueueLimit",
                   EnumValue (Queue::QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&BlueQueueDisc::SetMode),
                   MakeEnumChecker (Queue::QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    Queue::QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&BlueQueueDisc::mean_pktsize),
                   MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

BlueQueueDisc::BlueQueueDisc ()
  : QueueDisc ()
{
  NS_LOG_FUNCTION (this)
  //event = Simulator::Schedule (m_sUpdate, &PieQueueDisc::CalculateP, this);   //
}

BlueQueueDisc::~BlueQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}


void
BlueQueueDisc::SetMode (Queue::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

Queue::QueueMode
BlueQueueDisc::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

void
BlueQueueDisc::SetQueueLimit (uint32_t lim)
{
  NS_LOG_FUNCTION (this << lim);
  m_queueLimit = lim;
}

uint32_t
BlueQueueDisc::GetQueueSize (void)
{
  NS_LOG_FUNCTION (this);
  if (GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      return GetInternalQueue (0)->GetNBytes ();
    }
  else if (GetMode () == Queue::QUEUE_MODE_PACKETS)
    {
      return GetInternalQueue (0)->GetNPackets ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown Blue mode.");
    }
}

Time
BlueQueueDisc::GetQueueDelay (void)
{
  NS_LOG_FUNCTION (this);
  return m_qDelay;
}

bool
BlueQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  uint32_t nQueued = GetQueueSize ();

  if ((GetMode () == Queue::QUEUE_MODE_PACKETS && nQueued >= m_queueLimit)
      || (GetMode () == Queue::QUEUE_MODE_BYTES && nQueued + item->GetPacketSize () > m_queueLimit))
    {
      // Drops due to queue limit: reactive
      Drop (item);
      return false;
    }
  else if (DropEarly (item, nQueued))
    {
      // Early probability drop: proactive
      Drop (item);
      return false;
    }

  // No drop
  bool retval = GetInternalQueue (0)->Enqueue (item);

  /** 
        FP1: Query
        When to call increment pmark
  */

  // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
  // because QueueDisc::AddInternalQueue sets the drop callback

  NS_LOG_LOGIC ("\t bytesInQueue  " << GetInternalQueue (0)->GetNBytes ());
  NS_LOG_LOGIC ("\t packetsInQueue  " << GetInternalQueue (0)->GetNPackets ());

  return retval;
}

void
BlueQueueDisc::InitializeParams (void)
{
  // Initially queue is empty so variables are initialize to zero except m_dqCount

}

bool BlueQueueDisc::DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize)
{
  NS_LOG_FUNCTION (this << item << qSize);
  if (m_burstAllowance.GetSeconds () > 0)
    {
      // If there is still burst_allowance left, skip random early drop.
      return false;
    }

  if (m_burstState == NO_BURST)
    {
      m_burstState = IN_BURST_PROTECTING;
      m_burstAllowance = m_maxBurst;
    }

  double p = m_dropProb;

  uint32_t packetSize = item->GetPacketSize ();

  if (GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      p = p * packetSize / m_meanPktSize;
    }
  bool earlyDrop = true;
  double u =  m_uv->GetValue ();

  if ((m_qDelayOld.GetSeconds () < (0.5 * m_qDelayRef.GetSeconds ())) && (m_dropProb < 0.2))
    {
      return false;
    }
  else if (GetMode () == Queue::QUEUE_MODE_BYTES && qSize <= 2 * m_meanPktSize)
    {
      return false;
    }
  else if (GetMode () == Queue::QUEUE_MODE_PACKETS && qSize <= 2)
    {
      return false;
    }

  if (u > p)
    {
      earlyDrop = false;
    }
  if (!earlyDrop)
    {
      return false;
    }

  return true;
}

void BlueQueueDisc::IncrementPmark (int how)
{
  NS_LOG_FUNCTION (this);
  //TODO Check how to get current time 
  double now ;//= Scheduler::instance().clock();

  if (now - ifreezetime > iholdtime) {
    ifreezetime = now;
    switch (ialgorithm) {
      case 0:
        switch (how) {
          case 0:
            pmark += increment;
            break;
			    case 1:
			    default:
				    break;
		    }
		    break;
      
      case 2:
        switch (how) {
			    case 0:
            pmark = 2*pmark + increment;
				    break;         
    			case 1:
		    	default:
				    break;
		    }
        //TODO break missing
    	default:
    	case 1:
    		switch (how) {
		    	case 0:
            pmark += increment;
    				break;
          case 1:
          default:
            pmark += increment/10;
    				break;
		    }
      }
      if (pmark > 1.0) 
        pmark = 1.00;
    }
    //TODO What is significance of below linw and how can we update it for blue
    //m_rtrsEvent = Simulator::Schedule (m_tUpdate, &BlueQueueDisc::IncrementPmark, this);
}

void BlueQueueDisc::DecrementPmark (int how)
{
  NS_LOG_FUNCTION (this);
  //TODO Check how to get current time 
  double now ;//= Scheduler::instance().clock();
	if (now - dfreezetime > dholdtime) {
		dfreezetime = now;
    	switch (dalgorithm) {
    	  case 0:
	      case 2:
		      switch (how) {
			      case 0:
        			pmark -= decrement;
				      break;
			      case 1:
			      default:
				      break;
		      } 
		      break;
    	
        default:
    	  case 1:
		      switch (how) {
			      case 0:
        		  pmark -= decrement;
				      break;
			      case 1:
			      default:
        			pmark -= decrement/10;
				      break;
		      }
    	  }
    	  if (pmark < 0)
		      pmark = 0.0;
      }
    //TODO What is significance of below linw and how can we update it for blue
    //m_rtrsEvent = Simulator::Schedule (m_tUpdate, &BlueQueueDisc::DecrementPmark, this);
}
  
  
Ptr<QueueDiscItem>
PieQueueDisc::DoDequeue ()
{
  NS_LOG_FUNCTION (this);

  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());
  double now = Simulator::Now ().GetSeconds ();
  uint32_t pktSize = item->GetPacketSize ();

  // if not in a measurement cycle and the queue has built up to dq_threshold,
  // start the measurement cycle

  if ( (GetInternalQueue (0)->GetNBytes () >= m_dqThreshold) && (!m_inMeasurement) )
    {
      m_dqStart = now;
      m_dqCount = 0;
      m_inMeasurement = true;
    }

  if (m_inMeasurement)
    {
      m_dqCount += pktSize;

      // done with a measurement cycle
      if (m_dqCount >= m_dqThreshold)
        {

          double tmp = now - m_dqStart;

          if (tmp > 0)
            {
              if (m_avgDqRate == 0)
                {
                  m_avgDqRate = m_dqCount / tmp;
                }
              else
                {
                  m_avgDqRate = (0.5 * m_avgDqRate) + (0.5 * (m_dqCount / tmp));
                }
            }

          // restart a measurement cycle if there is enough data
          if (GetInternalQueue (0)->GetNBytes () > m_dqThreshold)
            {
              m_dqStart = now;
              m_dqCount = 0;
              m_inMeasurement = true;
            }
          else
            {
              m_dqCount = 0;
              m_inMeasurement = false;
            }
        }
    }

  return item;
}

Ptr<const QueueDiscItem>
PieQueueDisc::DoPeek () const
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
PieQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("PieQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("PieQueueDisc cannot have packet filters");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create a DropTail queue
      Ptr<Queue> queue = CreateObjectWithAttributes<DropTailQueue> ("Mode", EnumValue (m_mode));
      if (m_mode == Queue::QUEUE_MODE_PACKETS)
        {
          queue->SetMaxPackets (m_queueLimit);
        }
      else
        {
          queue->SetMaxBytes (m_queueLimit);
        }
      AddInternalQueue (queue);
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("PieQueueDisc needs 1 internal queue");
      return false;
    }

  if (GetInternalQueue (0)->GetMode () != m_mode)
    {
      NS_LOG_ERROR ("The mode of the provided queue does not match the mode set on the PieQueueDisc");
      return false;
    }

  if ((m_mode ==  Queue::QUEUE_MODE_PACKETS && GetInternalQueue (0)->GetMaxPackets () < m_queueLimit)
      || (m_mode ==  Queue::QUEUE_MODE_BYTES && GetInternalQueue (0)->GetMaxBytes () < m_queueLimit))
    {
      NS_LOG_ERROR ("The size of the internal queue is less than the queue disc limit");
      return false;
    }

  return true;
}

} //namespace ns3
