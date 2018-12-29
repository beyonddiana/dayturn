/*
 * @file main-objc.mm
 * @brief Dayturn for macOS application entry
 * Dayturn
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Dayturn Viewer Source Code
 * Created by Dayturn on 27.05.2016.
 * Copyright (C) 2017, Geir Nøklebye <geir.noklebye@dayturn.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details  
 * $/LicenseInfo$
*/

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

int main(int argc, char * argv[]) 
{
    return NSApplicationMain(argc, (const char**)argv);   
}

// argc is a counter to the number of arguments given to the program when started with the terminal open command.
// argv is an array of char pointers holding the arguments given. 
// The first entry in argv is always the name of the command itself, meaning that argc will always be 1. 
