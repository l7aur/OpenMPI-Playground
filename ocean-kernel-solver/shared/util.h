#pragma once

#define     __try       int __error_code = 0;
#define     __throw(X)  { __error_code = (X); goto finally; }
#define     __finally   finally: