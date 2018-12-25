make version 1.0

Created by Yongzhen Huang

#####################################
Inspired by the original Make program
#####################################

Requires a file named Makefile in order to execute.

Each command is in the form of 

TARGET: DEPENDENCIES
	ACTIONS

Currently the program only supports .c and .h files as dependencies and only 1 action.

NOTE: there MUST be a tab before ACTIONS or the make would not work