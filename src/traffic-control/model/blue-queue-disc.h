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
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief BlueQueueDisc Constructor
   */
  BlueQueueDisc ();

  /**
   * \brief BlueQueueDisc Destructor
   */
  virtual ~BlueQueueDisc ();



  /**
   * \brief Set the operating mode of this queue.
   *
   * \param mode The operating mode of this queue.
   */
  void SetMode (Queue::QueueMode mode);

  /**
   * \brief Get the encapsulation mode of this queue.
   *
   * \returns The encapsulation mode of this queue.
   */
  Queue::QueueMode GetMode (void);

  /**
   * \brief Get the current value of the queue in bytes or packets.
   *
   * \returns The queue size in bytes or packets.
   */
  uint32_t GetQueueSize (void);

  /**
   * \brief Set the limit of the queue in bytes or packets.
   *
   * \param lim The limit in bytes or packets.
   */
  void SetQueueLimit (uint32_t lim);

  /**
   * \brief Get queue delay
   */
  Time GetQueueDelay (void);

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);


protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);


  /**
   * \brief Initialize the queue parameters.
   */
  virtual void InitializeParams (void);

  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void) const;
  virtual bool CheckConfig (void);

  virtual void IncrementPmark (uint32_t how);                // how is used for specifing increment type  // I think it's not necessary to have 'how' as this is not specified in the paper
  virtual void DecrementPmark (uint32_t how);                // how is used for specifing decrement type  // I think it's not necessary to have 'how' as this is not specified in the paper

  /**
   * \brief Check if a packet needs to be dropped due to probability drop
   * \param item queue item
   * \param qSize queue size
   * \returns false for no drop, true for drop
   */
  virtual bool DropEarly (Ptr<QueueDiscItem> item, uint32_t qSize);


private:
  Queue::QueueMode m_mode;                        //!< Mode (bytes or packets)
  uint32_t m_queueLimit;                      //!< Queue limit in bytes / packets


  Ptr<UniformRandomVariable> m_uv;                //!< Rng stream


  uint32_t m_dropFront;                               // drop-from-front (rather than from tail)
  uint32_t m_bytes;                                   // bytes or packet as measuring unit
  uint32_t m_setBit;                                  // Whether to Use ECN (Cannot use this because ns-3 doesn't have support for ECN)
  uint32_t m_meanPktSize;                             // Average Packet Size
  double m_decrement;                            // marking probability decrement value
  double m_increment;                            // marking probability increment value
  Time m_iHoldTime;                              // last time at which pmark incremented
  Time m_dHoldTime;                              // last time at which pmark decremented
  uint32_t m_dAlgorithm;                              // which decrement algo to use (refer to ns-2 code) (default is additive decrease)
  uint32_t m_iAlgorithm;                              // which increment algo to use (refer to ns-2 code) (default is additive increase)
  double m_bandwidth;                            // ??

  int m_idle;                                     //??
  Time m_idletime;                                //??
  double m_ptc;                                   //??
  Time m_iFreezeTime;                             // Time interval during which pmark cannot be increased
  Time m_dFreezeTime;                             // Time interval during which pmark cannot be decreased
  double m_Pmark;                                 // Marking Probability

};

}    // namespace ns3

#endif

