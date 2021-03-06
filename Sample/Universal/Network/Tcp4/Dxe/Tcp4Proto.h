/*++

Copyright (c) 2005 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Tcp4Proto.h

Abstract:

--*/

#ifndef _TCP4_PROTO_H_
#define _TCP4_PROTO_H_

typedef struct _TCP_CB  TCP_CB;

#include "Tcp4Driver.h"
#include "Socket.h"
#include "NetBuffer.h"
#include "Tcp4Option.h"

//
// tcp states, Don't change their order, it is used as
// index to mTcpOutFlag and other macros
//
enum {
  TCP_CLOSED      = 0,
  TCP_LISTEN,
  TCP_SYN_SENT,
  TCP_SYN_RCVD,
  TCP_ESTABLISHED,
  TCP_FIN_WAIT_1,
  TCP_FIN_WAIT_2,
  TCP_CLOSING,
  TCP_TIME_WAIT,
  TCP_CLOSE_WAIT,
  TCP_LAST_ACK,
};

//
// flags in the TCP header
//
enum {

  TCP_FLG_FIN     = 0x01,
  TCP_FLG_SYN     = 0x02,
  TCP_FLG_RST     = 0x04,
  TCP_FLG_PSH     = 0x08,
  TCP_FLG_ACK     = 0x10,
  TCP_FLG_URG     = 0x20,
  TCP_FLG_FLAG    = 0x3F, // mask for all the flags
};

enum {

  //
  // TCP error status
  //
  TCP_CONNECT_REFUSED     = -1,
  TCP_CONNECT_RESET       = -2,
  TCP_CONNECT_CLOSED      = -3,

  //
  // Current congestion status as suggested by RFC3782.
  //
  TCP_CONGEST_RECOVER     = 1,  // during the NewReno fast recovery
  TCP_CONGEST_LOSS        = 2,  // retxmit because of retxmit time out
  TCP_CONGEST_OPEN        = 3,  // TCP is opening its congestion window

  //
  // TCP control flags
  //
  TCP_CTRL_NO_NAGLE       = 0x0001, // disable Nagle algorithm
  TCP_CTRL_NO_KEEPALIVE   = 0x0002, // disable keepalive timer
  TCP_CTRL_NO_WS          = 0x0004, // disable window scale option
  TCP_CTRL_RCVD_WS        = 0x0008, // rcvd a wnd scale option in syn
  TCP_CTRL_NO_TS          = 0x0010, // disable Timestamp option
  TCP_CTRL_RCVD_TS        = 0x0020, // rcvd a Timestamp option in syn
  TCP_CTRL_SND_TS         = 0x0040, // Send Timestamp option to remote
  TCP_CTRL_SND_URG        = 0x0080, // in urgent send mode
  TCP_CTRL_RCVD_URG       = 0x0100, // in urgent receive mode
  TCP_CTRL_SND_PSH        = 0x0200, // in PUSH send mode
  TCP_CTRL_FIN_SENT       = 0x0400, // FIN is sent
  TCP_CTRL_FIN_ACKED      = 0x0800, // FIN is ACKed.
  TCP_CTRL_TIMER_ON       = 0x1000, // At least one of the timer is on
  TCP_CTRL_RTT_ON         = 0x2000, // The RTT measurement is on
  TCP_CTRL_ACK_NOW        = 0x4000, // Send the ACK now, don't delay

  //
  // Timer related values
  //
  TCP_TIMER_CONNECT       = 0,                // Connection establishment timer
  TCP_TIMER_REXMIT        = 1,                // retransmit timer
  TCP_TIMER_PROBE         = 2,                // Window probe timer
  TCP_TIMER_KEEPALIVE     = 3,                // Keepalive timer
  TCP_TIMER_FINWAIT2      = 4,                // FIN_WAIT_2 timer
  TCP_TIMER_2MSL          = 5,                // TIME_WAIT tiemr
  TCP_TIMER_NUMBER        = 6,                // the total number of TCP timer.
  TCP_TICK                = 200,              // every TCP tick is 200ms
  TCP_TICK_HZ             = 5,                // the frequence of TCP tick
  TCP_RTT_SHIFT           = 3,                // SRTT & RTTVAR scaled by 8
  TCP_RTO_MIN             = TCP_TICK_HZ,      // the minium value of RTO
  TCP_RTO_MAX             = TCP_TICK_HZ *60,  // the maxium value of RTO
  TCP_FOLD_RTT            = 4,                // timeout threshod to fold RTT

