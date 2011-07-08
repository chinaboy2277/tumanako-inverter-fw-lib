//TODO: turn this into a proper unit test

#include "../include/FixedPoint.h"

typedef tFixedPoint< 4 >   tQ4;
typedef tFixedPoint< 8 >   tQ8;
typedef tFixedPoint< 12 >  tQ12;
typedef tFixedPoint< 16 >  tQ16;
typedef tFixedPoint< 18 >  tQ18;

typedef tFixedPoint< 18, long long >  tBigQ18;
typedef tFixedPoint< 36, long long >  tBigQ36;

/*
typedef tFixedPoint< 4,  unsigned >   tQ4;
typedef tFixedPoint< 8,  unsigned >   tQ8;
typedef tFixedPoint< 12, unsigned >  tQ12;
typedef tFixedPoint< 16, unsigned >  tQ16;
typedef tFixedPoint< 18, unsigned >  tQ18;

typedef tFixedPoint< 18, unsigned long long >  tBigQ18;
typedef tFixedPoint< 36, unsigned long long >  tBigQ36;
*/

#define Q8CONST( dec,frac )  FIXEDPOINT_CONSTANT( tQ8, dec,frac )

int main()
{
    tQ4 a4( 1.1 );
    tQ4 b4( 1 );
    tQ12 a12( 3.3 );
    tQ12 b12( 3 );
    tQ18 a18;
    double ad;
    double bd;
    
    tQ8 a8( -2.3 );
    tQ8 b8( 2 );
    tQ8 c8( Q8CONST( -2,3 ) );
    tQ8 d8( a8 );
    tQ8 e8( a4 );
//  tQ8 f8( a12 );      // Warning: left shift count is negative
    tQ8 g8( a12.roundedTo< tQ8 >() );
    tQ8 h8( a12.roundedTo< 8 >() );
    
//  a8 = a4.roundedTo( h8 );  //Warning: left/right shift is negative
    
    a8 = 1;
    a8 = -2;
    a8 = 3;
    a8 = Q8CONST( 3,001 );
    a8 = tQ8( 3.001 );
    a8 = tQ8::truncated( 3.001 );
    a8 = tQ8::rounded( 3.001 );
    a8.setTruncated( 3.2 );
    a8.setRounded( 3.3 );
    a8 = tQ8( 123, 8 );
    a8 = tQ8::create( 123 << 8 );
    
    a8 = a4;
    a8 = a8;
//  a8 = a12;          // Warning: left shift count is negative
    a8 = a12.roundedTo< tQ8 >();
    a8 = a12.roundedTo( a8 );
    a8.setRounded( a12 );
    a8 = -a4;
    a8 = -a8;

    a8 += 3;
    a8 += 4u;
    a8 += 5l;
    a8 += 6lu;
    a8 += tQ8( 3.2 );
    a8 += truncatedTo( a8, 3.3 );
    a8 += roundedTo( a8, 3.4 );
    a8 += a4;
//  a8 += a12;          // Warning: left shift count is negative
    a8 += a12.roundedTo< tQ8 >();
    a8 += a12.roundedTo( a8 );
    a8 = a8 + 2;
    a8 = 3 + a8;
    a8 = a8 + a4;
//  a8 = a4 + a8;       // Warning: left shift count is negative
    
    a8 -= 3;
    a8 -= 4u;
    a8 -= 5l;
    a8 -= 6lu;
    a8 -= roundedTo< tQ8::cQBits >( 3.2 );
    a8 -= roundedTo( a8, 3.3 );
    a8 -= a4;
//  a8 -= a12;          // Warning: left shift count is negative
    a8 -= a12.roundedTo< tQ8 >();
    a8 -= a12.roundedTo( a8 );
    a8 = a8 - 2;
    a8 = 3 - a8;
    a8 = a8 - a4;
//  a8 = a4 - a8;       // Warning: left shift count is negative
    
    a8 *= 3;
    a8 *= 4u;
    a8 *= 5l;
    a8 *= 6lu;
//  a8 *= 3.2;          // Warning: converting to int from double
    a8 *= a4;
    a8 *= a12;
    a8 = a8 * 2;
    a8 = 3 * a8;
    a12 = a8 * a4;
    a12 = a4 * a8;
    
    a8 /= 3;
    a8 /= 4u;
    a8 /= 5l;
    a8 /= 6lu;
//  a8 /= 3.2;          // Warning: converting to int from double
    a8 /= a4;           // Note: possible overflow due to pre-shifting "(a8 << 4) / a4"
//  a8 /= a12;          // Warning: left shift count is negative
    a8 = a8.increasedBy( a12 ) / a12;
    a8 = a8 / 2;
//  a8 = 3 / a8;        // Error: no match for 'operator/'
    a8 = tQ16( 3 ) / a8;
    a12 = a8 / a4;
    a12 = a4 / a8;
    
    a8 == 3;
    a8 == 4u;
    a8 == 5l;
    a8 == 6lu;
    a8 == tQ8( 3.2 );
    a8 == truncatedTo( a8, 3.3 );
    a8 == roundedTo( a8, 3.4 );
    a8 == a4;
//  a8 == a12;          // Warning: left shift count is negative
    a8 == a12.roundedTo< tQ8 >();
    a8 == a12.roundedTo( a8 );
    3 == a8;
    int(4u) == a8;
    int(5l) == a8;
    int(6lu) == a8;
    
    a8 < 3;
    a8 < 4u;
    a8 < 5l;
    a8 < 6lu;
    a8 < tQ8( 3.2 );
    a8 < a4;
//  a8 < a12;          // Warning: left shift count is negative
    3 < a8;
    int(4u) < a8;
    int(5l) < a8;
    int(6lu) < a8;
    
    a8 > 3;
    a8 > 4u;
    a8 > 5l;
    a8 > 6lu;
    a8 > tQ8( 3.2 );
    a8 > a4;
//  a8 > a12;          // Warning: left shift count is negative
    3 > a8;
    int(4u) > a8;
    int(5l) > a8;
    int(6lu) > a8;
    
    !a8;
    int intPart = a8.intPart();
    int fracPart = a8.fracPart();
    int fracPlaces = a8.fracPlaces( 3 );
    unsigned abs = a8.absolute();

    ad = a8.toDouble();
    a8 = ad;
//  a8.set( ad );    // Warning: conversion from int to double, possible loss of data
    a8.setRounded( ad );
    a8 = truncatedTo( a8, ad );
    a8 = truncatedTo<8>( ad );
    a8 = truncatedTo<tQ8>( ad );
    
//  tBigQ36 aB36( 123567890 );     // Error: ambiguous
    tBigQ36 aB36( 123567890ll );
    tBigQ36 bB36( a8 );
    
    aB36 = tBigQ18( a18 ) * a18;
}
