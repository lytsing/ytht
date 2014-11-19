/*

packet.h

Author: Tatu Ylonen <ylo@cs.hut.fi>

Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
                   All rights reserved

Created: Sat Mar 18 02:02:14 1995 ylo

Interface for the packet protocol functions.

*/

/*
 * $Id: packet.h 2237 2002-10-01 09:42:04Z ylsdd $
 * $Log$
 * Revision 1.1  2002/10/01 09:42:05  ylsdd
 * Initial revision
 *
 * Revision 1.3  2002/08/04 11:39:42  kcn
 * format c
 *
 * Revision 1.2  2002/08/04 11:08:47  kcn
 * format C
 *
 * Revision 1.1.1.1  2002/04/27 05:47:25  kxn
 * no message
 *
 * Revision 1.1  2001/07/04 06:07:11  bbsdev
 * bbs sshd
 *
 * Revision 1.7  1998/08/04 00:04:57  kivinen
 * 	Removed socks.h.
 *
 * Revision 1.6  1998/03/27  16:59:16  kivinen
 * 	Added socks.h include.
 *
 * Revision 1.5  1997/04/05 17:29:14  ylo
 * 	Added packet_get_len (returns the remaining length of incoming
 * 	packet).
 *
 * Revision 1.4  1997/03/26 07:11:41  kivinen
 * 	Fixed prototypes.
 *
 * Revision 1.3  1997/03/19 19:26:16  kivinen
 * 	Added packet_get_all prototype.
 *
 * Revision 1.2  1996/11/24 08:24:14  kivinen
 * 	Fixed the comment of packet_send_debug.
 *
 * Revision 1.1.1.1  1996/02/18 21:38:10  ylo
 * 	Imported ssh-1.2.13.
 *
 * Revision 1.4  1995/09/24  23:59:20  ylo
 * 	Added packet_get_protocol_flags.
 *
 * Revision 1.3  1995/07/27  02:17:53  ylo
 * 	Pass as argument to packet_set_encryption_key whether running
 * 	as the client or the server.
 *
 * Revision 1.2  1995/07/13  01:27:54  ylo
 * 	Removed "Last modified" header.
 * 	Added cvs log.
 *
 * $Endlog$
 */

#ifndef PACKET_H
#define PACKET_H

#include <gmp.h>
#include "randoms.h"

/* Sets the socket used for communication.  Disables encryption until
   packet_set_encryption_key is called.  It is permissible that fd_in
   and fd_out are the same descriptor; in that case it is assumed to
   be a socket. */
void packet_set_connection(int fd_in, int fd_out, RandomState * state);

/* Puts the connection file descriptors into non-blocking mode. */
void packet_set_nonblocking(void);

/* Returns the file descriptor used for input. */
int packet_get_connection_in(void);

/* Returns the file descriptor used for output. */
int packet_get_connection_out(void);

/* Closes the connection (both descriptors) and clears and frees
   internal data structures. */
void packet_close(void);

/* Causes any further packets to be encrypted using the given key.  The same
   key is used for both sending and reception.  However, both directions
   are encrypted independently of each other.  Cipher types are
   defined in ssh.h. */
void packet_set_encryption_key(const unsigned char *key, unsigned int keylen, int cipher_type, int is_client);

/* Sets remote side protocol flags for the current connection.  This can
   be called at any time. */
void packet_set_protocol_flags(unsigned int flags);

/* Returns the remote protocol flags set earlier by the above function. */
unsigned int packet_get_protocol_flags(void);

/* Enables compression in both directions starting from the next packet. */
void packet_start_compression(int level);

/* Informs that the current session is interactive.  Sets IP flags for optimal
   performance in interactive use. */
void packet_set_interactive(int interactive, int keepalives);

/* Returns true if the current connection is interactive. */
int packet_is_interactive(void);

/* Starts constructing a packet to send. */
void packet_start(int type);

/* Appends a character to the packet data. */
void packet_put_char(int ch);

/* Appends an integer to the packet data. */
void packet_put_int(unsigned int value);

/* Appends an arbitrary precision integer to packet data. */
void packet_put_mp_int(MP_INT * value);

/* Appends a string to packet data. */
void packet_put_string(const char *buf, unsigned int len);

/* Finalizes and sends the packet.  If the encryption key has been set,
   encrypts the packet before sending. */
void packet_send(void);

/* Waits until a packet has been received, and returns its type. */
int packet_read(void);

/* Waits until a packet has been received, verifies that its type matches
   that given, and gives a fatal error and exits if there is a mismatch. */
void packet_read_expect(int type);

/* Checks if a full packet is available in the data received so far via
   packet_process_incoming.  If so, reads the packet; otherwise returns
   SSH_MSG_NONE.  This does not wait for data from the connection. 
   
   SSH_MSG_DISCONNECT is handled specially here.  Also,
   SSH_MSG_IGNORE messages are skipped by this function and are never returned
   to higher levels. */
int packet_read_poll(void);

/* Buffers the given amount of input characters.  This is intended to be
   used together with packet_read_poll. */
void packet_process_incoming(const char *buf, unsigned int len);

/* Returns the remaining number of bytes in the incoming packet. */
unsigned int packet_get_len(void);

/* Returns a character (0-255) from the packet data. */
unsigned int packet_get_char(void);

/* Returns an integer from the packet data. */
unsigned int packet_get_int(void);

/* Returns an arbitrary precision integer from the packet data.  The integer
   must have been initialized before this call. */
void packet_get_mp_int(MP_INT * value);

/* Returns a string from the packet data.  The string is allocated using
   xmalloc; it is the responsibility of the calling program to free it when
   no longer needed.  The length_ptr argument may be NULL, or point to an
   integer into which the length of the string is stored. */
char *packet_get_string(unsigned int *length_ptr);

/* Clears incoming data buffer */
void packet_get_all(void);

/* Logs the error in syslog using LOG_INFO, constructs and sends a disconnect
   packet, closes the connection, and exits.  This function never returns.
   The error message should not contain a newline.  The total length of the
   message must not exceed 1024 bytes. */
void packet_disconnect(const char *fmt, ...);

/* Sends a diagnostic message to the other side.  This message
   can be sent at any time (but not while constructing another message).
   The message is printed immediately, but only if the client is being
   executed in verbose mode. If the first character of the message is '*'
   the message is printed to stderr always.
   These messages are primarily intended to ease debugging authentication
   problems. The total length of the message must not exceed 1024 bytes. This
   will automatically call packet_write_wait. If the remote side protocol flags
   do not indicate that it supports SSH_MSG_DEBUG, this will do nothing. */
void packet_send_debug(const char *fmt, ...);

/* Checks if there is any buffered output, and tries to write some of the
   output. */
void packet_write_poll(void);

/* Waits until all pending output data has been written. */
void packet_write_wait(void);

/* Returns true if there is buffered data to write to the connection. */
int packet_have_data_to_write(void);

/* Returns true if there is not too much data to write to the connection. */
int packet_not_very_much_data_to_write(void);

/* Sets the maximum packet size that can be sent to the other side. */
void packet_set_max_size(unsigned int max_size);

/* Returns the maximum packet size that can be sent to the other side. */
unsigned int packet_max_size(void);

/* Parses tty modes for the fd from the current packet. */
void tty_parse_modes(int fd);

#endif                          /* PACKET_H */