  //
  // default values for some timers
  //
  TCP_MAX_LOSS            = 12,                  // default max times to retxmit
  TCP_KEEPALIVE_IDLE_MIN  = TCP_TICK_HZ *60 *60 *2, // First keep alive
  TCP_KEEPALIVE_PERIOD    = TCP_TICK_HZ *60,
  TCP_MAX_KEEPALIVE       = 8,
  TCP_FIN_WAIT2_TIME      = 2 *TCP_TICK_HZ,         // * 60,
  TCP_TIME_WAIT_TIME      = 2 *TCP_TICK_HZ,
  TCP_PAWS_24DAY          = 24 *24 *60 *60 *TCP_TICK_HZ,
  TCP_CONNECT_TIME        = 75 *TCP_TICK_HZ,

  //
  // The header space to be reserved before TCP data to accomodate :
  // 60byte IP head + 60byte TCP head + link layer head
  //
  TCP_MAX_HEAD            = 192,

  //
  // value ranges for some control option
  //
  TCP_RCV_BUF_SIZE        = 2 *1024 *1024,
  TCP_RCV_BUF_SIZE_MIN    = 8 *1024,
  TCP_SND_BUF_SIZE        = 2 *1024 *1024,
  TCP_SND_BUF_SIZE_MIN    = 8 *1024,
  TCP_BACKLOG             = 10,
  TCP_BACKLOG_MIN         = 5,
  TCP_MAX_LOSS_MIN        = 6,
  TCP_CONNECT_TIME_MIN    = 60 *TCP_TICK_HZ,
  TCP_MAX_KEEPALIVE_MIN   = 4,
  TCP_KEEPALIVE_IDLE_MAX  = TCP_TICK_HZ *60 *60 *4,
  TCP_KEEPALIVE_PERIOD_MIN= TCP_TICK_HZ *30,
  TCP_FIN_WAIT2_TIME_MAX  = 4 *TCP_TICK_HZ,
  TCP_TIME_WAIT_TIME_MAX  = 60 *TCP_TICK_HZ,
};

typedef struct _TCP_SEG {
  TCP_SEQNO Seq;  // Starting sequence number
  TCP_SEQNO End;  // The sequence of the last byte + 1,
                  // include SYN/FIN. End-Seq = SEG.LEN
  TCP_SEQNO Ack;  // ACK fild in the segment
  UINT8     Flag; // TCP header flags
  UINT16    Urg;  // Valid if URG flag is set.
  UINT32    Wnd;  // TCP window size field
} TCP_SEG;

typedef struct _TCP_PEER {
  UINT32      Ip;   // Network byte order
  TCP_PORTNO  Port; // Network byte order
} TCP_PEER;

