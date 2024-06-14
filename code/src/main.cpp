/**
 * @ubit_name_assignment1
 * @author  William Becker <wbecker2@buffalo.edu>
 * @author	Timothy Erckert <terckert@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/chatapp.h"
#include "../include/chatclient.h"
#include "../include/chatserver.h"

using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{	
	/*
		Validates input length from input, and prints message if input is invalid.
	*/
	if(argc != 3) {
		cse4589_print_and_log("Incorrect Usage\nFormant:%s [s/c] [port]\n", argv[0]);
		return -1;
	}
	
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	ChatApp *app;
	if(argv[1][0] == 's'){
		app = new ChatServer(argv[2]);
	} else if(argv[1][0] == 'c') {
		app = new ChatClient(argv[2]);
	} else {
		cse4589_print_and_log("Incorrect Usage\nFormant:%s [s/c] [port]\n", argv[0]);
		return -1;
	}
	
	int res = app->run(argv[2]);
	
	return 0;
}
