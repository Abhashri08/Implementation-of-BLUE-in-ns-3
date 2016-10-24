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

TypeId BlueQueueDisc::GetTypeId (void)
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
                   MakeUintegerAccessor (&BlueQueueDisc::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   UintegerValue (25),
                   MakeUintegerAccessor (&BlueQueueDisc::SetQueueLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Increment",
                   "Pmark increment value",
                   DoubleValue (0.0025),
                   MakeDoubleAccessor (&BlueQueueDisc::m_increment),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Decrement",
                   "Pmark decrement Value",
                   DoubleValue (0.00025),
                   MakeDoubleAccessor (&BlueQueueDisc::m_decrement),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("IHoldTime",
                   "Value of increment hold time",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&BlueQueueDisc::m_iHoldTime),
                   MakeTimeChecker ())
    .AddAttribute ("DHoldTime",
                   "Value of decrement hold time",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&BlueQueueDisc::m_dHoldTime),
                   MakeTimeChecker ())
    .AddAttribute ("IFreezeTime",
                   "Value of ifreezetime",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&BlueQueueDisc::m_iFreezeTime),
                   MakeTimeChecker ())
    .AddAttribute ("DFreezeTime",
                   "Value of dfreezetime",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&BlueQueueDisc::m_dFreezeTime),
                   MakeTimeChecker ())
    .AddAttribute ("DAlgorithm",
                   "Decrement Algorithm to use",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BlueQueueDisc::m_dAlgorithm),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("IAlgorithm",
                   "Increment Algorithm to use",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BlueQueueDisc::m_iAlgorithm),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("PMark",
                   "Marking Probabilty",
                   DoubleValue (0),
                   MakeDoubleAccessor (&BlueQueueDisc::m_Pmark),
                   MakeDoubleChecker<double> ())
  ;

  return tid;
}

BlueQueueDisc::BlueQueueDisc ()
  : QueueDisc ()
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
}

BlueQueueDisc::~BlueQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

BlueQueueDisc::Stats
BlueQueueDisc::GetStats ()
{
  NS_LOG_FUNCTION (this);
  return m_stats;
}

void
BlueQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_uv = 0;
  QueueDisc::DoDispose ();
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


int64_t
BlueQueueDisc::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
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
      IncrementPmark (0);
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
  bool isEnqueued = GetInternalQueue (0)->Enqueue (item);
  if (isEnqueued)
    {
      int bytes = GetInternalQueue (0)->GetNBytes ();
      int packets = GetInternalQueue (0)->GetNPackets ();
      int qMetric = 0;
      int qLimit = 0;
      switch ( m_bytes )
        {
        case 0:
          qMetric = packets; 
          qLimit = m_queueLimit;
          break;
        default:
        case 1:
          qMetric = bytes;
          qLimit = m_queueLimit * m_meanPktSize;
          break;
        }
      // int half = qLimit / 2;
      // if ( qMetric > qLimit )
      //    IncrementPmark (1);
      // else
      //    DecrementPmark (1);
      if ( qMetric > qLimit )
        {
          Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());
          IncrementPmark (0);
          return false;
        }
    }
  NS_LOG_LOGIC ("\t bytesInQueue  " << GetInternalQueue (0)->GetNBytes ());
  NS_LOG_LOGIC ("\t packetsInQueue  " << GetInternalQueue (0)->GetNPackets ());

  return isEnqueued;
}

void
BlueQueueDisc::InitializeParams (void)
{

}

bool BlueQueueDisc::DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize)
{
  NS_LOG_FUNCTION (this << item << qSize);
  double u =  m_uv->GetValue ();
  if (u <= m_Pmark)
    {
      return true;
    }
  return false;
}

void BlueQueueDisc::IncrementPmark (uint32_t how)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();

  if (now - m_iFreezeTime > m_iHoldTime)
    {
      m_iFreezeTime = now;
      switch (m_iAlgorithm)
        {
        case 0:
          switch (how)
            {
            case 0:
              m_Pmark += m_increment;
              break;

            case 1:
            default:
              break;
            }
          break;

        case 2:
          switch (how)
            {
            case 0:
              m_Pmark = 2 * m_Pmark + m_increment;
              break;
            case 1:
            default:
              break;
            }
          break;
        default:
        case 1:
          switch (how)
            {
            case 0:
              m_Pmark += m_increment;
              break;
            case 1:
            default:
              m_Pmark += m_increment / 10;
              break;
            }
        }
      if (m_Pmark > 1.0)
        {
          m_Pmark = 1.00;
        }
    }
}

void BlueQueueDisc::DecrementPmark (uint32_t how)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  if (now - m_dFreezeTime > m_dHoldTime)
    {
      m_dFreezeTime = now;
      switch (m_dAlgorithm)
        {
        case 0:
        case 2:
          switch (how)
            {
            case 0:
              m_Pmark -= m_decrement;
              break;
            case 1:
            default:
              break;
            }
          break;

        default:
        case 1:
          switch (how)
            {
            case 0:
              m_Pmark -= m_decrement;
              break;
            case 1:
            default:
              m_Pmark -= m_decrement / 10;
              break;
            }
        }
      if (m_Pmark < 0)
        {
          m_Pmark = 0.0;
        }
    }
}


Ptr<QueueDiscItem>
BlueQueueDisc::DoDequeue ()
{
  NS_LOG_FUNCTION (this);

  if ( GetInternalQueue (0)->IsEmpty () )
    {
      NS_LOG_LOGIC ("Queue empty");
      DecrementPmark (0);
      return 0;
    }

  Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());
  return item;
}

Ptr<const QueueDiscItem>
BlueQueueDisc::DoPeek () const
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
BlueQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("BlueQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("BlueQueueDisc cannot have packet filters");
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
      NS_LOG_ERROR ("BlueQueueDisc needs 1 internal queue");
      return false;
    }

  if (GetInternalQueue (0)->GetMode () != m_mode)
    {
      NS_LOG_ERROR ("The mode of the provided queue does not match the mode set on the BlueQueueDisc");
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