//
// tcp control block, it includes various states
//
typedef struct _TCP_CB {
  NET_LIST_ENTRY    List;
  TCP_CB            *Parent;

  SOCKET            *Sk;
  TCP_PEER          LocalEnd;
  TCP_PEER          RemoteEnd;

  NET_LIST_ENTRY    SndQue;   // retxmission queue
  NET_LIST_ENTRY    RcvQue;   // reassemble queue
  UINT32            CtrlFlag; // control flags, such as NO_NAGLE
  INT32             Error;    // soft error status,TCP_CONNECT_RESET...

  //
  // RFC793 and RFC1122 defined variables
  //
  UINT8             State;      // TCP state, such as SYN_SENT, LISTEN
  UINT8             DelayedAck; // number of delayed ACKs
  UINT16            HeadSum;    // checksum of the fixed parts of pesudo
                                // header: Src IP, Dst IP, 0, Protocol,
                                // not include the TCP length.

  TCP_SEQNO         Iss;        // Initial Sending Sequence
  TCP_SEQNO         SndUna;     // first unacknowledged data
  TCP_SEQNO         SndNxt;     // next data sequence to send.
  TCP_SEQNO         SndPsh;     // Send PUSH point
  TCP_SEQNO         SndUp;      // Send urgent point
  UINT32            SndWnd;     // Window advertised by the remote peer
  UINT32            SndWndMax;  // max send window advertised by the peer
  TCP_SEQNO         SndWl1;     // Seq number used for last window update
  TCP_SEQNO         SndWl2;     // ack no of last window update
  UINT16            SndMss;     // Max send segment size
  TCP_SEQNO         RcvNxt;     // Next sequence no to receive
  UINT32            RcvWnd;     // Window advertised by the local peer
  TCP_SEQNO         RcvWl2;     // The RcvNxt (or ACK) of last window update.
                                // It is necessary because of delayed ACK

  TCP_SEQNO         RcvUp;                   // urgent point;
  TCP_SEQNO         Irs;                     // Initial Receiving Sequence
  UINT16            RcvMss;                  // Max receive segment size
  UINT16            EnabledTimer;            // which timer is currently enabled
  UINT32            Timer[TCP_TIMER_NUMBER]; // when the timer will expire
  INT32             NextExpire;  // count down offset for the nearest timer
  UINT32            Idle;        // How long the connection is in idle
  UINT32            ProbeTime;   // the time out value for current window prober

  //
  // RFC1323 defined variables, about window scale,
  // timestamp and PAWS
  //
  UINT8             SndWndScale;  // Wndscale received from the peer
  UINT8             RcvWndScale;  // Wndscale used to scale local buffer
  UINT32            TsRecent;     // TsRecent to echo to the remote peer
  UINT32            TsRecentAge;  // When this TsRecent is updated

  // TCP_SEQNO  LastAckSent;
  // It isn't necessary to add LastAckSent here,
  // since it is the same as RcvWl2

  //
  // RFC2988 defined variables. about RTT measurement
  //
  TCP_SEQNO         RttSeq;     // the seq of measured segment now
  UINT32            RttMeasure; // currently measured RTT in heart beats
  UINT32            SRtt;       // Smoothed RTT, scaled by 8
  UINT32            RttVar;     // RTT variance, scaled by 8
  UINT32            Rto;        // Current RTO, not scaled

  //
  // RFC2581, and 3782 variables.
  // Congestion control + NewReno fast recovery.
  //
  UINT32            CWnd;         // Sender's congestion window
  UINT32            Ssthresh;     // Slow start threshold.
  TCP_SEQNO         Recover;      // recover point for NewReno
  UINT16            DupAck;       // number of duplicate ACKs
  UINT8             CongestState; // the current congestion state(RFC3782)
  UINT8             LossTimes;    // number of retxmit timeouts in a row
  TCP_SEQNO         LossRecover;  // recover point for retxmit

  //
  // configuration parameters, for EFI_TCP4_PROTOCOL specification
  //
  UINT32            KeepAliveIdle;   // idle time before sending first probe
  UINT32            KeepAlivePeriod; // interval for subsequent keep alive probe
  UINT8             MaxKeepAlive;    // Maxium keep alive probe times.
  UINT8             KeepAliveProbes; // the number of keep alive probe.
  UINT16            MaxRexmit;      // The maxium number of retxmit before abort
  UINT32            FinWait2Timeout; // The FIN_WAIT_2 time out
  UINT32            TimeWaitTimeout; // The TIME_WAIT time out
  UINT32            ConnectTimeout;

  //
  // configuration for tcp provided by user
  //
  BOOLEAN           UseDefaultAddr;
  UINT8             TOS;
  UINT8             TTL;
  EFI_IPv4_ADDRESS  SubnetMask;

  //
  // pointer reference to Ip used to send pkt
  //
  IP_IO_IP_INFO     *IpInfo;
} TCP_CB;

