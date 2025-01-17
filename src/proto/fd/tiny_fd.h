/*
    Copyright 2019-2021 (C) Alexey Dynda

    This file is part of Tiny Protocol Library.

    Protocol Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Protocol Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Protocol Library.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 This is Tiny Full-Duplex protocol implementation for microcontrollers.
 It is built on top of Tiny Protocol (hdlc/low_level/hdlc.c)

 @file
 @brief Tiny Protocol Full Duplex API

 @details Implements full duplex asynchronous ballanced mode (ABM)
*/
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "proto/crc/crc.h"
#include "hal/tiny_types.h"

    /**
     * @defgroup FULL_DUPLEX_API Tiny Full Duplex API functions
     * @{
     */

    struct tiny_fd_data_t;

    /**
     * This handle points to service data, required for full-duplex
     * functioning.
     */
    typedef struct tiny_fd_data_t *tiny_fd_handle_t;

    /**
     * This structure is used for initialization of Tiny Full Duplex protocol.
     */
    typedef struct tiny_fd_init_t_
    {
        /// user data for block read/write functions
        void *pdata;
        /// callback function to process incoming frames. Callback is called from tiny_fd_run_rx() context.
        on_frame_cb_t on_frame_cb;
        /// Callback to get notification of sent frames. Callback is called from tiny_fd_run_tx() context.
        on_frame_cb_t on_sent_cb;

        /**
         * buffer to store data during full-duplex protocol operating.
         * The size should be at least size returned by tiny_fd_buffer_size_by_mtu()
         */
        void *buffer;

        /// maximum input buffer size, see tiny_fd_buffer_size_by_mtu()
        uint16_t buffer_size;

        /**
         * timeout. Can be set to 0 during initialization. In this case timeout will be set to default.
         * Timeout parameter sets timeout in milliseconds for blocking API functions: tiny_fd_send().
         */
        uint16_t send_timeout;

        /**
         * timeout for retry operation. It is valid and applicable to I-frames only.
         * retry_timeout sets timeout in milliseconds. If zero value is specified, it is calculated as
         */
        uint16_t retry_timeout;

        /**
         * number retries to perform before timeout takes place
         */
        uint8_t retries;

        /**
         * crc field type to use on hdlc level.
         * If HDLC_CRC_DEFAULT is passed, crc type will be selected automatically (depending on library configuration),
         * but HDLC_CRC_16 has higher priority.
         */
        hdlc_crc_t crc_type;

        /**
         * Number of frames in window, which confirmation may be deferred for. Must be at least 1. Maximum allowable
         * value is 7. Extended HDLC format (with 127 window size) is not yet supported.
         * Smaller values reduce channel throughput, while higher values require more RAM.
         * It is not mandatory to have the same window_frames value on both endpoints.
         */
        uint8_t window_frames;

        /**
         * Maximum transmission unit in bytes. If this parameter is zero, the protocol
         * will automatically calculate mtu based on buffer_size, window_frames.
         */
        int mtu;
    } tiny_fd_init_t;

    /**
     * @brief Initialized communication for Tiny Full Duplex protocol.
     *
     * The function initializes internal structures for Tiny Full Duplex state machine.
     *
     * @param handle - pointer to Tiny Full Duplex data
     * @param init - pointer to tiny_fd_init_t data.
     * @return TINY_NO_ERROR in case of success.
     *         TINY_ERR_FAILED if init parameters are incorrect.
     * @remarks This function is not thread safe.
     */
    extern int tiny_fd_init(tiny_fd_handle_t *handle, tiny_fd_init_t *init);

    /**
     * @brief Returns status of the connection
     *
     * The function returns status of the connection
     *
     * @param handle pointer to Tiny Full Duplex data
     * @return TINY_ERR_INVALID_DATA in case of error
     *         TINY_SUCCESS if connection is established
     *         TINY_ERR_FAILED if protocol is in disconnected state
     */
    extern int tiny_fd_get_status(tiny_fd_handle_t handle);

    /**
     * @brief Sends DISC command to remote side
     *
     * The function sends DISC command to remote side and exits. It doesn't wait for UA answer back.
     *
     * @param handle pointer to Tiny Full Duplex data
     * @return TINY_ERR_INVALID_DATA in case of error
     *         TINY_SUCCESS if DISC command is put to tx queue successfully
     *         TINY_ERR_FAILED if DISC command cannot be put to tx queue since it is full.
     */
    extern int tiny_fd_disconnect(tiny_fd_handle_t handle);

    /**
     * @brief stops Tiny Full Duplex state machine
     *
     * stops Tiny Full Duplex state machine.
     *
     * @param handle handle of full-duplex protocol
     */
    extern void tiny_fd_close(tiny_fd_handle_t handle);

    /**
     * @brief runs tx processing to fill specified buffer with data.
     *
     * Runs tx processing to fill specified buffer with data.
     *
     * @param handle handle of full-duplex protocol
     * @param data pointer to buffer to fill with tx data
     * @param len maximum size of specified buffer
     * @return number of bytes written to specified buffer
     */
    extern int tiny_fd_get_tx_data(tiny_fd_handle_t handle, void *data, int len);

    /**
     * @brief sends tx data to the communication channel via user callback `write_func()`.
     *
     * Sends tx data to the communication channel via user callback `write_func()`.
     * Internally this function generates next 4 bytes to send (or less if nothing to send),
     * and calls user callback write_func() until all generated bytes are sent, or error
     * happens. This function helps to simplify application code.
     *
     * @param handle handle of full-duplex protocol
     * @param write_func callback to the function to write data to the physical channel.
     *
     * @return number of bytes sent
     */
    extern int tiny_fd_run_tx(tiny_fd_handle_t handle, write_block_cb_t write_func);

    /**
     * @brief runs rx bytes processing for specified buffer.
     *
     * Runs rx bytes processing for specified buffer.
     * This is alternative method to run Full-duplex protocol for
     * rx data. Use it, when you wish to control reading from hardware
     * by yourself.
     *
     * @param handle handle of full-duplex protocol
     * @param data pointer to data to process
     * @param len length of data to process
     * @return TINY_SUCCESS
     */
    extern int tiny_fd_on_rx_data(tiny_fd_handle_t handle, const void *data, int len);

    /**
     * @brief reads rx data from the communication channel via user callback `read_func()`
     *
     * Reads rx data from the communication channel via user callback `read_func()`.
     * Internally this function has 4-byte buffer, and tries to read 4 bytes from the channel.
     * Then received bytes are processed by the protocol. If FD protocol detects new incoming
     * message then it calls on_sent_callback.
     * If no data available in the channel, the function returns immediately after read_func() callback
     * returns control.
     *
     * @param handle handle of full-duplex protocol
     * @param read_func callback to the function to read data from the physical channel.
     * @return number of bytes received
     */
    extern int tiny_fd_run_rx(tiny_fd_handle_t handle, read_block_cb_t read_func);

    /**
     * @brief Sends userdata over full-duplex protocol.
     *
     * Sends userdata over full-duplex protocol. Note, that this command
     * will return success, when data are copied to internal queue. That doesn't mean
     * that data are physically sent, but they are enqueued for sending.
     *
     * @important the maximum size of the allowed buffer to send is equal to mtu size set for
     *            the protocol. If you want to send data larger than mtu size, please, use
     *            tiny_fd_send() function.
     *
     * When timeout happens, the data were not actually enqueued. Call this function once again.
     * If TINY_ERR_DATA_TOO_LARGE is returned, try to send less data. If you don't want to care about
     * mtu size, please, use different function tiny_fd_send()..
     *
     * @param handle   tiny_fd_handle_t handle
     * @param buf      data to send
     * @param len      length of data to send
     *
     * @return Success result or error code:
     *         * TINY_SUCCESS          if user data are put to internal queue.
     *         * TINY_ERR_TIMEOUT      if no room in internal queue to put data. Retry operation once again.
     *         * TINY_ERR_FAILED       if request was cancelled, by tiny_fd_close() or other error happened.
     *         * TINY_ERR_DATA_TOO_LARGE if user data are too big to fit in tx buffer.
     */
    extern int tiny_fd_send_packet(tiny_fd_handle_t handle, const void *buf, int len);

    /**
     * Returns minimum required buffer size for specified parameters.
     *
     * @important This function calculates buffer requirements based on HDLC_CRC_16.
     *
     * @param mtu size of desired user payload in bytes.
     * @param window maximum tx queue size of I-frames.
     */
    extern int tiny_fd_buffer_size_by_mtu(int mtu, int window);

    /**
     * Returns minimum required buffer size for specified parameters.
     *
     * @param mtu size of desired user payload in bytes.
     * @param window maximum tx queue size of I-frames.
     * @param crc_type crc type to be used with FD protocol
     */
    extern int tiny_fd_buffer_size_by_mtu_ex(int mtu, int window, hdlc_crc_t crc_type);

    /**
     * @brief returns max packet size in bytes.
     *
     * Returns max packet size in bytes.
     *
     * @param handle   tiny_fd_handle_t handle
     * @return mtu size in bytes
     * @see tiny_fd_send_packet
     */
    extern int tiny_fd_get_mtu(tiny_fd_handle_t handle);

    /**
     * @brief Sends userdata over full-duplex protocol.
     *
     * Sends userdata over full-duplex protocol. Note, that this function will try to send
     * all data from buf. In success case it will return number of bytes sent, equal to len
     * input parameter. But if timeout happens, it returns number of bytes actually enqueued.
     *
     * If you constantly get number of sent bytes less than expected, try to increase
     * timeout value of the speed of used communication channel.
     *
     * @param handle   tiny_fd_handle_t handle
     * @param buf      data to send
     * @param len      length of data to send
     *
     * @return Number of bytes sent
     */
    extern int tiny_fd_send(tiny_fd_handle_t handle, const void *buf, int len);

    /**
     * Sets keep alive timeout in milliseconds. This timeout is used to send special RR
     * frames, when no user data queued for sending.
     * @param handle   pointer to tiny_fd_handle_t
     * @param keep_alive timeout in milliseconds
     */
    extern void tiny_fd_set_ka_timeout(tiny_fd_handle_t handle, uint32_t keep_alive);

    /**
     * @}
     */

#ifdef __cplusplus
}
#endif
