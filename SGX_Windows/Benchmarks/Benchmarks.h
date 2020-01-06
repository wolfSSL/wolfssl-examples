/* Benchmarks.h
*
* Copyright (C) 2006-2020 wolfSSL Inc.
*
* This file is part of wolfSSL.
*
* wolfSSL is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* wolfSSL is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
*/

#ifndef BENCHMARKS_H
#define BENCHMARKS_H

#include "sgx_urts.h"	 /* Manages Enclave */
#include <iostream>		 /* IO stream Operations */
#include "sgx_status.h"  /* checks the status of the sgx APIs */
#include "Enclave_u.h"   /* contains untrusted wrapper functions used to call enclave functions*/
#include <sstream> 

#define BENCH_RSA

enum BenchmarkBounds {
	/* these numbers are lower then default wolfSSL one to collect benchmark values faster for GUI */
	numBlocks = 10, /* how many megs to test */
	ntimes = 30 /* how many itteration to run RSA decrypt/encrypt */
};

#endif