extern NET_LIST_ENTRY mTcpRunQue;
extern NET_LIST_ENTRY mTcpListenQue;
extern TCP_SEQNO      mTcpGlobalIss;
extern UINT32         mTcpTick;

//
// TCP_CONNECTED: both ends have synchronized their ISN.
//
#define TCP_CONNECTED(state)  ((state) > TCP_SYN_RCVD)

#define TCP_FIN_RCVD(State) \
    (((State) == TCP_CLOSE_WAIT) || \
    ((State) == TCP_LAST_ACK) || \
    ((State) == TCP_CLOSING) || \
    ((State) == TCP_TIME_WAIT))

#define TCP_LOCAL_CLOSED(State) \
      (((State) == TCP_FIN_WAIT_1) || \
      ((State) == TCP_FIN_WAIT_2) || \
      ((State) == TCP_CLOSING) || \
      ((State) == TCP_TIME_WAIT) || \
      ((State) == TCP_LAST_ACK))

//
// Get the TCP_SEG point from a net buffer's ProtoData
//
#define TCPSEG_NETBUF(NBuf) ((TCP_SEG *) ((NBuf)->ProtoData))

//
// macros to compare sequence no
//
#define TCP_SEQ_LT(SeqA, SeqB)  ((INT32) ((SeqA) - (SeqB)) < 0)
#define TCP_SEQ_LEQ(SeqA, SeqB) ((INT32) ((SeqA) - (SeqB)) <= 0)
#define TCP_SEQ_GT(SeqA, SeqB)  ((INT32) ((SeqB) - (SeqA)) < 0)
#define TCP_SEQ_GEQ(SeqA, SeqB) ((INT32) ((SeqB) - (SeqA)) <= 0)

//
// TCP_SEQ_BETWEEN return whether b <= m <= e
//
#define TCP_SEQ_BETWEEN(b, m, e)  ((e) - (b) >= (m) - (b))

//
// TCP_SUB_SEQ returns Seq1 - Seq2. Make sure Seq1 >= Seq2
//
#define TCP_SUB_SEQ(Seq1, Seq2)     ((UINT32) ((Seq1) - (Seq2)))

#define TCP_FLG_ON(Value, Flag)     ((BOOLEAN) (((Value) & (Flag)) != 0))
#define TCP_SET_FLG(Value, Flag)    ((Value) |= (Flag))
#define TCP_CLEAR_FLG(Value, Flag)  ((Value) &= ~(Flag))

//
// test whether two peers are equal
//
#define TCP_PEER_EQUAL(Pa, Pb) \
  (((Pa)->Ip == (Pb)->Ip) && ((Pa)->Port == (Pb)->Port))

//
// test whether Pa matches Pb, or Pa is more specific
// than pb. Zero means wildcard.
//
#define TCP_PEER_MATCH(Pa, Pb) \
    ((((Pb)->Ip == 0) || ((Pb)->Ip == (Pa)->Ip)) && \
    (((Pb)->Port == 0) || ((Pb)->Port == (Pa)->Port)))

#define TCP_TIMER_ON(Flag, Timer)     ((Flag) & (1 << (Timer)))
#define TCP_SET_TIMER(Flag, Timer)    ((Flag) |= (1 << (Timer)))
#define TCP_CLEAR_TIMER(Flag, Timer)  ((Flag) &= ~(1 << (Timer)))

#define TCP_TIME_LT(Ta, Tb)           ((INT32) ((Ta) - (Tb)) < 0)
#define TCP_TIME_LEQ(Ta, Tb)          ((INT32) ((Ta) - (Tb)) <= 0)
#define TCP_SUB_TIME(Ta, Tb)          ((UINT32) ((Ta) - (Tb)))

#define TCP_MAX_WIN                   0xFFFFU

typedef
VOID
(*TCP_TIMER_HANDLER) (
  IN TCP_CB * Tcb
  );

#include "Tcp4Func.h"
#endif
