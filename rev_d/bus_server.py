#!/usr/bin/env python

# This code initially came from:
#
#    http://www.binarytides.com/python-socket-server-code-example/
#
# It will be heavily edited:

# A socket server in python using select function:
 
import glob
import select
import serial
import socket
import sys
  
def main():
    # Search command line arguments for pattern "/dev/tty*":
    unix_serials = glob.glob("/dev/ttyUSB*")
    macos_serials = glob.glob("/dev/tty.usbserial-*")
    pi_serials = glob.glob("/dev/ttyAMA*")

    # Sort everything that we found and concatenate them together:
    pi_serials.sort()
    unix_serials.sort()
    macos_serials.sort()
    serials = unix_serials + macos_serials + pi_serials
    print("serials={0}".format(serials))

    # Squirt out an error message
    bus_serial = None
    if len(serials) == 0:
	print "There is no serial port to open"
	serial_name = None
    else:
	# Use the device listed on the command line:
	serial_name = serials[0]

	# Try to open a serial connection:
	bus_serial = None
	if serial_name != None:
	    try:
		bus_serial = serial.Serial(serial_name, 115200)
	    except:
		bus_serial = None
    if bus_serial == None:
	print "Unable to open serial port '{0}'".format(serial_name)
	sys.exit(1)
    print("Opened {0}".format(serial_name))

    # Now fire up the server:
    connections = []    # list of socket clients
    receive_buffer_size = 4096 # Advisable to keep it as an exponent of 2
    listen_port = 5000

    # Create the internet protocol *server_socket*:
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # If you want to know what the line below does, read the following:
    #
    #    http://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Set *server_sockect* to start listening for all connections:
    server_socket.bind(("0.0.0.0", listen_port))
    server_socket.listen(10)

    # Add server socket to the list of readable *connections*:
    connections.append(server_socket)

    print("Bus server listening on port {0}".format(listen_port))

    pending_data = {}
    while True:
	# Get the list sockets which are ready to be read through select:
	read_sockets, write_sockets, error_sockets = \
	  select.select(connections, [], [])

	# Process each socket:
	for read_socket in read_sockets:
	    if read_socket == server_socket:
		# We got a new connection from *server_socket*:
		socket_fd, address = server_socket.accept()
		connections.append(socket_fd)
		pending_data[socket_fd] = []
		print("Client {0} connected".format(address))
	    else:
		# *read_socket* is a client connection and it either has
		# some data for us or it has closed:

		# In Windows (barf), sometimes when a TCP program closes
		# abruptly, a "Connection reset by peer" exception will
		# be thrown:
		try:
		    # Read some more *data*:
		    received_data = read_socket.recv(receive_buffer_size)

		    if len(recieved_data) == 0:
			# No data means the connection is closed:
			print("Closed {0}".format(address))
			read_socket.close()
			connections.remove(read_socket)
		    else:
			# We have some *data* that we need to process:

			# Prepend any *previous_data* that has not yet
			# been processed:
			data = pending_data[read_socket]
			for character in received_data:
			    data.append(ord(character))

			# Format of a server request is:
			#
			# [0]: 0xa5		# Begin synchronization byte
			# [1]: Cmd		# Command
			# [2]: L		# Data length
			# [3]: Data[0]		# Data[0]
			# [4]: Data[1]		# Data[1]
			# ...
			# [L+2]: Data[L-1]	# Data[L-1]
			# [L+3]: 0x5a		# End synchronization byte

			# Now see if we have enough *data* to process:
			size = len(data)
			if size > 0 and data[0] != 0xa5:
			    # There is no begin synchronization byte:
			    print("No begin synchronization byte")
			    read_socket.close()
			    connections.remove(read_socket)
			elif size < 4 or size < data[2] + 4:
			    # A server request consists of a packet contains
			    # [Begin, Cmd, L, {L Data bytes}, End]
			    pass
			elif data[-1] != 0x5a:
			    print("No end synchronization byte")
			    read_socket.close()
			    connections.remove(read_socket)
			elif data[1] == 1:
			    # Standard data packet send receive:
			    length = data[2]
			    bus_address = data[3]

			    # Compute 4-bit check-sum:
			    check_sum = 0
			    for byte in data[4:-1]:
				check_sum += byte
			    check_sum += check_sum >> 4
			    check_sum &= 0xf

			    # Send the address as a double byte sequence:
			    bytes.append(0xc2 | (byte >> 7))
			    bytes.append(address & 0x7f)

			    # Send the rest of the bytes:
			    bytes = []
			    for byte in data[4:-1]:
				if 0xc0 <= byte <= 0xc7:
				    # We need to convert into a double byte
				    # sequence:
				    bytes.append(0xc0 | (byte >> 7))
				    bytes.append(byte & 0x7f);

			    # Now convert *bytes* into *send_text*:
			    send_text = map(str, bytes).join()
			    serial.write(send_text)

			    bytes = []
                            header_text = serial.read(1)
			    if len(header_text) == 0:
				# Timeout:
				print("No response header byte")
				read_socket.close()
				connections_remove(read_socket)
			    else:
				# We got a header:
				header_byte = ord(header_text)
				header_length = header_byte >> 4
				header_check_sum = header_byte & 0xf
				if header_length == 0 && header_check_sum != 0:
				    # We have an error:
				    print("Response error: {0}".
				      format(header_check_sum))
				    read_socket.close()
				    connections_remove(read_socket)
				else:
				    response_text = serial.read(header_length)
				    if len(response_text) != header_length:
					# Timeout:
					print("Response packet timeout")
					read_socket.close()
					connections_remove(read_socket)
				    else:
					# Return the response length + data:
					read_socket.send(
					  str(header_length) + response_text)
			else:
                            print("Bad command: {0}".format(data[1]))
			    read_socket.close()
			    connections.remove(read_socket)
		except:
		    # Assume something bad has happened on *read_socket*
		    # and shut it down:
		    print("Exception on {0}".format(address))
		    read_socket.close()
		    connections.remove(read_socket)

    # It is unclear how we get here to close *server_socket*:
    server_socket.close()

if __name__ == "__main__":
    main()
